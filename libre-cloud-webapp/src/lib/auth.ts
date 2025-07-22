import { NextAuthOptions } from 'next-auth';
import CognitoProvider from 'next-auth/providers/cognito';
import { awsConfig } from './aws';

export const authOptions: NextAuthOptions = {
  providers: [
    CognitoProvider({
      clientId: awsConfig.cognitoClientId,
      clientSecret: '', // Not using client secret for PKCE flow
      issuer: `https://cognito-idp.${awsConfig.region}.amazonaws.com/${awsConfig.cognitoUserPoolId}`,
      authorization: {
        params: {
          scope: 'openid email profile',
          response_type: 'code',
        },
      },
    }),
  ],
  callbacks: {
    async jwt({ token, account, profile }) {
      if (account && profile) {
        token.accessToken = account.access_token;
        token.refreshToken = account.refresh_token;
        token.sub = profile.sub;
        token.email = profile.email;
      }
      return token;
    },
    async session({ session, token }) {
      if (token) {
        session.user.id = token.sub as string;
        session.user.email = token.email as string;
        session.accessToken = token.accessToken as string;
      }
      return session;
    },
  },
  session: {
    strategy: 'jwt',
    maxAge: 30 * 24 * 60 * 60, // 30 days
  },
  pages: {
    signIn: '/auth/signin',
    error: '/auth/error',
  },
  debug: process.env.NODE_ENV === 'development',
};

declare module 'next-auth' {
  interface Session {
    accessToken?: string;
    user: {
      id: string;
      email: string;
      name?: string | null;
      image?: string | null;
    };
  }
}

declare module 'next-auth/jwt' {
  interface JWT {
    accessToken?: string;
    refreshToken?: string;
  }
} 