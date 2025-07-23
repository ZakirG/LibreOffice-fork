import { NextRequest } from 'next/server';
import { verifyToken } from '@clerk/backend';
import { validateDesktopJWT, type DesktopJWTPayload } from './jwt';

export interface AuthPayload {
  userId: string;
  email?: string;
  type: 'web' | 'desktop';
}

export async function validateAuth(request: NextRequest): Promise<AuthPayload | null> {
  console.log('[BACKEND DEBUG] validateAuth called');
  const authHeader = request.headers.get('authorization');
  console.log('[BACKEND DEBUG] Auth header:', authHeader ? `Bearer ${authHeader.substring(7, 57)}...` : 'MISSING');
  
  // First try desktop JWT validation (uses its own token extraction)
  console.log('[BACKEND DEBUG] Trying desktop JWT validation...');
  const desktopPayload = validateDesktopJWT(request);
  if (desktopPayload) {
    console.log('[BACKEND DEBUG] Desktop JWT validation SUCCESS, userId:', desktopPayload.userId);
    return {
      userId: desktopPayload.userId,
      email: desktopPayload.email,
      type: 'desktop'
    };
  }
  
  console.log('[BACKEND DEBUG] Desktop JWT validation failed, trying Clerk...');
  
  // Then try Clerk web session token validation
  if (!authHeader || !authHeader.startsWith('Bearer ')) {
    console.log('[BACKEND DEBUG] No valid auth header for Clerk validation');
    return null;
  }
  
  const token = authHeader.substring(7);
  
  try {
    console.log('[BACKEND DEBUG] Attempting Clerk token verification...');
    const clerkPayload = await verifyToken(token, {
      secretKey: process.env.CLERK_SECRET_KEY!,
    });
    
    if (clerkPayload && clerkPayload.sub) {
      console.log('[BACKEND DEBUG] Clerk validation SUCCESS, userId:', clerkPayload.sub);
      return {
        userId: clerkPayload.sub,
        email: typeof clerkPayload.email === 'string' ? clerkPayload.email : undefined,
        type: 'web'
      };
    }
  } catch (error) {
    console.log('[BACKEND DEBUG] Clerk token validation failed:', error instanceof Error ? error.message : String(error));
    // Clerk token validation failed, continue to return null
  }
  
  console.log('[BACKEND DEBUG] All authentication methods failed');
  return null;
}

// Middleware function for API routes that accepts both auth types
export function withAuth<T extends any[]>(
  handler: (payload: AuthPayload, ...args: T) => Promise<Response>
) {
  return async (request: NextRequest, ...args: T): Promise<Response> => {
    const payload = await validateAuth(request);
    
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