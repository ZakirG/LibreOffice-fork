import { NextRequest } from 'next/server';
import { verifyToken } from '@clerk/backend';
import { validateDesktopJWT, type DesktopJWTPayload } from './jwt';

export interface AuthPayload {
  userId: string;
  email?: string;
  type: 'web' | 'desktop';
}

export async function validateAuth(request: NextRequest): Promise<AuthPayload | null> {
  // First try desktop JWT validation (uses its own token extraction)
  const desktopPayload = validateDesktopJWT(request);
  if (desktopPayload) {
    return {
      userId: desktopPayload.userId,
      email: desktopPayload.email,
      type: 'desktop'
    };
  }
  
  // Then try Clerk web session token validation
  const authHeader = request.headers.get('authorization');
  if (!authHeader || !authHeader.startsWith('Bearer ')) {
    return null;
  }
  
  const token = authHeader.substring(7);
  
  try {
    const clerkPayload = await verifyToken(token, {
      secretKey: process.env.CLERK_SECRET_KEY!,
    });
    
    if (clerkPayload && clerkPayload.sub) {
      return {
        userId: clerkPayload.sub,
        email: typeof clerkPayload.email === 'string' ? clerkPayload.email : undefined,
        type: 'web'
      };
    }
  } catch (error) {
    // Clerk token validation failed, continue to return null
  }
  
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