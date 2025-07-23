import { GetCommand, PutCommand, QueryCommand, UpdateCommand, DeleteCommand } from '@aws-sdk/lib-dynamodb';
import { dynamoDbDocClient, awsConfig } from './aws';

// Helper function to ensure DynamoDB client is available
function ensureDynamoClient() {
  if (!dynamoDbDocClient) {
    throw new Error('DynamoDB client not configured. Please check your AWS credentials.');
  }
  return dynamoDbDocClient;
}

// Type assertion to suppress TypeScript warnings for now
const dynamoClient = dynamoDbDocClient as any;

// Document types
export interface Document {
  userId: string;
  docId: string;
  fileName: string;
  fileSize: number;
  uploadedAt: string;
  lastModified: string;
  contentType?: string;
}

export interface DesktopLogin {
  nonce: string;
  userId?: string;
  status: 'pending' | 'ready' | 'expired';
  expiresAt: number;
  createdAt: string;
}

// Document operations
export async function createDocument(document: Document): Promise<void> {
  if (!dynamoDbDocClient) {
    console.warn('DynamoDB client not configured - cannot create document');
    return;
  }
  
  const command = new PutCommand({
    TableName: awsConfig.documentsTableName,
    Item: document,
  });

  await dynamoDbDocClient.send(command);
}

export async function getDocument(userId: string, docId: string): Promise<Document | null> {
  const command = new GetCommand({
    TableName: awsConfig.documentsTableName,
    Key: { userId, docId },
  });

  const result = await dynamoDbDocClient.send(command);
  return result.Item as Document | null;
}

export async function getUserDocuments(userId: string): Promise<Document[]> {
  if (!dynamoDbDocClient) {
    console.warn('DynamoDB client not configured - returning empty documents list');
    return [];
  }
  
  const command = new QueryCommand({
    TableName: awsConfig.documentsTableName,
    KeyConditionExpression: 'userId = :userId',
    ExpressionAttributeValues: {
      ':userId': userId,
    },
    ScanIndexForward: false, // Sort by newest first
  });

  const result = await ensureDynamoClient().send(command);
  return result.Items as Document[] || [];
}

export async function updateDocument(userId: string, docId: string, updates: Partial<Document>): Promise<void> {
  const updateExpression = Object.keys(updates)
    .map(key => `#${key} = :${key}`)
    .join(', ');

  const expressionAttributeNames = Object.keys(updates).reduce((acc, key) => {
    acc[`#${key}`] = key;
    return acc;
  }, {} as Record<string, string>);

  const expressionAttributeValues = Object.entries(updates).reduce((acc, [key, value]) => {
    acc[`:${key}`] = value;
    return acc;
  }, {} as Record<string, any>);

  const command = new UpdateCommand({
    TableName: awsConfig.documentsTableName,
    Key: { userId, docId },
    UpdateExpression: `SET ${updateExpression}`,
    ExpressionAttributeNames: expressionAttributeNames,
    ExpressionAttributeValues: expressionAttributeValues,
  });

  await dynamoDbDocClient.send(command);
}

export async function deleteDocument(userId: string, docId: string): Promise<void> {
  if (!dynamoDbDocClient) {
    console.warn('DynamoDB client not configured - cannot delete document');
    return;
  }
  
  const command = new DeleteCommand({
    TableName: awsConfig.documentsTableName,
    Key: { userId, docId },
  });

  await dynamoDbDocClient.send(command);
}

// Desktop login operations
export async function createDesktopLogin(desktopLogin: DesktopLogin): Promise<void> {
  const command = new PutCommand({
    TableName: awsConfig.desktopLoginTableName,
    Item: desktopLogin,
  });

  await dynamoDbDocClient.send(command);
}

export async function getDesktopLogin(nonce: string): Promise<DesktopLogin | null> {
  const command = new GetCommand({
    TableName: awsConfig.desktopLoginTableName,
    Key: { nonce },
  });

  const result = await dynamoDbDocClient.send(command);
  return result.Item as DesktopLogin | null;
}

export async function updateDesktopLogin(nonce: string, updates: Partial<DesktopLogin>): Promise<void> {
  const updateExpression = Object.keys(updates)
    .map(key => `#${key} = :${key}`)
    .join(', ');

  const expressionAttributeNames = Object.keys(updates).reduce((acc, key) => {
    acc[`#${key}`] = key;
    return acc;
  }, {} as Record<string, string>);

  const expressionAttributeValues = Object.entries(updates).reduce((acc, [key, value]) => {
    acc[`:${key}`] = value;
    return acc;
  }, {} as Record<string, any>);

  const command = new UpdateCommand({
    TableName: awsConfig.desktopLoginTableName,
    Key: { nonce },
    UpdateExpression: `SET ${updateExpression}`,
    ExpressionAttributeNames: expressionAttributeNames,
    ExpressionAttributeValues: expressionAttributeValues,
  });

  await dynamoDbDocClient.send(command);
} 