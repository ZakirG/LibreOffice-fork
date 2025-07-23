import jwt from 'jsonwebtoken';
import { NextRequest } from 'next/server';

const JWT_SECRET = process.env.NEXTAUTH_SECRET || 'fallback-secret-for-development';

export interface DesktopJWTPayload {
  userId: string;
  email: string;
  firstName?: string;
  lastName?: string;
  iat: number;
  exp: number;
  type: 'desktop';
  iss?: string;
  aud?: string;
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
    // First, try to decode as base64 (Phase 4 desktop token format)
    try {
      const base64Decoded = Buffer.from(token, 'base64').toString('utf-8');
      const tokenData = JSON.parse(base64Decoded);
      
      // Validate it's a desktop token with required fields
      if (tokenData.type === 'desktop' && 
          tokenData.userId && 
          tokenData.email && 
          tokenData.exp && 
          tokenData.iat) {
        
        // Check if token is expired
        const now = Math.floor(Date.now() / 1000);
        if (tokenData.exp < now) {
          console.log('Desktop token expired');
          return null;
        }
        
        // Check issuer and audience if present
        if (tokenData.iss && tokenData.iss !== 'libre-cloud-app') {
          console.log('Invalid token issuer');
          return null;
        }
        
        if (tokenData.aud && tokenData.aud !== 'libre-cloud-desktop') {
          console.log('Invalid token audience');
          return null;
        }
        
        console.log('Base64 desktop token validated successfully for user:', tokenData.userId);
        return tokenData as DesktopJWTPayload;
      }
    } catch (base64Error) {
      // Not a valid base64 token, try JWT parsing
    }
    
    // Fallback: try to decode as JWT token (old format)
    const unverifiedDecoded = jwt.decode(token);
    
    // If it's not an object or doesn't have our desktop token structure, skip validation
    if (!unverifiedDecoded || typeof unverifiedDecoded !== 'object' || unverifiedDecoded.type !== 'desktop') {
      return null;
    }
    
    // Now verify the JWT token since it looks like ours
    const decoded = jwt.verify(token, JWT_SECRET, {
      issuer: 'libre-cloud-app',
      audience: 'libre-cloud-desktop',
    });

    if (typeof decoded === 'object' && decoded.type === 'desktop') {
      console.log('JWT desktop token validated successfully');
      return decoded as DesktopJWTPayload;
    }

    return null;
  } catch (error) {
    console.log('Token validation failed:', error);
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