import { getServerSession } from 'next-auth/next';
import { authOptions } from '@/lib/auth';
import { redirect } from 'next/navigation';

interface DesktopLoginPageProps {
  searchParams: {
    nonce?: string;
  };
}

export default async function DesktopLoginPage({ searchParams }: DesktopLoginPageProps) {
  const session = await getServerSession(authOptions);
  const { nonce } = searchParams;

  // If no nonce provided, show error
  if (!nonce) {
    return (
      <div className="min-h-screen bg-gray-50 flex items-center justify-center">
        <div className="max-w-md mx-auto bg-white rounded-lg shadow-md p-6">
          <div className="text-center">
            <div className="text-4xl mb-4">❌</div>
            <h1 className="text-xl font-bold text-gray-900 mb-2">Invalid Request</h1>
            <p className="text-gray-600 mb-6">
              No authentication nonce provided. Please try again from LibreOffice.
            </p>
            <a
              href="/"
              className="inline-block bg-blue-600 hover:bg-blue-700 text-white px-6 py-2 rounded-md transition-colors"
            >
              Go to Home
            </a>
          </div>
        </div>
      </div>
    );
  }

  // If already authenticated, redirect to completion page
  if (session) {
    redirect(`/desktop-done?nonce=${nonce}`);
  }

  // Redirect to sign in with the nonce preserved
  redirect(`/api/auth/signin?callbackUrl=${encodeURIComponent(`/desktop-done?nonce=${nonce}`)}`);
}

// This page will handle the nonce storage in the actual implementation
// For now, it's a simple redirect page 