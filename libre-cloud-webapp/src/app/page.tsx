import { auth } from '@clerk/nextjs/server';
import { redirect } from 'next/navigation';
import { SignInButton } from '@clerk/nextjs';

export default async function HomePage() {
  const { userId } = await auth();

  // Redirect to dashboard if already authenticated
  if (userId) {
    redirect('/dashboard');
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-indigo-100">
      <div className="container mx-auto px-4 py-16">
        <div className="max-w-4xl mx-auto text-center">
          {/* Header */}
          <div className="mb-12">
            <h1 className="text-5xl font-bold text-gray-900 mb-6">
              LibreCloud
            </h1>
            <p className="text-xl text-gray-600 mb-8">
              The future of document collaboration. LibreOffice meets the cloud.
            </p>
          </div>

          {/* Features */}
          <div className="grid md:grid-cols-3 gap-8 mb-12">
            <div className="bg-white p-6 rounded-lg shadow-md">
              <div className="text-3xl mb-4">☁️</div>
              <h3 className="text-lg font-semibold mb-2">Cloud Storage</h3>
              <p className="text-gray-600">
                Save your documents to the cloud and access them anywhere
              </p>
            </div>

            <div className="bg-white p-6 rounded-lg shadow-md">
              <div className="text-3xl mb-4">🎵</div>
              <h3 className="text-lg font-semibold mb-2">Audio Integration</h3>
              <p className="text-gray-600">
                Perfect for musicians - embed and play audio files in your documents
              </p>
            </div>

            <div className="bg-white p-6 rounded-lg shadow-md">
              <div className="text-3xl mb-4">📱</div>
              <h3 className="text-lg font-semibold mb-2">Mobile Friendly</h3>
              <p className="text-gray-600">
                Access and manage your documents from any device
              </p>
            </div>
          </div>

          {/* CTA */}
          <div className="bg-white p-8 rounded-lg shadow-lg">
            <h2 className="text-2xl font-bold text-gray-900 mb-4">
              Ready to get started?
            </h2>
            <p className="text-gray-600 mb-6">
              Sign in to access your cloud documents and start collaborating
            </p>
            
            <SignInButton mode="modal">
              <button className="inline-block bg-blue-600 hover:bg-blue-700 text-white font-semibold py-3 px-8 rounded-lg transition-colors duration-200">
                Sign In to LibreCloud
              </button>
            </SignInButton>
          </div>

          {/* Footer */}
          <div className="mt-16 text-center text-gray-500">
            <p>Built with LibreOffice, Next.js, and AWS</p>
          </div>
        </div>
      </div>
    </div>
  );
}
