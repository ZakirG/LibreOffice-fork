'use client';

import { useState, useEffect } from 'react';
import { useAuth } from '@clerk/nextjs';
import { Descendant } from 'slate';
import SlateEditor from './SlateEditor';
import Toolbar from './Toolbar';

interface DocumentEditorProps {
  documentId: string;
  fileName?: string;
}

// Define proper Slate element types
interface ParagraphElement {
  type: 'paragraph';
  children: Descendant[];
}

const defaultValue: Descendant[] = [
  {
    type: 'paragraph',
    children: [{ text: '' }],
  } as ParagraphElement,
];

export default function DocumentEditor({ documentId, fileName }: DocumentEditorProps) {
  const { getToken } = useAuth();
  const [value, setValue] = useState<Descendant[]>(defaultValue);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [isDirty, setIsDirty] = useState(false);
  const [isSaving, setIsSaving] = useState(false);
  const [saveStatus, setSaveStatus] = useState<'idle' | 'saving' | 'saved' | 'error'>('idle');
  const [editor, setEditor] = useState<any>(null);
  const [documentName, setDocumentName] = useState<string>(fileName || `Document ${documentId}`);
  const [originalContentType, setOriginalContentType] = useState<string>('application/json');
  const [isPlainTextDocument, setIsPlainTextDocument] = useState<boolean>(false);

  useEffect(() => {
    fetchDocument();
  }, [documentId]);

  const fetchDocument = async () => {
    try {
      setLoading(true);
      setError(null);

      // Get auth token
      const token = await getToken();
      if (!token) {
        throw new Error('Authentication required. Please sign in again.');
      }

      // First, try to get document metadata to get the proper filename
      try {
        const metadataResponse = await fetch(`/api/documents/${documentId}`, {
          method: 'GET',
          headers: {
            'Authorization': `Bearer ${token}`,
          },
        });
        
        if (metadataResponse.ok) {
          const metadata = await metadataResponse.json();
          if (metadata.document) {
            if (metadata.document.fileName) {
              setDocumentName(metadata.document.fileName);
            }
            if (metadata.document.contentType) {
              setOriginalContentType(metadata.document.contentType);
              setIsPlainTextDocument(metadata.document.contentType === 'text/plain');
            }
          }
        }
      } catch (metadataError) {
        console.warn('Could not fetch document metadata:', metadataError);
        // Continue with document loading even if metadata fetch fails
      }

      // Get presigned URL for the document
      const presignResponse = await fetch('/api/presign', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`,
        },
        body: JSON.stringify({
          docId: documentId,
          mode: 'get',
        }),
      });

      if (!presignResponse.ok) {
        if (presignResponse.status === 404) {
          // Document doesn't exist yet, use default empty value
          setValue(defaultValue);
          setLoading(false);
          return;
        }
        throw new Error(`Failed to get presigned URL: ${presignResponse.status}`);
      }

      const { presignedUrl } = await presignResponse.json();

      // Fetch document content from S3
      const contentResponse = await fetch(presignedUrl);
      
      if (!contentResponse.ok) {
        if (contentResponse.status === 404) {
          // Document doesn't exist yet, use default empty value
          setValue(defaultValue);
          setLoading(false);
          return;
        }
        throw new Error(`Failed to fetch document: ${contentResponse.status}`);
      }

      const contentText = await contentResponse.text();
      
      if (contentText.trim() === '') {
        // Empty document, use default value
        setValue(defaultValue);
      } else {
        try {
          const parsedContent = JSON.parse(contentText);
          // Validate that it's a valid Slate document structure
          if (Array.isArray(parsedContent)) {
            setValue(parsedContent);
          } else {
            throw new Error('Invalid document format');
          }
        } catch (parseError) {
                           // If it's not JSON, check if it looks like plain text
                 if (contentText.length < 1000 && !contentText.includes('%PDF') && !contentText.includes('PK\x03\x04')) {
                   // Treat as plain text and convert to Slate format
                   setIsPlainTextDocument(true);
                   setOriginalContentType('text/plain');
                   setValue([
                     {
                       type: 'paragraph',
                       children: [{ text: contentText }],
                     } as ParagraphElement,
                   ]);
                 } else {
                   throw new Error('This document is not editable in the text editor. It appears to be an uploaded file (PDF, Word, etc.). Use the "New Document" button to create a new editable document, or click "Download" to get the original file.');
                 }
        }
      }

    } catch (err) {
      console.error('Error fetching document:', err);
      setError(err instanceof Error ? err.message : 'Failed to load document');
    } finally {
      setLoading(false);
    }
  };

  const handleChange = (newValue: Descendant[]) => {
    setValue(newValue);
    setIsDirty(true);
    setSaveStatus('idle'); // Reset save status when document is changed
  };

  const handleSave = async () => {
    if (!isDirty) return;

    try {
      setIsSaving(true);
      setSaveStatus('saving');
      setError(null);

      // Get auth token
      const token = await getToken();
      if (!token) {
        throw new Error('Authentication required. Please sign in again.');
      }

      // Prepare content based on original document type
      let documentContent: string;
      let contentType: string;
      let fileName: string;

      if (isPlainTextDocument && originalContentType === 'text/plain') {
        // Convert Slate content back to plain text
        documentContent = value
          .map((node: any) => {
            if (node.type === 'paragraph') {
              return node.children.map((child: any) => child.text || '').join('');
            }
            return '';
          })
          .join('\n');
        contentType = 'text/plain';
        fileName = documentName.endsWith('.txt') ? documentName : `${documentName}.txt`;
      } else {
        // Save as Slate JSON format
        documentContent = JSON.stringify(value);
        contentType = 'application/json';
        fileName = documentName.endsWith('.slate') ? documentName : `${documentName}.slate`;
      }

      // Get presigned PUT URL
      const presignResponse = await fetch('/api/presign', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`,
        },
        body: JSON.stringify({
          docId: documentId,
          mode: 'put',
          fileName: fileName,
          contentType: contentType,
          fileSize: new Blob([documentContent]).size,
        }),
      });

      if (!presignResponse.ok) {
        throw new Error(`Failed to get presigned URL: ${presignResponse.status}`);
      }

      const { presignedUrl } = await presignResponse.json();

      // Upload to S3
      const uploadResponse = await fetch(presignedUrl, {
        method: 'PUT',
        headers: {
          'Content-Type': contentType,
        },
        body: documentContent,
      });

      if (!uploadResponse.ok) {
        throw new Error(`Failed to save document: ${uploadResponse.status}`);
      }

      // Update metadata in DynamoDB
      const metadataResponse = await fetch(`/api/documents/${documentId}`, {
        method: 'PATCH',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`,
        },
        body: JSON.stringify({
          lastModified: new Date().toISOString(),
          fileSize: new Blob([documentContent]).size,
          fileName: fileName,
        }),
      });

      if (!metadataResponse.ok) {
        // Don't fail the entire save if metadata update fails, just log it
        console.warn('Failed to update document metadata:', metadataResponse.status);
      }

      setIsDirty(false);
      setSaveStatus('saved');

      // Show saved status for 3 seconds
      setTimeout(() => {
        if (!isDirty) {
          setSaveStatus('idle');
        }
      }, 3000);

    } catch (err) {
      console.error('Error saving document:', err);
      setError(err instanceof Error ? err.message : 'Failed to save document');
      setSaveStatus('error');
    } finally {
      setIsSaving(false);
    }
  };

  if (loading) {
    return (
      <div className="bg-white rounded-lg shadow p-6">
        <div className="flex items-center justify-center py-12">
          <div className="text-center">
            <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-blue-600 mx-auto mb-4"></div>
            <p className="text-gray-600">Loading document...</p>
          </div>
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="bg-white rounded-lg shadow p-6">
        <div className="text-center py-12">
          <div className="text-red-500 text-4xl mb-4">⚠️</div>
          <h3 className="text-lg font-medium text-gray-900 mb-2">Error Loading Document</h3>
          <p className="text-red-600 mb-4">{error}</p>
          <button
            onClick={fetchDocument}
            className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700"
          >
            Try Again
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="bg-white rounded-lg shadow">
      {/* Document Header */}
      <div className="px-6 py-4 border-b border-gray-200">
        <h2 className="text-xl font-semibold text-gray-900">{documentName}</h2>
      </div>
      
      {/* Editor Area */}
      <div className="p-6">
        <Toolbar 
          editor={editor}
          onSave={handleSave}
          isSaving={isSaving}
          isDirty={isDirty}
          saveStatus={saveStatus}
        />
        <SlateEditor 
          initialValue={value}
          onChange={handleChange}
          onEditorReady={setEditor}
        />
      </div>
    </div>
  );
} 