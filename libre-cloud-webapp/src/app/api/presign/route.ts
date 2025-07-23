import { NextRequest, NextResponse } from 'next/server';
import { validateDesktopJWT } from '@/lib/jwt';
import { generatePresignedUrl, isAllowedFileType, getFileExtension } from '@/lib/s3';
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
    const jwtPayload = validateDesktopJWT(request);
    if (!jwtPayload) {
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

    // For GET requests, validate docId
    if (mode === 'get' && !docId) {
      return NextResponse.json(
        { error: 'docId is required for GET operations' },
        { status: 400 }
      );
    }

    // Generate document ID for PUT operations
    const finalDocId = mode === 'put' ? (docId || uuidv4()) : docId;
    
    // Add file extension if not present
    let finalFileName = fileName;
    if (mode === 'put' && fileName && contentType) {
      const extension = getFileExtension(contentType);
      if (extension && !fileName.endsWith(extension)) {
        finalFileName = fileName + extension;
      }
    }

    // Generate presigned URL
    const presignedUrl = await generatePresignedUrl({
      userId: jwtPayload.userId,
      docId: finalDocId,
      fileName: finalFileName,
      contentType: mode === 'put' ? contentType : undefined,
      operation: mode,
      expiresIn: 60, // 60 seconds for security
    });

    logger.info('presign-api', 'Presigned URL generated successfully', {
      userId: jwtPayload.userId,
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