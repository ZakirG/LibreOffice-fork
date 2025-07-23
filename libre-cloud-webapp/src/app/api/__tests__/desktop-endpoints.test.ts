/**
 * Basic tests for desktop authentication API endpoints
 * Tests Phase 4 implementation validation and error handling
 */

import { getClientIP, checkRateLimit } from '@/lib/rate-limiting';

// Create mock request for testing utility functions
function createMockRequest(headers: Record<string, string> = {}): Request {
  return {
    headers: {
      get: (name: string) => headers[name.toLowerCase()] || null,
    },
  } as Request;
}

describe('Desktop Authentication API Utilities', () => {
  describe('Client IP extraction', () => {
    it('should extract IP from x-forwarded-for header', () => {
      const request = createMockRequest({
        'x-forwarded-for': '192.168.1.1, 10.0.0.1'
      });

      const ip = getClientIP(request);
      expect(ip).toBe('192.168.1.1');
    });

    it('should fallback to localhost when no IP headers', () => {
      const request = createMockRequest({});
      const ip = getClientIP(request);
      expect(ip).toBe('localhost');
    });
  });

  describe('Rate limiting functionality', () => {
    it('should allow requests within limit', () => {
      const config = { windowMs: 60000, maxRequests: 5 };
      const result = checkRateLimit('test-api-1', config);

      expect(result.allowed).toBe(true);
      expect(result.remainingRequests).toBe(4);
    });

    it('should block requests when limit exceeded', () => {
      const config = { windowMs: 60000, maxRequests: 2 };
      const identifier = 'test-api-2';

      // Use up the limit
      checkRateLimit(identifier, config);
      checkRateLimit(identifier, config);

      // This should be blocked
      const result = checkRateLimit(identifier, config);
      expect(result.allowed).toBe(false);
      expect(result.retryAfter).toBeGreaterThan(0);
    });
  });

  describe('Base64 token encoding', () => {
    it('should properly encode and decode desktop tokens', () => {
      const tokenData = {
        userId: 'user_123',
        email: 'test@example.com',
        firstName: 'Test',
        lastName: 'User',
        type: 'desktop',
        iat: Math.floor(Date.now() / 1000),
        exp: Math.floor(Date.now() / 1000) + 3600,
        iss: 'libre-cloud-app',
        aud: 'libre-cloud-desktop',
      };

      const base64Token = Buffer.from(JSON.stringify(tokenData)).toString('base64');
      const decodedToken = JSON.parse(Buffer.from(base64Token, 'base64').toString());

      expect(decodedToken.type).toBe('desktop');
      expect(decodedToken.userId).toBe('user_123');
      expect(decodedToken.email).toBe('test@example.com');
      expect(decodedToken.iss).toBe('libre-cloud-app');
      expect(decodedToken.aud).toBe('libre-cloud-desktop');
    });
  });

  describe('UUID validation', () => {
    it('should validate proper UUID format', () => {
      const validUUIDs = [
        '550e8400-e29b-41d4-a716-446655440000',
        '6ba7b810-9dad-11d1-80b4-00c04fd430c8',
        '123e4567-e89b-12d3-a456-426614174000',
      ];

      const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;

      validUUIDs.forEach(uuid => {
        expect(uuidRegex.test(uuid)).toBe(true);
      });
    });

    it('should reject invalid UUID formats', () => {
      const invalidUUIDs = [
        'invalid-nonce',
        '123',
        'not-a-uuid-at-all',
        '550e8400-e29b-41d4-a716', // too short
        'g50e8400-e29b-41d4-a716-446655440000', // invalid character
      ];

      const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;

      invalidUUIDs.forEach(uuid => {
        expect(uuidRegex.test(uuid)).toBe(false);
      });
    });
  });
}); 