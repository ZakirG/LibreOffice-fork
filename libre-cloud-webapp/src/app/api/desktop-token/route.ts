import { NextRequest, NextResponse } from 'next/server';
import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import { DynamoDBDocumentClient, GetCommand, UpdateCommand } from '@aws-sdk/lib-dynamodb';

const dynamoClient = new DynamoDBClient({
  region: process.env.AWS_REGION,
});
const docClient = DynamoDBDocumentClient.from(dynamoClient);

export async function GET(request: NextRequest) {
  try {
    const { searchParams } = new URL(request.url);
    const nonce = searchParams.get('nonce');

    if (!nonce) {
      return NextResponse.json(
        { error: 'Nonce is required' },
        { status: 400 }
      );
    }

    // Check nonce status in DynamoDB
    const getCommand = new GetCommand({
      TableName: process.env.DYNAMODB_DESKTOP_LOGIN_TABLE_NAME,
      Key: { nonce },
    });

    const result = await docClient.send(getCommand);

    if (!result.Item) {
      return NextResponse.json(
        { error: 'Invalid or expired nonce' },
        { status: 404 }
      );
    }

    const { status, userId, expiresAt } = result.Item;

    // Check if nonce has expired
    if (Date.now() > expiresAt) {
      return NextResponse.json(
        { error: 'Nonce has expired' },
        { status: 410 }
      );
    }

    // If status is still pending, return waiting response
    if (status === 'pending') {
      return NextResponse.json(
        { status: 'pending', message: 'Authentication still pending' },
        { status: 202 }
      );
    }

    // If status is ready, generate and return JWT
    if (status === 'ready' && userId) {
      try {
        // Get user data from Clerk (simplified approach)
        const { data: userData } = result.Item;
        
        // Create a simple token with user data stored during authentication
        const tokenData = {
          userId: userId,
          email: userData?.email || '',
          firstName: userData?.firstName || '',
          lastName: userData?.lastName || '',
          type: 'desktop',
          iat: Math.floor(Date.now() / 1000),
          exp: Math.floor(Date.now() / 1000) + 3600, // 1 hour
        };

        // Mark nonce as consumed
        await docClient.send(new UpdateCommand({
          TableName: process.env.DYNAMODB_DESKTOP_LOGIN_TABLE_NAME,
          Key: { nonce },
          UpdateExpression: 'SET #status = :consumed',
          ExpressionAttributeNames: {
            '#status': 'status'
          },
          ExpressionAttributeValues: {
            ':consumed': 'consumed'
          }
        }));

        return NextResponse.json({
          token: Buffer.from(JSON.stringify(tokenData)).toString('base64'), // Simple base64 encoded token
          expiresAt: tokenData.exp,
          user: {
            id: userId,
            email: tokenData.email,
            firstName: tokenData.firstName,
            lastName: tokenData.lastName,
          }
        });
      } catch (userError) {
        console.error('Error fetching user data:', userError);
        return NextResponse.json(
          { error: 'Failed to generate token' },
          { status: 500 }
        );
      }
    }

    return NextResponse.json(
      { error: 'Invalid nonce status' },
      { status: 400 }
    );

  } catch (error) {
    console.error('Error in desktop-token API:', error);
    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
} 