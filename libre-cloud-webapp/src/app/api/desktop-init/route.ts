import { NextResponse } from 'next/server';
import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import { DynamoDBDocumentClient, PutCommand } from '@aws-sdk/lib-dynamodb';
import { randomUUID } from 'crypto';

const dynamoClient = new DynamoDBClient({
  region: process.env.AWS_REGION,
});
const docClient = DynamoDBDocumentClient.from(dynamoClient);

export async function POST() {
  try {
    // Generate a unique nonce
    const nonce = randomUUID();
    
    // Set expiration to 5 minutes from now
    const expiresAt = Date.now() + (5 * 60 * 1000);
    
    // Store nonce in DynamoDB with pending status
    const putCommand = new PutCommand({
      TableName: process.env.DYNAMODB_DESKTOP_LOGIN_TABLE_NAME,
      Item: {
        nonce,
        status: 'pending',
        expiresAt,
        createdAt: Date.now(),
      },
    });

    await docClient.send(putCommand);

    // Create the login URL that the desktop client should open
    const baseUrl = process.env.NEXTAUTH_URL || 'http://localhost:3009';
    const loginUrl = `${baseUrl}/desktop-login?nonce=${nonce}`;

    return NextResponse.json({
      nonce,
      loginUrl,
      expiresAt,
      message: 'Open the loginUrl in your browser to authenticate'
    });

  } catch (error) {
    console.error('Error in desktop-init API:', error);
    return NextResponse.json(
      { error: 'Failed to initialize desktop authentication' },
      { status: 500 }
    );
  }
} 