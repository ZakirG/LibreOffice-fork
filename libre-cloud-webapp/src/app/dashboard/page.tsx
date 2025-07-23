'use client';

import { useEffect, useState, useCallback } from 'react';
import { useUser, UserButton, useAuth } from '@clerk/nextjs';
import { redirect } from 'next/navigation';

interface Document {
  userId: string;
  docId: string;
  fileName: string;
  fileSize: number;
  uploadedAt: string;
  lastModified: string;
  contentType?: string;
}

interface UploadProgress {
  fileName: string;
  progress: number;
  status: 'uploading' | 'processing' | 'completed' | 'error';
  error?: string;
}

const ALLOWED_FILE_TYPES = [
  'application/vnd.oasis.opendocument.text',
  'application/vnd.oasis.opendocument.spreadsheet', 
  'application/vnd.oasis.opendocument.presentation',
  'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
  'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
  'application/vnd.openxmlformats-officedocument.presentationml.presentation',
  'application/msword',
  'application/vnd.ms-excel',
  'application/vnd.ms-powerpoint',
  'application/pdf',
  'text/plain',
  'text/rtf',
  'audio/mpeg',
  'audio/wav',
  'audio/flac',
  'audio/ogg'
];

const MAX_FILE_SIZE = 50 * 1024 * 1024; // 50MB

function getFileIcon(contentType?: string): string {
  if (!contentType) return '📄';
  
  if (contentType === 'application/json') return '📝'; // Slate documents
  if (contentType.includes('text')) return '📝';
  if (contentType.includes('word') || contentType.includes('opendocument.text')) return '📄';
  if (contentType.includes('spreadsheet') || contentType.includes('excel')) return '📊';
  if (contentType.includes('presentation') || contentType.includes('powerpoint')) return '📈';
  if (contentType.includes('pdf')) return '📕';
  if (contentType.includes('audio')) return '🎵';
  
  return '📄';
}



function formatFileSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

function formatDate(dateString: string): string {
  return new Date(dateString).toLocaleDateString('en-US', {
    year: 'numeric',
    month: 'short',
    day: 'numeric',
    hour: '2-digit',
    minute: '2-digit'
  });
}

export default function DashboardPage() {
  const { user, isLoaded } = useUser();
  const { getToken } = useAuth();
  const [documents, setDocuments] = useState<Document[]>([]);
  const [filteredDocuments, setFilteredDocuments] = useState<Document[]>([]);
  const [loading, setLoading] = useState(true);
  const [searchTerm, setSearchTerm] = useState('');
  const [uploadProgress, setUploadProgress] = useState<Record<string, UploadProgress>>({});
  const [dragActive, setDragActive] = useState(false);
  const [deleteConfirmId, setDeleteConfirmId] = useState<string | null>(null);

  // Redirect if not loaded or no user
  useEffect(() => {
    if (isLoaded && !user) {
      redirect('/');
    }
  }, [isLoaded, user]);

  // Load documents
  const loadDocuments = useCallback(async () => {
    if (!user) return;
    
    try {
             const response = await fetch('/api/documents', {
         headers: {
           'Authorization': `Bearer ${await getToken()}`
         }
       });

      if (response.ok) {
        const data = await response.json();
        setDocuments(data.documents || []);
      } else {
        console.error('Failed to load documents');
      }
    } catch (error) {
      console.error('Error loading documents:', error);
    } finally {
      setLoading(false);
    }
  }, [user]);

  useEffect(() => {
    if (user) {
      loadDocuments();
    }
  }, [user, loadDocuments]);

  // Filter documents based on search term
  useEffect(() => {
    if (!searchTerm) {
      setFilteredDocuments(documents);
    } else {
      const filtered = documents.filter(doc =>
        doc.fileName.toLowerCase().includes(searchTerm.toLowerCase())
      );
      setFilteredDocuments(filtered);
    }
  }, [documents, searchTerm]);

  // File validation
  const validateFile = (file: File): string | null => {
    if (!ALLOWED_FILE_TYPES.includes(file.type)) {
      return `File type "${file.type}" is not allowed. Please use supported document or audio formats.`;
    }
    
    if (file.size > MAX_FILE_SIZE) {
      return `File size ${formatFileSize(file.size)} exceeds the maximum allowed size of ${formatFileSize(MAX_FILE_SIZE)}.`;
    }
    
    return null;
  };

  // Upload file function
  const uploadFile = async (file: File) => {
    const validationError = validateFile(file);
    if (validationError) {
      setUploadProgress(prev => ({
        ...prev,
        [file.name]: {
          fileName: file.name,
          progress: 0,
          status: 'error',
          error: validationError
        }
      }));
      return;
    }

    const uploadId = file.name;
    
    setUploadProgress(prev => ({
      ...prev,
      [uploadId]: {
        fileName: file.name,
        progress: 0,
        status: 'uploading'
      }
    }));

         try {
       // Get presigned URL
       const token = await getToken();
       if (!token) throw new Error('No auth token');

      const presignResponse = await fetch('/api/presign', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({
          mode: 'put',
          fileName: file.name,
          contentType: file.type,
          fileSize: file.size
        })
      });

      if (!presignResponse.ok) {
        const error = await presignResponse.json();
        throw new Error(error.error || 'Failed to get upload URL');
      }

      const { presignedUrl, docId, fileName } = await presignResponse.json();

      // Upload to S3
      setUploadProgress(prev => ({
        ...prev,
        [uploadId]: { ...prev[uploadId], progress: 50 }
      }));

      const uploadResponse = await fetch(presignedUrl, {
        method: 'PUT',
        body: file,
        headers: {
          'Content-Type': file.type
        }
      });

      if (!uploadResponse.ok) {
        throw new Error('Failed to upload file to S3');
      }

      // Register document metadata
      setUploadProgress(prev => ({
        ...prev,
        [uploadId]: { ...prev[uploadId], progress: 75, status: 'processing' }
      }));

      const metadataResponse = await fetch('/api/documents', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({
          docId,
          fileName: fileName || file.name,
          fileSize: file.size,
          contentType: file.type
        })
      });

      if (!metadataResponse.ok) {
        const error = await metadataResponse.json();
        throw new Error(error.error || 'Failed to register document');
      }

      // Success
      setUploadProgress(prev => ({
        ...prev,
        [uploadId]: { ...prev[uploadId], progress: 100, status: 'completed' }
      }));

      // Remove progress after 3 seconds and reload documents
      setTimeout(() => {
        setUploadProgress(prev => {
          const newProgress = { ...prev };
          delete newProgress[uploadId];
          return newProgress;
        });
      }, 3000);

      await loadDocuments();

    } catch (error) {
      setUploadProgress(prev => ({
        ...prev,
        [uploadId]: {
          ...prev[uploadId],
          status: 'error',
          error: error instanceof Error ? error.message : 'Upload failed'
        }
      }));
    }
  };

  // Handle file input change
  const handleFileInput = (event: React.ChangeEvent<HTMLInputElement>) => {
    const files = event.target.files;
    if (files) {
      Array.from(files).forEach(uploadFile);
      event.target.value = ''; // Reset input
    }
  };

  // Drag and drop handlers
  const handleDrag = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
  };

  const handleDragIn = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setDragActive(true);
  };

  const handleDragOut = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setDragActive(false);
  };

  const handleDrop = (e: React.DragEvent) => {
    e.preventDefault();
    e.stopPropagation();
    setDragActive(false);
    
    if (e.dataTransfer.files && e.dataTransfer.files.length > 0) {
      Array.from(e.dataTransfer.files).forEach(uploadFile);
    }
  };

  // Download document
  const downloadDocument = async (docId: string, fileName: string) => {
    try {
      const token = await getToken();
      if (!token) throw new Error('No auth token');

      const response = await fetch('/api/presign', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({
          mode: 'get',
          docId
        })
      });

      if (!response.ok) {
        throw new Error('Failed to get download URL');
      }

      const { presignedUrl } = await response.json();
      
      // Create a temporary link and click it to download
      const link = document.createElement('a');
      link.href = presignedUrl;
      link.download = fileName;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
    } catch (error) {
      console.error('Download failed:', error);
    }
  };

  // Delete document
  const deleteDocument = async (docId: string) => {
    try {
      const token = await getToken();
      if (!token) throw new Error('No auth token');

      const response = await fetch(`/api/documents?docId=${docId}`, {
        method: 'DELETE',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });

      if (!response.ok) {
        throw new Error('Failed to delete document');
      }

      await loadDocuments();
      setDeleteConfirmId(null);
    } catch (error) {
      console.error('Delete failed:', error);
    }
  };

  if (!isLoaded || !user) {
    return <div className="min-h-screen bg-gray-50 flex items-center justify-center">
      <div className="text-lg">Loading...</div>
    </div>;
  }

  return (
    <div className="min-h-screen bg-gray-50">
      {/* Header */}
      <header className="bg-white shadow-sm border-b">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <h1 className="text-2xl font-bold text-gray-900">LibreCloud Dashboard</h1>
            
            <div className="flex items-center space-x-4">
              <span className="text-sm text-gray-600">
                Welcome, {user.emailAddresses[0]?.emailAddress || user.firstName || 'User'}
              </span>
              <UserButton afterSignOutUrl="/" />
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        {/* Upload Progress */}
        {Object.keys(uploadProgress).length > 0 && (
          <div className="mb-6">
            <h3 className="text-lg font-medium text-gray-900 mb-3">Upload Progress</h3>
            <div className="space-y-2">
              {Object.values(uploadProgress).map((progress) => (
                <div key={progress.fileName} className="bg-white p-3 rounded-lg shadow-sm border">
                  <div className="flex items-center justify-between mb-2">
                    <span className="text-sm font-medium text-gray-700">{progress.fileName}</span>
                    <span className="text-sm text-gray-500">
                      {progress.status === 'error' ? 'Failed' : `${progress.progress}%`}
                    </span>
                  </div>
                  {progress.status !== 'error' && (
                    <div className="w-full bg-gray-200 rounded-full h-2">
                      <div 
                        className="bg-blue-600 h-2 rounded-full transition-all duration-300"
                        style={{ width: `${progress.progress}%` }}
                      />
                    </div>
                  )}
                  {progress.error && (
                    <p className="text-sm text-red-600 mt-1">{progress.error}</p>
                  )}
                </div>
              ))}
            </div>
          </div>
        )}

        {/* Upload Area */}
        <div className="mb-8">
          <div 
            className={`border-2 border-dashed rounded-lg p-8 text-center transition-colors ${
              dragActive ? 'border-blue-400 bg-blue-50' : 'border-gray-300 bg-white'
            }`}
            onDragEnter={handleDragIn}
            onDragLeave={handleDragOut}
            onDragOver={handleDrag}
            onDrop={handleDrop}
          >
            <div className="text-6xl mb-4">📁</div>
            <h3 className="text-lg font-medium text-gray-900 mb-2">Upload Documents</h3>
            <p className="text-gray-600 mb-4">
              Drag and drop files here, or click to browse
            </p>
            <input
              type="file"
              multiple
              accept={ALLOWED_FILE_TYPES.join(',')}
              onChange={handleFileInput}
              className="hidden"
              id="file-upload"
            />
            <label
              htmlFor="file-upload"
              className="inline-flex items-center px-4 py-2 border border-transparent text-sm font-medium rounded-md text-white bg-blue-600 hover:bg-blue-700 cursor-pointer"
            >
              Choose Files
            </label>
            <p className="text-xs text-gray-500 mt-2">
              Supported formats: LibreOffice, Microsoft Office, PDF, TXT, Audio files. Max size: 50MB
            </p>
          </div>
        </div>

        {/* Search */}
        <div className="mb-6">
          <input
            type="text"
            placeholder="Search documents..."
            value={searchTerm}
            onChange={(e) => setSearchTerm(e.target.value)}
            className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-blue-500"
          />
        </div>

        {/* Documents List */}
        <div className="bg-white rounded-lg shadow">
          <div className="px-6 py-4 border-b border-gray-200 flex justify-between items-center">
            <h2 className="text-xl font-semibold text-gray-900">Your Documents</h2>
            <div className="flex space-x-2">
              <button
                onClick={() => window.open(`/doc/new-${Date.now()}`, '_blank')}
                className="inline-flex items-center px-4 py-2 border border-transparent text-sm font-medium rounded-md text-white bg-green-600 hover:bg-green-700"
              >
                New Text Document
              </button>
            </div>
          </div>
          
          {loading ? (
            <div className="p-8 text-center">
              <div className="text-lg text-gray-600">Loading documents...</div>
            </div>
          ) : filteredDocuments.length === 0 ? (
            <div className="p-8 text-center">
              <div className="text-6xl mb-4">📭</div>
              <h3 className="text-lg font-medium text-gray-900 mb-2">
                {searchTerm ? 'No documents found' : 'No documents yet'}
              </h3>
              <p className="text-gray-600">
                {searchTerm 
                  ? 'Try adjusting your search terms' 
                  : 'Upload your first document to get started'
                }
              </p>
            </div>
          ) : (
            <div className="divide-y divide-gray-200">
              {filteredDocuments.map((doc) => (
                <div key={doc.docId} className="p-6 hover:bg-gray-50">
                  <div className="flex items-center justify-between">
                    <div className="flex items-center space-x-4 flex-1">
                      <div className="text-2xl">{getFileIcon(doc.contentType)}</div>
                      <div className="flex-1 min-w-0">
                        <h3 className="text-sm font-medium text-gray-900 truncate">
                          {doc.fileName}
                        </h3>
                        <p className="text-sm text-gray-500">
                          {formatFileSize(doc.fileSize)} • Uploaded {formatDate(doc.uploadedAt)}
                        </p>
                      </div>
                    </div>
                    
                    <div className="flex items-center space-x-2">
                      <button
                        onClick={() => window.open(`/doc/${doc.docId}`, '_blank')}
                        className="inline-flex items-center px-3 py-1 border border-blue-300 text-sm font-medium rounded-md text-blue-700 bg-blue-50 hover:bg-blue-100"
                      >
                        Edit
                      </button>
                      <button
                        onClick={() => downloadDocument(doc.docId, doc.fileName)}
                        className="inline-flex items-center px-3 py-1 border border-gray-300 text-sm font-medium rounded-md text-gray-700 bg-white hover:bg-gray-50"
                      >
                        Download
                      </button>
                      <button
                        onClick={() => setDeleteConfirmId(doc.docId)}
                        className="inline-flex items-center px-3 py-1 border border-red-300 text-sm font-medium rounded-md text-red-700 bg-red-50 hover:bg-red-100"
                      >
                        Delete
                      </button>
                    </div>
                  </div>
                </div>
              ))}
            </div>
          )}
        </div>
      </main>

      {/* Delete Confirmation Modal */}
      {deleteConfirmId && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
          <div className="bg-white rounded-lg p-6 max-w-sm w-full mx-4">
            <h3 className="text-lg font-medium text-gray-900 mb-2">Delete Document</h3>
            <p className="text-gray-600 mb-4">
              Are you sure you want to delete this document? This action cannot be undone.
            </p>
            <div className="flex space-x-3">
              <button
                onClick={() => deleteDocument(deleteConfirmId)}
                className="flex-1 bg-red-600 text-white px-4 py-2 rounded-md hover:bg-red-700"
              >
                Delete
              </button>
              <button
                onClick={() => setDeleteConfirmId(null)}
                className="flex-1 bg-gray-300 text-gray-700 px-4 py-2 rounded-md hover:bg-gray-400"
              >
                Cancel
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
} 