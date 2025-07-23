import { auth } from '@clerk/nextjs/server';
import { NextRequest, NextResponse } from 'next/server';
import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import { DynamoDBDocumentClient, UpdateCommand, GetCommand } from '@aws-sdk/lib-dynamodb';

const client = new DynamoDBClient({
  region: process.env.AWS_REGION,
  credentials: {
    accessKeyId: process.env.AWS_ACCESS_KEY_ID!,
    secretAccessKey: process.env.AWS_SECRET_ACCESS_KEY!,
  },
});

const docClient = DynamoDBDocumentClient.from(client);

interface RouteParams {
  params: Promise<{
    id: string;
  }>;
}

export async function PATCH(request: NextRequest, context: RouteParams) {
  try {
    const { userId } = await auth();

    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { id: documentId } = await context.params;

    if (!documentId) {
      return NextResponse.json({ error: 'Document ID is required' }, { status: 400 });
    }

    const body = await request.json();
    const { lastModified, fileName, fileSize } = body;

    // Build update expression dynamically based on provided fields
    const updateExpressionParts: string[] = [];
    const expressionAttributeValues: Record<string, any> = {};
    const expressionAttributeNames: Record<string, string> = {};

    if (lastModified) {
      updateExpressionParts.push('#lastModified = :lastModified');
      expressionAttributeNames['#lastModified'] = 'lastModified';
      expressionAttributeValues[':lastModified'] = lastModified;
    } else {
      // Always update lastModified to current timestamp if not provided
      updateExpressionParts.push('#lastModified = :lastModified');
      expressionAttributeNames['#lastModified'] = 'lastModified';
      expressionAttributeValues[':lastModified'] = new Date().toISOString();
    }

    if (fileName) {
      updateExpressionParts.push('#fileName = :fileName');
      expressionAttributeNames['#fileName'] = 'fileName';
      expressionAttributeValues[':fileName'] = fileName;
    }

    if (fileSize !== undefined) {
      updateExpressionParts.push('#fileSize = :fileSize');
      expressionAttributeNames['#fileSize'] = 'fileSize';
      expressionAttributeValues[':fileSize'] = fileSize;
    }

    if (updateExpressionParts.length === 0) {
      return NextResponse.json({ error: 'No valid fields to update' }, { status: 400 });
    }

    const updateExpression = 'SET ' + updateExpressionParts.join(', ');

    const command = new UpdateCommand({
      TableName: process.env.DYNAMODB_DOCUMENTS_TABLE_NAME,
      Key: {
        userId: userId,
        docId: documentId,
      },
      UpdateExpression: updateExpression,
      ExpressionAttributeNames: expressionAttributeNames,
      ExpressionAttributeValues: expressionAttributeValues,
      ConditionExpression: 'attribute_exists(userId)', // Ensure document exists and belongs to user
      ReturnValues: 'ALL_NEW',
    });

    const result = await docClient.send(command);

    return NextResponse.json({
      success: true,
      document: result.Attributes,
    });

  } catch (error: any) {
    console.error('Error updating document metadata:', error);

    if (error.name === 'ConditionalCheckFailedException') {
      return NextResponse.json(
        { error: 'Document not found or access denied' },
        { status: 404 }
      );
    }

    return NextResponse.json(
      { error: 'Failed to update document metadata' },
      { status: 500 }
    );
  }
}

export async function GET(request: NextRequest, context: RouteParams) {
  try {
    const { userId } = await auth();

    if (!userId) {
      return NextResponse.json({ error: 'Unauthorized' }, { status: 401 });
    }

    const { id: documentId } = await context.params;

    if (!documentId) {
      return NextResponse.json({ error: 'Document ID is required' }, { status: 400 });
    }

    // Get document from DynamoDB
    const command = new GetCommand({
      TableName: process.env.DYNAMODB_DOCUMENTS_TABLE_NAME,
      Key: {
        userId: userId,
        docId: documentId,
      },
    });

    const result = await docClient.send(command);

    if (!result.Item) {
      return NextResponse.json({ error: 'Document not found' }, { status: 404 });
    }

    return NextResponse.json({
      success: true,
      document: result.Item,
    });

  } catch (error: any) {
    console.error('Error fetching document metadata:', error);

    return NextResponse.json(
      { error: 'Failed to fetch document metadata' },
      { status: 500 }
    );
  }
} 