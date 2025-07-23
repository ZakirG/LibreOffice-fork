import { NextRequest, NextResponse } from 'next/server';
import { dynamoDbDocClient, awsConfig } from '@/lib/aws';
import { GetCommand, UpdateCommand } from '@aws-sdk/lib-dynamodb';
import { checkRateLimit, getClientIP, RATE_LIMITS } from '@/lib/rate-limiting';
import { logger, createRequestMeta, COMPONENTS } from '@/lib/logging';

// CORS headers for desktop client requests
const CORS_HEADERS = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'GET, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type, Authorization, User-Agent',
  'Access-Control-Max-Age': '86400',
} as const;

export async function OPTIONS() {
  return new NextResponse(null, {
    status: 200,
    headers: CORS_HEADERS,
  });
}

export async function GET(request: NextRequest) {
  const requestMeta = createRequestMeta(request);
  
  logger.info(
    COMPONENTS.DESKTOP_TOKEN,
    'Desktop token retrieval requested',
    undefined,
    requestMeta
  );

  try {
    // Rate limiting
    const clientIP = getClientIP(request);
    const rateLimitResult = checkRateLimit(
      `desktop-token:${clientIP}`,
      RATE_LIMITS.DESKTOP_TOKEN
    );

    if (!rateLimitResult.allowed) {
      logger.warn(
        COMPONENTS.DESKTOP_TOKEN,
        'Rate limit exceeded',
        { 
          clientIP, 
          remainingRequests: rateLimitResult.remainingRequests,
          retryAfter: rateLimitResult.retryAfter 
        },
        requestMeta
      );

      return NextResponse.json(
        { 
          error: 'Rate limit exceeded. Please try again later.',
          retryAfter: rateLimitResult.retryAfter 
        },
        { 
          status: 429,
          headers: {
            ...CORS_HEADERS,
            'X-RateLimit-Limit': RATE_LIMITS.DESKTOP_TOKEN.maxRequests.toString(),
            'X-RateLimit-Remaining': rateLimitResult.remainingRequests.toString(),
            'X-RateLimit-Reset': new Date(rateLimitResult.resetTime).toISOString(),
            'Retry-After': rateLimitResult.retryAfter?.toString() || '60',
          }
        }
      );
    }

    const { searchParams } = new URL(request.url);
    const nonce = searchParams.get('nonce');

    // Input validation
    if (!nonce) {
      logger.warn(
        COMPONENTS.DESKTOP_TOKEN,
        'Missing nonce parameter',
        undefined,
        requestMeta
      );

      return NextResponse.json(
        { error: 'Nonce parameter is required' },
        { status: 400, headers: CORS_HEADERS }
      );
    }

    // Validate nonce format (should be a valid UUID)
    const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;
    if (!uuidRegex.test(nonce)) {
      logger.warn(
        COMPONENTS.DESKTOP_TOKEN,
        'Invalid nonce format',
        { nonce },
        requestMeta
      );

      return NextResponse.json(
        { error: 'Invalid nonce format' },
        { status: 400, headers: CORS_HEADERS }
      );
    }

    logger.debug(
      COMPONENTS.DESKTOP_TOKEN,
      'Looking up nonce in DynamoDB',
      { nonce },
      requestMeta
    );

    // Check nonce status in DynamoDB
    const getCommand = new GetCommand({
      TableName: awsConfig.desktopLoginTableName,
      Key: { nonce },
    });

    const result = await dynamoDbDocClient.send(getCommand);

    if (!result.Item) {
      logger.warn(
        COMPONENTS.DESKTOP_TOKEN,
        'Nonce not found in database',
        { nonce },
        requestMeta
      );

      return NextResponse.json(
        { error: 'Invalid or expired nonce' },
        { status: 404, headers: CORS_HEADERS }
      );
    }

    const { status, userId, expiresAt, data: userData } = result.Item;

    logger.debug(
      COMPONENTS.DESKTOP_TOKEN,
      'Nonce found in database',
      { nonce, status, expiresAt: new Date(expiresAt).toISOString() },
      requestMeta
    );

    // Check if nonce has expired
    if (Date.now() > expiresAt) {
      logger.warn(
        COMPONENTS.DESKTOP_TOKEN,
        'Nonce has expired',
        { nonce, expiresAt: new Date(expiresAt).toISOString() },
        requestMeta
      );

      return NextResponse.json(
        { error: 'Nonce has expired' },
        { status: 410, headers: CORS_HEADERS }
      );
    }

    // If status is still pending, return waiting response
    if (status === 'pending') {
      logger.debug(
        COMPONENTS.DESKTOP_TOKEN,
        'Authentication still pending',
        { nonce },
        requestMeta
      );

      return NextResponse.json(
        { status: 'pending', message: 'Authentication still pending' },
        { 
          status: 202,
          headers: {
            ...CORS_HEADERS,
            'X-RateLimit-Limit': RATE_LIMITS.DESKTOP_TOKEN.maxRequests.toString(),
            'X-RateLimit-Remaining': rateLimitResult.remainingRequests.toString(),
            'X-RateLimit-Reset': new Date(rateLimitResult.resetTime).toISOString(),
          }
        }
      );
    }

    // If status is ready, generate and return base64 token
    if (status === 'ready' && userId) {
      try {
        // Create a comprehensive token with user data stored during authentication
        const tokenData = {
          userId: userId,
          email: userData?.email || '',
          firstName: userData?.firstName || '',
          lastName: userData?.lastName || '',
          type: 'desktop',
          iat: Math.floor(Date.now() / 1000),
          exp: Math.floor(Date.now() / 1000) + 3600, // 1 hour
          iss: 'libre-cloud-app',
          aud: 'libre-cloud-desktop',
        };

        // Create base64 encoded token as specified in Phase 4
        const base64Token = Buffer.from(JSON.stringify(tokenData)).toString('base64');

        logger.info(
          COMPONENTS.DESKTOP_TOKEN,
          'Token generated successfully',
          { nonce, userId },
          requestMeta
        );

        // Mark nonce as consumed
        await dynamoDbDocClient.send(new UpdateCommand({
          TableName: awsConfig.desktopLoginTableName,
          Key: { nonce },
          UpdateExpression: 'SET #status = :consumed, consumedAt = :consumedAt',
          ExpressionAttributeNames: {
            '#status': 'status'
          },
          ExpressionAttributeValues: {
            ':consumed': 'consumed',
            ':consumedAt': Date.now()
          }
        }));

        logger.info(
          COMPONENTS.DESKTOP_TOKEN,
          'Nonce marked as consumed',
          { nonce },
          requestMeta
        );

        const response = {
          token: base64Token,
          expiresAt: tokenData.exp,
          user: {
            id: userId,
            email: tokenData.email,
            firstName: tokenData.firstName,
            lastName: tokenData.lastName,
          }
        };

        return NextResponse.json(response, {
          status: 200,
          headers: {
            ...CORS_HEADERS,
            'X-RateLimit-Limit': RATE_LIMITS.DESKTOP_TOKEN.maxRequests.toString(),
            'X-RateLimit-Remaining': rateLimitResult.remainingRequests.toString(),
            'X-RateLimit-Reset': new Date(rateLimitResult.resetTime).toISOString(),
          }
        });

      } catch (tokenError) {
        logger.error(
          COMPONENTS.DESKTOP_TOKEN,
          'Failed to generate token',
          tokenError,
          requestMeta
        );

        return NextResponse.json(
          { error: 'Failed to generate authentication token' },
          { status: 500, headers: CORS_HEADERS }
        );
      }
    }

    logger.warn(
      COMPONENTS.DESKTOP_TOKEN,
      'Invalid nonce status',
      { nonce, status },
      requestMeta
    );

    return NextResponse.json(
      { error: 'Invalid nonce status' },
      { status: 400, headers: CORS_HEADERS }
    );

  } catch (error) {
    logger.error(
      COMPONENTS.DESKTOP_TOKEN,
      'Failed to retrieve desktop token',
      error,
      requestMeta
    );

    return NextResponse.json(
      { error: 'Internal server error. Please try again later.' },
      { status: 500, headers: CORS_HEADERS }
    );
  }
} 