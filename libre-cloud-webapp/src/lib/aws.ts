import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import { DynamoDBDocumentClient } from '@aws-sdk/lib-dynamodb';
import { S3Client } from '@aws-sdk/client-s3';
import { CognitoIdentityProviderClient } from '@aws-sdk/client-cognito-identity-provider';

const region = process.env.AWS_REGION || 'us-east-1';

// DynamoDB Client
export const dynamoDbClient = new DynamoDBClient({
  region,
  credentials: {
    accessKeyId: process.env.AWS_ACCESS_KEY_ID!,
    secretAccessKey: process.env.AWS_SECRET_ACCESS_KEY!,
  },
});

export const dynamoDbDocClient = DynamoDBDocumentClient.from(dynamoDbClient);

// S3 Client
export const s3Client = new S3Client({
  region,
  credentials: {
    accessKeyId: process.env.AWS_ACCESS_KEY_ID!,
    secretAccessKey: process.env.AWS_SECRET_ACCESS_KEY!,
  },
});

// Cognito Client
export const cognitoClient = new CognitoIdentityProviderClient({
  region,
  credentials: {
    accessKeyId: process.env.AWS_ACCESS_KEY_ID!,
    secretAccessKey: process.env.AWS_SECRET_ACCESS_KEY!,
  },
});

// Environment configuration
export const awsConfig = {
  region,
  cognitoUserPoolId: process.env.COGNITO_USER_POOL_ID!,
  cognitoClientId: process.env.COGNITO_CLIENT_ID!,
  cognitoDomain: process.env.COGNITO_DOMAIN!,
  s3BucketName: process.env.S3_BUCKET_NAME!,
  documentsTableName: process.env.DYNAMODB_DOCUMENTS_TABLE_NAME!,
  desktopLoginTableName: process.env.DYNAMODB_DESKTOP_LOGIN_TABLE_NAME!,
} as const; 