/**
 * Unit test for AWS SDK client instantiation
 * This test verifies that AWS SDK clients can be instantiated with environment variables
 */

describe('AWS SDK Configuration', () => {
  beforeEach(() => {
    // Mock environment variables
    process.env.AWS_REGION = 'us-east-1';
    process.env.AWS_ACCESS_KEY_ID = 'test-access-key';
    process.env.AWS_SECRET_ACCESS_KEY = 'test-secret-key';
    process.env.COGNITO_USER_POOL_ID = 'test-user-pool-id';
    process.env.COGNITO_CLIENT_ID = 'test-client-id';
    process.env.COGNITO_DOMAIN = 'test-domain';
    process.env.S3_BUCKET_NAME = 'test-bucket';
    process.env.DYNAMODB_DOCUMENTS_TABLE_NAME = 'test-documents-table';
    process.env.DYNAMODB_DESKTOP_LOGIN_TABLE_NAME = 'test-desktop-login-table';
  });

  it('should instantiate AWS SDK clients without errors', () => {
    // Dynamic import to ensure environment variables are set
    const { dynamoDbClient, s3Client, cognitoClient, awsConfig } = require('../aws');

    // Test that clients are instantiated
    expect(dynamoDbClient).toBeDefined();
    expect(s3Client).toBeDefined();
    expect(cognitoClient).toBeDefined();
    
    // Test that config object is properly constructed
    expect(awsConfig).toBeDefined();
    expect(awsConfig.region).toBe('us-east-1');
    expect(awsConfig.cognitoUserPoolId).toBe('test-user-pool-id');
    expect(awsConfig.cognitoClientId).toBe('test-client-id');
    expect(awsConfig.s3BucketName).toBe('test-bucket');
  });

  it('should have correct AWS region configuration', () => {
    const { awsConfig } = require('../aws');
    expect(awsConfig.region).toBe('us-east-1');
  });

  afterEach(() => {
    // Clean up environment variables
    delete process.env.AWS_REGION;
    delete process.env.AWS_ACCESS_KEY_ID;
    delete process.env.AWS_SECRET_ACCESS_KEY;
    delete process.env.COGNITO_USER_POOL_ID;
    delete process.env.COGNITO_CLIENT_ID;
    delete process.env.COGNITO_DOMAIN;
    delete process.env.S3_BUCKET_NAME;
    delete process.env.DYNAMODB_DOCUMENTS_TABLE_NAME;
    delete process.env.DYNAMODB_DESKTOP_LOGIN_TABLE_NAME;
    
    // Clear module cache to ensure fresh imports
    delete require.cache[require.resolve('../aws')];
  });
}); 