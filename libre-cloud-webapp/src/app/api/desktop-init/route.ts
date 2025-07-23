import { NextRequest, NextResponse } from 'next/server';
import { randomUUID } from 'crypto';
import { dynamoDbDocClient, awsConfig } from '@/lib/aws';
import { PutCommand } from '@aws-sdk/lib-dynamodb';
import { checkRateLimit, getClientIP, RATE_LIMITS } from '@/lib/rate-limiting';
import { logger, createRequestMeta, COMPONENTS } from '@/lib/logging';

// CORS headers for desktop client requests
const CORS_HEADERS = {
  'Access-Control-Allow-Origin': '*',
  'Access-Control-Allow-Methods': 'POST, OPTIONS',
  'Access-Control-Allow-Headers': 'Content-Type, Authorization, User-Agent',
  'Access-Control-Max-Age': '86400',
} as const;

export async function OPTIONS() {
  return new NextResponse(null, {
    status: 200,
    headers: CORS_HEADERS,
  });
}

export async function POST(request: NextRequest) {
  const requestMeta = createRequestMeta(request);
  
  logger.info(
    COMPONENTS.DESKTOP_INIT,
    'Desktop authentication initialization requested',
    undefined,
    requestMeta
  );

  try {
    // Rate limiting
    const clientIP = getClientIP(request);
    const rateLimitResult = checkRateLimit(
      `desktop-init:${clientIP}`,
      RATE_LIMITS.DESKTOP_INIT
    );

    if (!rateLimitResult.allowed) {
      logger.warn(
        COMPONENTS.DESKTOP_INIT,
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
            'X-RateLimit-Limit': RATE_LIMITS.DESKTOP_INIT.maxRequests.toString(),
            'X-RateLimit-Remaining': rateLimitResult.remainingRequests.toString(),
            'X-RateLimit-Reset': new Date(rateLimitResult.resetTime).toISOString(),
            'Retry-After': rateLimitResult.retryAfter?.toString() || '900',
          }
        }
      );
    }

    // Input validation
    const contentType = request.headers.get('content-type');
    if (contentType && !contentType.includes('application/json')) {
      logger.warn(
        COMPONENTS.DESKTOP_INIT,
        'Invalid content type',
        { contentType },
        requestMeta
      );

      return NextResponse.json(
        { error: 'Content-Type must be application/json' },
        { status: 400, headers: CORS_HEADERS }
      );
    }

    // Generate a unique nonce
    const nonce = randomUUID();
    
    // Set expiration to 5 minutes from now
    const expiresAt = Date.now() + (5 * 60 * 1000);
    
    logger.debug(
      COMPONENTS.DESKTOP_INIT,
      'Generated nonce for desktop authentication',
      { nonce, expiresAt: new Date(expiresAt).toISOString() },
      requestMeta
    );

    // Store nonce in DynamoDB with pending status
    const putCommand = new PutCommand({
      TableName: awsConfig.desktopLoginTableName,
      Item: {
        nonce,
        status: 'pending',
        expiresAt,
        createdAt: Date.now(),
        clientIP,
      },
    });

    await dynamoDbDocClient.send(putCommand);

    logger.info(
      COMPONENTS.DESKTOP_INIT,
      'Nonce stored in DynamoDB successfully',
      { nonce },
      requestMeta
    );

    // Create the login URL that the desktop client should open
    const baseUrl = process.env.NEXTAUTH_URL || 'http://localhost:3009';
    const loginUrl = `${baseUrl}/desktop-login?nonce=${nonce}`;

    const response = {
      nonce,
      loginUrl,
      expiresAt,
      message: 'Open the loginUrl in your browser to authenticate'
    };

    logger.info(
      COMPONENTS.DESKTOP_INIT,
      'Desktop authentication initialization successful',
      { nonce, loginUrl },
      requestMeta
    );

    return NextResponse.json(response, {
      status: 200,
      headers: {
        ...CORS_HEADERS,
        'X-RateLimit-Limit': RATE_LIMITS.DESKTOP_INIT.maxRequests.toString(),
        'X-RateLimit-Remaining': rateLimitResult.remainingRequests.toString(),
        'X-RateLimit-Reset': new Date(rateLimitResult.resetTime).toISOString(),
      }
    });

  } catch (error) {
    logger.error(
      COMPONENTS.DESKTOP_INIT,
      'Failed to initialize desktop authentication',
      error,
      requestMeta
    );

    return NextResponse.json(
      { error: 'Internal server error. Please try again later.' },
      { status: 500, headers: CORS_HEADERS }
    );
  }
} 