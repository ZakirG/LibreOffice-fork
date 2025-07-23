import { auth } from '@clerk/nextjs/server';
import { redirect } from 'next/navigation';
import { SignInButton } from '@clerk/nextjs';
import Link from 'next/link';
import { logger, COMPONENTS } from '@/lib/logging';

interface DesktopLoginPageProps {
  searchParams: Promise<{
    nonce?: string;
  }>;
}

export default async function DesktopLoginPage({ searchParams }: DesktopLoginPageProps) {
  const { userId } = await auth();
  const { nonce } = await searchParams;

  logger.info(
    COMPONENTS.DESKTOP_LOGIN,
    `Desktop login page accessed with nonce: ${nonce || 'none'}, userId: ${userId || 'none'}`
  );

  // If no nonce provided, show error
  if (!nonce) {
    logger.warn(
      COMPONENTS.DESKTOP_LOGIN,
      'Desktop login page accessed without nonce parameter'
    );
    return (
      <div className="min-h-screen bg-gray-50 flex items-center justify-center">
        <div className="max-w-md mx-auto bg-white rounded-lg shadow-md p-6">
          <div className="text-center">
            <div className="text-4xl mb-4">❌</div>
            <h1 className="text-xl font-bold text-gray-900 mb-2">Invalid Request</h1>
            <p className="text-gray-600 mb-6">
              No authentication nonce provided. Please try again from LibreOffice.
            </p>
            <Link
              href="/"
              className="inline-block bg-blue-600 hover:bg-blue-700 text-white px-6 py-2 rounded-md transition-colors"
            >
              Go to Home
            </Link>
          </div>
        </div>
      </div>
    );
  }

  // Validate nonce format (should be a valid UUID)
  const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;
  if (!uuidRegex.test(nonce)) {
    logger.warn(
      COMPONENTS.DESKTOP_LOGIN,
      `Invalid nonce format received: ${nonce}`
    );

    return (
      <div className="min-h-screen bg-gray-50 flex items-center justify-center">
        <div className="max-w-md mx-auto bg-white rounded-lg shadow-md p-6">
          <div className="text-center">
            <div className="text-4xl mb-4">❌</div>
            <h1 className="text-xl font-bold text-gray-900 mb-2">Invalid Authentication Request</h1>
            <p className="text-gray-600 mb-6">
              The authentication nonce format is invalid. Please try again from LibreOffice.
            </p>
            <Link
              href="/"
              className="inline-block bg-blue-600 hover:bg-blue-700 text-white px-6 py-2 rounded-md transition-colors"
            >
              Go to Home
            </Link>
          </div>
        </div>
      </div>
    );
  }

  // If already authenticated, redirect to completion page
  if (userId) {
    logger.info(
      COMPONENTS.DESKTOP_LOGIN,
      `User already authenticated, redirecting to completion page for nonce: ${nonce}`
    );
    
    redirect(`/desktop-done?nonce=${nonce}`);
  }

  // Show sign-in page with nonce preserved
  return (
    <div className="min-h-screen bg-gray-50 flex items-center justify-center">
      <div className="max-w-md mx-auto bg-white rounded-lg shadow-md p-6">
        <div className="text-center">
          <div className="text-4xl mb-4">🔐</div>
          <h1 className="text-xl font-bold text-gray-900 mb-2">Desktop Authentication</h1>
          <p className="text-gray-600 mb-6">
            Please sign in to authenticate your LibreOffice desktop application.
          </p>
          
          <SignInButton 
            mode="modal"
            forceRedirectUrl={`/desktop-done?nonce=${nonce}`}
          >
            <button className="w-full bg-blue-600 hover:bg-blue-700 text-white px-6 py-2 rounded-md transition-colors">
              Sign In
            </button>
          </SignInButton>
          
          <p className="text-xs text-gray-500 mt-4">
            Authentication nonce: <code className="bg-gray-100 px-1 rounded">{nonce}</code>
          </p>
        </div>
      </div>
    </div>
  );
}

// This page will handle the nonce storage in the actual implementation
// For now, it's a simple redirect page 