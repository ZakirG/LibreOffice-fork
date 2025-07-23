// Rate limiting utility for API endpoints
// Simple in-memory rate limiting - in production, use Redis or similar

interface RateLimitInfo {
  count: number;
  resetTime: number;
}

const rateLimitStore = new Map<string, RateLimitInfo>();

export interface RateLimitConfig {
  windowMs: number; // Time window in milliseconds
  maxRequests: number; // Maximum requests per window
}

export interface RateLimitResult {
  allowed: boolean;
  remainingRequests: number;
  resetTime: number;
  retryAfter?: number;
}

export function checkRateLimit(
  identifier: string,
  config: RateLimitConfig
): RateLimitResult {
  const now = Date.now();
  const windowStart = now - config.windowMs;
  
  // Clean up expired entries
  for (const [key, info] of rateLimitStore.entries()) {
    if (info.resetTime < now) {
      rateLimitStore.delete(key);
    }
  }
  
  const current = rateLimitStore.get(identifier);
  
  if (!current || current.resetTime < now) {
    // New window or expired entry
    const newInfo: RateLimitInfo = {
      count: 1,
      resetTime: now + config.windowMs,
    };
    rateLimitStore.set(identifier, newInfo);
    
    return {
      allowed: true,
      remainingRequests: config.maxRequests - 1,
      resetTime: newInfo.resetTime,
    };
  }
  
  if (current.count >= config.maxRequests) {
    // Rate limit exceeded
    return {
      allowed: false,
      remainingRequests: 0,
      resetTime: current.resetTime,
      retryAfter: Math.ceil((current.resetTime - now) / 1000),
    };
  }
  
  // Increment count
  current.count++;
  rateLimitStore.set(identifier, current);
  
  return {
    allowed: true,
    remainingRequests: config.maxRequests - current.count,
    resetTime: current.resetTime,
  };
}

// Get client IP for rate limiting
export function getClientIP(request: Request): string {
  const forwarded = request.headers.get('x-forwarded-for');
  const realIP = request.headers.get('x-real-ip');
  const cfConnectingIP = request.headers.get('cf-connecting-ip');
  
  if (forwarded) {
    return forwarded.split(',')[0].trim();
  }
  
  if (realIP) {
    return realIP;
  }
  
  if (cfConnectingIP) {
    return cfConnectingIP;
  }
  
  // Fallback for development
  return 'localhost';
}

// Rate limit configurations
export const RATE_LIMITS = {
  DESKTOP_INIT: {
    windowMs: 15 * 60 * 1000, // 15 minutes
    maxRequests: 10, // 10 requests per 15 minutes per IP
  },
  DESKTOP_TOKEN: {
    windowMs: 60 * 1000, // 1 minute
    maxRequests: 60, // 60 requests per minute per IP (for polling)
  },
} as const; 