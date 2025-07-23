import { auth, currentUser } from '@clerk/nextjs/server';
import { redirect } from 'next/navigation';
import Link from 'next/link';
import DocumentEditor from '@/components/editor/DocumentEditor';

interface DocumentPageProps {
  params: Promise<{
    id: string;
  }>;
}

export default async function DocumentPage({ params }: DocumentPageProps) {
  const { userId } = await auth();

  if (!userId) {
    redirect('/');
  }

  const user = await currentUser();
  const { id } = await params;

  return (
    <div className="min-h-screen bg-gray-50">
      {/* Header */}
      <header className="bg-white shadow-sm border-b">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex justify-between items-center py-4">
            <div className="flex items-center space-x-4">
              <Link
                href="/dashboard"
                className="text-blue-600 hover:text-blue-800 font-medium"
              >
                ← Back to Dashboard
              </Link>
              <h1 className="text-xl font-bold text-gray-900">
                Document: {id}
              </h1>
            </div>
            
            <div className="flex items-center space-x-4">
              <span className="text-sm text-gray-600">
                {user?.emailAddresses[0]?.emailAddress || user?.firstName || 'User'}
              </span>
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        <DocumentEditor documentId={id} />
      </main>
    </div>
  );
} 