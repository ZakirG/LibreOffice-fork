/**
 * @jest-environment node
 */
import { PATCH } from '../[id]/route';

// Mock Clerk
jest.mock('@clerk/nextjs/server', () => ({
  auth: jest.fn(),
}));

// Mock AWS SDK
jest.mock('@aws-sdk/client-dynamodb', () => ({
  DynamoDBClient: jest.fn(() => ({})),
}));

jest.mock('@aws-sdk/lib-dynamodb', () => ({
  DynamoDBDocumentClient: {
    from: jest.fn(() => ({
      send: jest.fn(),
    })),
  },
  UpdateCommand: jest.fn(),
}));

const mockAuth = require('@clerk/nextjs/server').auth as jest.MockedFunction<any>;
const mockSend = require('@aws-sdk/lib-dynamodb').DynamoDBDocumentClient.from().send as jest.MockedFunction<any>;
const mockUpdateCommand = require('@aws-sdk/lib-dynamodb').UpdateCommand as jest.MockedFunction<any>;

// Mock NextRequest
const createMockRequest = (body: any) => ({
  json: jest.fn().mockResolvedValue(body),
  headers: new Map(),
  method: 'PATCH',
  url: 'http://localhost:3009/api/documents/doc_456',
});

describe('PATCH /api/documents/[id]', () => {
  const mockUserId = 'user_123';
  const mockDocId = 'doc_456';

  beforeEach(() => {
    jest.clearAllMocks();
    mockAuth.mockResolvedValue({ userId: mockUserId });
    
    // Set up environment variables
    process.env.DYNAMODB_DOCUMENTS_TABLE_NAME = 'test-documents';
    process.env.AWS_REGION = 'us-east-1';
    process.env.AWS_ACCESS_KEY_ID = 'test-key';
    process.env.AWS_SECRET_ACCESS_KEY = 'test-secret';
  });

  it('updates document metadata successfully', async () => {
    const mockUpdatedDoc = {
      userId: mockUserId,
      docId: mockDocId,
      fileName: 'test.slate',
      fileSize: 1024,
      lastModified: '2023-01-01T00:00:00.000Z',
    };

    mockSend.mockResolvedValueOnce({
      Attributes: mockUpdatedDoc,
    });

    const request = createMockRequest({
      fileName: 'test.slate',
      fileSize: 1024,
      lastModified: '2023-01-01T00:00:00.000Z',
    });

    const response = await PATCH(request as any, {
      params: Promise.resolve({ id: mockDocId }),
    });

    expect(response.status).toBe(200);
    const body = await response.json();
    expect(body.success).toBe(true);
    expect(body.document).toEqual(mockUpdatedDoc);
    expect(mockUpdateCommand).toHaveBeenCalledWith({
      TableName: 'test-documents',
      Key: { userId: mockUserId, docId: mockDocId },
      UpdateExpression: expect.stringContaining('SET'),
      ExpressionAttributeNames: expect.any(Object),
      ExpressionAttributeValues: expect.any(Object),
      ConditionExpression: 'attribute_exists(userId)',
      ReturnValues: 'ALL_NEW',
    });
  });

  it('updates only lastModified when no other fields provided', async () => {
    const mockUpdatedDoc = {
      userId: mockUserId,
      docId: mockDocId,
      lastModified: expect.any(String),
    };

    mockSend.mockResolvedValueOnce({
      Attributes: mockUpdatedDoc,
    });

    const request = createMockRequest({});

    const response = await PATCH(request as any, {
      params: Promise.resolve({ id: mockDocId }),
    });

    expect(response.status).toBe(200);
    expect(mockUpdateCommand).toHaveBeenCalledWith(
      expect.objectContaining({
        UpdateExpression: 'SET #lastModified = :lastModified',
        ExpressionAttributeNames: { '#lastModified': 'lastModified' },
        ExpressionAttributeValues: { ':lastModified': expect.any(String) },
      })
    );
  });

  it('returns 401 for unauthenticated user', async () => {
    mockAuth.mockResolvedValueOnce({ userId: null });

    const request = createMockRequest({ fileName: 'test.slate' });

    const response = await PATCH(request as any, {
      params: Promise.resolve({ id: mockDocId }),
    });

    expect(response.status).toBe(401);
    const body = await response.json();
    expect(body.error).toBe('Unauthorized');
  });

  it('returns 400 for missing document ID', async () => {
    const request = createMockRequest({ fileName: 'test.slate' });

    const response = await PATCH(request as any, {
      params: Promise.resolve({ id: '' }),
    });

    expect(response.status).toBe(400);
    const body = await response.json();
    expect(body.error).toBe('Document ID is required');
  });

  it('returns 404 for non-existent document', async () => {
    mockSend.mockRejectedValueOnce({
      name: 'ConditionalCheckFailedException',
    });

    const request = createMockRequest({ fileName: 'test.slate' });

    const response = await PATCH(request as any, {
      params: Promise.resolve({ id: mockDocId }),
    });

    expect(response.status).toBe(404);
    const body = await response.json();
    expect(body.error).toBe('Document not found or access denied');
  });

  it('returns 500 for DynamoDB errors', async () => {
    mockSend.mockRejectedValueOnce(new Error('DynamoDB error'));

    const request = createMockRequest({ fileName: 'test.slate' });

    const response = await PATCH(request as any, {
      params: Promise.resolve({ id: mockDocId }),
    });

    expect(response.status).toBe(500);
    const body = await response.json();
    expect(body.error).toBe('Failed to update document metadata');
  });

  it('handles fileSize of 0 correctly', async () => {
    mockSend.mockResolvedValueOnce({
      Attributes: { fileSize: 0 },
    });

    const request = createMockRequest({ fileSize: 0 });

    const response = await PATCH(request as any, {
      params: Promise.resolve({ id: mockDocId }),
    });

    expect(response.status).toBe(200);
    expect(mockUpdateCommand).toHaveBeenCalledWith(
      expect.objectContaining({
        ExpressionAttributeValues: expect.objectContaining({
          ':fileSize': 0,
        }),
      })
    );
  });
}); 