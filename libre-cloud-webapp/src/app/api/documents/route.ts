import { NextRequest, NextResponse } from 'next/server';
import { validateAuth } from '@/lib/auth';
import { getUserDocuments, createDocument, updateDocument, deleteDocument, type Document } from '@/lib/dynamodb';
import { deleteS3Object } from '@/lib/s3';
import { logger } from '@/lib/logging';
import { v4 as uuidv4 } from 'uuid';

export async function GET(request: NextRequest) {
  const requestId = uuidv4();
  
  try {
    logger.info('documents-api', 'Documents list request initiated', undefined, {
      requestId,
      ip: request.headers.get('x-forwarded-for') || 'unknown'
    });

    // Validate JWT token
    const authPayload = await validateAuth(request);
    if (!authPayload) {
      logger.warn('documents-api', 'Invalid JWT token in documents request', undefined, {
        requestId
      });
      return NextResponse.json(
        { error: 'Invalid or missing authorization token' },
        { status: 401 }
      );
    }

    // Get user documents from DynamoDB
    const documents = await getUserDocuments(authPayload.userId);

    logger.info('documents-api', 'Documents retrieved successfully', {
      userId: authPayload.userId,
      documentCount: documents.length
    }, {
      requestId
    });

    return NextResponse.json({
      documents,
      total: documents.length
    });

  } catch (error) {
    logger.error('documents-api', 'Error retrieving documents', error, {
      requestId
    });

    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
}

export async function POST(request: NextRequest) {
  const requestId = uuidv4();
  
  try {
    logger.info('documents-api', 'Document registration request initiated', undefined, {
      requestId,
      ip: request.headers.get('x-forwarded-for') || 'unknown'
    });

    // Validate JWT token
    const authPayload = await validateAuth(request);
    if (!authPayload) {
      logger.warn('documents-api', 'Invalid JWT token in document registration', undefined, {
        requestId
      });
      return NextResponse.json(
        { error: 'Invalid or missing authorization token' },
        { status: 401 }
      );
    }

    // Parse request body
    const body = await request.json();
    const { docId, fileName, fileSize, contentType } = body;

    // Validate required fields
    if (!docId || !fileName || !fileSize) {
      logger.warn('documents-api', 'Missing required fields for document registration', {
        docId, fileName, fileSize
      }, {
        requestId
      });
      return NextResponse.json(
        { error: 'docId, fileName, and fileSize are required' },
        { status: 400 }
      );
    }

    // Validate file size is positive
    if (typeof fileSize !== 'number' || fileSize <= 0) {
      return NextResponse.json(
        { error: 'fileSize must be a positive number' },
        { status: 400 }
      );
    }

    // Validate docId format (should be UUID)
    const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;
    if (!uuidRegex.test(docId)) {
      return NextResponse.json(
        { error: 'docId must be a valid UUID' },
        { status: 400 }
      );
    }

    // Create document metadata
    const now = new Date().toISOString();
    const document: Document = {
      userId: authPayload.userId,
      docId,
      fileName,
      fileSize,
      uploadedAt: now,
      lastModified: now,
      ...(contentType && { contentType })
    };

    // Store document metadata in DynamoDB
    await createDocument(document);

    logger.info('documents-api', 'Document registered successfully', {
      userId: authPayload.userId,
      docId,
      fileName,
      fileSize
    }, {
      requestId
    });

    return NextResponse.json({
      success: true,
      document
    }, { status: 201 });

  } catch (error) {
    logger.error('documents-api', 'Error registering document', error, {
      requestId
    });

    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
}

export async function DELETE(request: NextRequest) {
  const requestId = uuidv4();
  
  try {
    logger.info('documents-api', 'Document deletion request initiated', undefined, {
      requestId,
      ip: request.headers.get('x-forwarded-for') || 'unknown'
    });

    // Validate JWT token
    const authPayload = await validateAuth(request);
    if (!authPayload) {
      logger.warn('documents-api', 'Invalid JWT token in document deletion', undefined, {
        requestId
      });
      return NextResponse.json(
        { error: 'Invalid or missing authorization token' },
        { status: 401 }
      );
    }

    // Get docId from URL parameters
    const { searchParams } = new URL(request.url);
    const docId = searchParams.get('docId');

    if (!docId) {
      return NextResponse.json(
        { error: 'docId parameter is required' },
        { status: 400 }
      );
    }

    // Validate docId format (should be UUID)
    const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;
    if (!uuidRegex.test(docId)) {
      return NextResponse.json(
        { error: 'docId must be a valid UUID' },
        { status: 400 }
      );
    }

    // Delete from S3 and DynamoDB
    await Promise.all([
      deleteS3Object(authPayload.userId, docId),
      deleteDocument(authPayload.userId, docId)
    ]);

    logger.info('documents-api', 'Document deleted successfully', {
      userId: authPayload.userId,
      docId
    }, {
      requestId
    });

    return NextResponse.json({
      success: true,
      message: 'Document deleted successfully'
    });

  } catch (error) {
    logger.error('documents-api', 'Error deleting document', error, {
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
      'Access-Control-Allow-Methods': 'GET, POST, DELETE, OPTIONS',
      'Access-Control-Allow-Headers': 'Content-Type, Authorization',
    },
  });
} 