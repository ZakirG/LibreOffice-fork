import jwt from 'jsonwebtoken';
import { NextRequest } from 'next/server';

const JWT_SECRET = process.env.NEXTAUTH_SECRET || 'fallback-secret-for-development';

export interface DesktopJWTPayload {
  userId: string;
  email: string;
  iat: number;
  exp: number;
  type: 'desktop';
}

export function generateDesktopJWT(userId: string, email: string): string {
  const payload: Omit<DesktopJWTPayload, 'iat' | 'exp'> = {
    userId,
    email,
    type: 'desktop',
  };

  return jwt.sign(payload, JWT_SECRET, {
    expiresIn: '1h', // 1 hour expiration
    issuer: 'libre-cloud-app',
    audience: 'libre-cloud-desktop',
  });
}

export function verifyDesktopJWT(token: string): DesktopJWTPayload | null {
  try {
    // First, try to decode without verification to check if it's our desktop token format
    const unverifiedDecoded = jwt.decode(token);
    
    // If it's not an object or doesn't have our desktop token structure, skip validation
    if (!unverifiedDecoded || typeof unverifiedDecoded !== 'object' || unverifiedDecoded.type !== 'desktop') {
      return null;
    }
    
    // Now verify the token since it looks like ours
    const decoded = jwt.verify(token, JWT_SECRET, {
      issuer: 'libre-cloud-app',
      audience: 'libre-cloud-desktop',
    });

    if (typeof decoded === 'object' && decoded.type === 'desktop') {
      return decoded as DesktopJWTPayload;
    }

    return null;
  } catch (error) {
    // Silently fail for JWT verification errors to allow fallback to Clerk
    return null;
  }
}

export function extractBearerToken(authHeader: string | null): string | null {
  if (!authHeader || !authHeader.startsWith('Bearer ')) {
    return null;
  }
  return authHeader.substring(7);
}

export function extractTokenFromRequest(request: NextRequest): string | null {
  const authHeader = request.headers.get('authorization');
  return extractBearerToken(authHeader);
}

export function validateDesktopJWT(request: NextRequest): DesktopJWTPayload | null {
  const token = extractTokenFromRequest(request);
  if (!token) return null;

  return verifyDesktopJWT(token);
}

// Middleware function for API routes
export function withDesktopAuth<T extends any[]>(
  handler: (payload: DesktopJWTPayload, ...args: T) => Promise<Response>
) {
  return async (request: NextRequest, ...args: T): Promise<Response> => {
    const payload = validateDesktopJWT(request);
    
    if (!payload) {
      return new Response(
        JSON.stringify({ error: 'Invalid or missing authorization token' }),
        {
          status: 401,
          headers: { 'Content-Type': 'application/json' },
        }
      );
    }

    return handler(payload, ...args);
  };
} 