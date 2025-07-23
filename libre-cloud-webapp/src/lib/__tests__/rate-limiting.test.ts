import { checkRateLimit, getClientIP, RATE_LIMITS } from '../rate-limiting';

// Mock for Request object
class MockRequest {
  public headers: Map<string, string>;

  constructor(headers: Record<string, string> = {}) {
    this.headers = new Map(Object.entries(headers));
  }

  get(name: string): string | null {
    return this.headers.get(name.toLowerCase()) || null;
  }
}

// Type assertion helper for tests
function createMockRequest(headers: Record<string, string> = {}): Request {
  const mockReq = new MockRequest(headers);
  return {
    headers: {
      get: mockReq.get.bind(mockReq)
    }
  } as Request;
}

describe('Rate Limiting', () => {
  beforeEach(() => {
    // Clear the in-memory rate limit store between tests
    // Note: In a real implementation, we'd need a way to clear the store
    // For now, we'll use different identifiers for each test
  });

  describe('checkRateLimit', () => {
    it('should allow first request within limit', () => {
      const config = { windowMs: 60000, maxRequests: 5 };
      const result = checkRateLimit('test-1', config);

      expect(result.allowed).toBe(true);
      expect(result.remainingRequests).toBe(4);
      expect(result.resetTime).toBeGreaterThan(Date.now());
    });

    it('should track multiple requests correctly', () => {
      const config = { windowMs: 60000, maxRequests: 3 };
      const identifier = 'test-2';

      // First request
      const result1 = checkRateLimit(identifier, config);
      expect(result1.allowed).toBe(true);
      expect(result1.remainingRequests).toBe(2);

      // Second request
      const result2 = checkRateLimit(identifier, config);
      expect(result2.allowed).toBe(true);
      expect(result2.remainingRequests).toBe(1);

      // Third request
      const result3 = checkRateLimit(identifier, config);
      expect(result3.allowed).toBe(true);
      expect(result3.remainingRequests).toBe(0);
    });

    it('should block requests when limit exceeded', () => {
      const config = { windowMs: 60000, maxRequests: 2 };
      const identifier = 'test-3';

      // Use up the limit
      checkRateLimit(identifier, config);
      checkRateLimit(identifier, config);

      // This should be blocked
      const result = checkRateLimit(identifier, config);
      expect(result.allowed).toBe(false);
      expect(result.remainingRequests).toBe(0);
      expect(result.retryAfter).toBeGreaterThan(0);
    });

    it('should reset limit after window expires', async () => {
      const config = { windowMs: 100, maxRequests: 1 }; // Very short window
      const identifier = 'test-4';

      // Use up the limit
      const result1 = checkRateLimit(identifier, config);
      expect(result1.allowed).toBe(true);

      // Should be blocked immediately
      const result2 = checkRateLimit(identifier, config);
      expect(result2.allowed).toBe(false);

      // Wait for window to expire
      await new Promise(resolve => setTimeout(resolve, 150));

      // Should be allowed again
      const result3 = checkRateLimit(identifier, config);
      expect(result3.allowed).toBe(true);
      expect(result3.remainingRequests).toBe(0);
    });

    it('should handle different identifiers separately', () => {
      const config = { windowMs: 60000, maxRequests: 1 };

      const result1 = checkRateLimit('user-1', config);
      const result2 = checkRateLimit('user-2', config);

      expect(result1.allowed).toBe(true);
      expect(result2.allowed).toBe(true);
    });
  });

  describe('getClientIP', () => {
    it('should extract IP from x-forwarded-for header', () => {
      const request = createMockRequest({
        'x-forwarded-for': '192.168.1.1, 10.0.0.1'
      });

      const ip = getClientIP(request);
      expect(ip).toBe('192.168.1.1');
    });

    it('should extract IP from x-real-ip header when x-forwarded-for is not present', () => {
      const request = createMockRequest({
        'x-real-ip': '192.168.1.2'
      });

      const ip = getClientIP(request);
      expect(ip).toBe('192.168.1.2');
    });

    it('should extract IP from cf-connecting-ip header when others are not present', () => {
      const request = createMockRequest({
        'cf-connecting-ip': '192.168.1.3'
      });

      const ip = getClientIP(request);
      expect(ip).toBe('192.168.1.3');
    });

    it('should fallback to localhost when no IP headers are present', () => {
      const request = createMockRequest({});

      const ip = getClientIP(request);
      expect(ip).toBe('localhost');
    });

    it('should handle x-forwarded-for with multiple IPs', () => {
      const request = createMockRequest({
        'x-forwarded-for': '  203.0.113.1  ,  198.51.100.1  ,  192.0.2.1  '
      });

      const ip = getClientIP(request);
      expect(ip).toBe('203.0.113.1');
    });
  });

  describe('RATE_LIMITS configuration', () => {
    it('should have proper configuration for DESKTOP_INIT', () => {
      expect(RATE_LIMITS.DESKTOP_INIT.windowMs).toBe(15 * 60 * 1000); // 15 minutes
      expect(RATE_LIMITS.DESKTOP_INIT.maxRequests).toBe(10);
    });

    it('should have proper configuration for DESKTOP_TOKEN', () => {
      expect(RATE_LIMITS.DESKTOP_TOKEN.windowMs).toBe(60 * 1000); // 1 minute
      expect(RATE_LIMITS.DESKTOP_TOKEN.maxRequests).toBe(60);
    });
  });
}); 