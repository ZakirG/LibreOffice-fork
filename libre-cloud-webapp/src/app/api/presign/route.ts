import { NextRequest, NextResponse } from 'next/server';
import { validateAuth } from '@/lib/auth';
import { generatePresignedUrl, isAllowedFileType, getFileExtension } from '@/lib/s3';
import { getDocument } from '@/lib/dynamodb';
import { logger } from '@/lib/logging';
import { v4 as uuidv4 } from 'uuid';

export async function POST(request: NextRequest) {
  const requestId = uuidv4();
  
  try {
    logger.info('presign-api', 'Presign request initiated', undefined, {
      requestId,
      ip: request.headers.get('x-forwarded-for') || 'unknown'
    });

    // Validate JWT token
    const authPayload = await validateAuth(request);
    if (!authPayload) {
      logger.warn('presign-api', 'Invalid JWT token in presign request', undefined, {
        requestId
      });
      return NextResponse.json(
        { error: 'Invalid or missing authorization token' },
        { status: 401 }
      );
    }

    // Parse request body
    const body = await request.json();
    const { mode, fileName, contentType, fileSize, docId } = body;

    // Validate required fields
    if (!mode || !['put', 'get'].includes(mode)) {
      logger.warn('presign-api', 'Invalid mode parameter', { mode }, {
        requestId
      });
      return NextResponse.json(
        { error: 'Mode parameter must be either "put" or "get"' },
        { status: 400 }
      );
    }

    // For PUT requests, validate file details
    if (mode === 'put') {
      if (!fileName || !contentType) {
        return NextResponse.json(
          { error: 'fileName and contentType are required for PUT operations' },
          { status: 400 }
        );
      }

      // Validate file type
      if (!isAllowedFileType(contentType)) {
        logger.warn('presign-api', 'Disallowed file type', { contentType }, {
          requestId
        });
        return NextResponse.json(
          { error: 'File type not allowed. Please use supported document or audio formats.' },
          { status: 400 }
        );
      }

      // Validate file size (50MB limit)
      const maxFileSize = 50 * 1024 * 1024; // 50MB in bytes
      if (fileSize && fileSize > maxFileSize) {
        logger.warn('presign-api', 'File size too large', { fileSize, maxFileSize }, {
          requestId
        });
        return NextResponse.json(
          { error: `File size too large. Maximum allowed size is ${maxFileSize / (1024 * 1024)}MB` },
          { status: 400 }
        );
      }
    }

    // For GET requests, validate docId and fetch document metadata
    let finalFileName = fileName;
    let finalContentType = contentType;
    
    if (mode === 'get') {
      if (!docId) {
        return NextResponse.json(
          { error: 'docId is required for GET operations' },
          { status: 400 }
        );
      }

      // Fetch document metadata to get original filename for proper downloads
      try {
        const document = await getDocument(authPayload.userId, docId);
        if (!document) {
          logger.warn('presign-api', 'Document not found for download', { docId }, {
            requestId
          });
          return NextResponse.json(
            { error: 'Document not found' },
            { status: 404 }
          );
        }
        
        finalFileName = document.fileName;
        finalContentType = document.contentType;
        
        logger.info('presign-api', 'Retrieved document metadata for download', {
          docId,
          fileName: finalFileName,
          contentType: finalContentType
        }, {
          requestId
        });
      } catch (error) {
        logger.error('presign-api', 'Error fetching document metadata', error, {
          requestId
        });
        return NextResponse.json(
          { error: 'Failed to fetch document metadata' },
          { status: 500 }
        );
      }
    }

    // Generate document ID for PUT operations
    const finalDocId = mode === 'put' ? (docId || uuidv4()) : docId;
    
    // Add file extension if not present (for PUT operations)
    if (mode === 'put' && fileName && contentType) {
      const extension = getFileExtension(contentType);
      if (extension && !fileName.endsWith(extension)) {
        finalFileName = fileName + extension;
      }
    }

    // Generate presigned URL
    const presignedUrl = await generatePresignedUrl({
      userId: authPayload.userId,
      docId: finalDocId,
      fileName: finalFileName,
      contentType: finalContentType,
      operation: mode,
      expiresIn: 60, // 60 seconds for security
    });

    logger.info('presign-api', 'Presigned URL generated successfully', {
      userId: authPayload.userId,
      docId: finalDocId,
      mode,
      fileName: finalFileName
    }, {
      requestId
    });

    return NextResponse.json({
      presignedUrl,
      docId: finalDocId,
      expiresIn: 60,
      ...(mode === 'put' && { fileName: finalFileName })
    });

  } catch (error) {
    logger.error('presign-api', 'Error generating presigned URL', error, {
      requestId
    });

    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
}

// Handle OPTIONS for CORS
export async function OPTIONS(request: NextRequest) {
  return new NextResponse(null, {
    status: 200,
    headers: {
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Methods': 'POST, OPTIONS',
      'Access-Control-Allow-Headers': 'Content-Type, Authorization',
    },
  });
} 