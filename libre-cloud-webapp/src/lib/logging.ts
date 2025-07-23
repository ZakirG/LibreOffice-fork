// Comprehensive logging utility for desktop authentication flow

export enum LogLevel {
  DEBUG = 0,
  INFO = 1,
  WARN = 2,
  ERROR = 3,
}

interface LogEntry {
  timestamp: string;
  level: LogLevel;
  component: string;
  message: string;
  data?: any;
  requestId?: string;
  userAgent?: string;
  ip?: string;
}

class Logger {
  private currentLevel: LogLevel;

  constructor() {
    const envLevel = process.env.LOG_LEVEL?.toUpperCase();
    this.currentLevel = LogLevel[envLevel as keyof typeof LogLevel] ?? LogLevel.INFO;
  }

  private log(level: LogLevel, component: string, message: string, data?: any, meta?: Partial<LogEntry>) {
    if (level < this.currentLevel) {
      return;
    }

    const entry: LogEntry = {
      timestamp: new Date().toISOString(),
      level,
      component,
      message,
      data,
      ...meta,
    };

    const levelName = LogLevel[level];
    const formattedMessage = `[${entry.timestamp}] ${levelName} [${component}] ${message}`;
    
    if (data || meta) {
      const additionalInfo = { 
        ...(data && { data }), 
        ...(meta && { meta: { requestId: meta.requestId, userAgent: meta.userAgent, ip: meta.ip } })
      };
      console.log(formattedMessage, additionalInfo);
    } else {
      console.log(formattedMessage);
    }

    // In production, you might want to send to external logging service
    if (level >= LogLevel.ERROR && process.env.NODE_ENV === 'production') {
      // TODO: Send to external logging service (e.g., CloudWatch, Sentry, etc.)
    }
  }

  debug(component: string, message: string, data?: any, meta?: Partial<LogEntry>) {
    this.log(LogLevel.DEBUG, component, message, data, meta);
  }

  info(component: string, message: string, data?: any, meta?: Partial<LogEntry>) {
    this.log(LogLevel.INFO, component, message, data, meta);
  }

  warn(component: string, message: string, data?: any, meta?: Partial<LogEntry>) {
    this.log(LogLevel.WARN, component, message, data, meta);
  }

  error(component: string, message: string, error?: any, meta?: Partial<LogEntry>) {
    let errorData = error;
    
    if (error instanceof Error) {
      errorData = {
        name: error.name,
        message: error.message,
        stack: error.stack,
      };
    }
    
    this.log(LogLevel.ERROR, component, message, errorData, meta);
  }
}

// Create singleton logger instance
export const logger = new Logger();

// Helper function to create request metadata from Next.js request
export function createRequestMeta(request: Request): Partial<LogEntry> {
  return {
    requestId: crypto.randomUUID(),
    userAgent: request.headers.get('user-agent') || undefined,
    ip: getClientIPFromRequest(request),
  };
}

function getClientIPFromRequest(request: Request): string {
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
  
  return 'localhost';
}

// Predefined component names for consistency
export const COMPONENTS = {
  DESKTOP_INIT: 'desktop-init-api',
  DESKTOP_TOKEN: 'desktop-token-api',
  DESKTOP_LOGIN: 'desktop-login-page',
  DESKTOP_DONE: 'desktop-done-page',
  RATE_LIMITING: 'rate-limiting',
  JWT: 'jwt-utils',
  DYNAMODB: 'dynamodb',
} as const; 