import { auth, currentUser } from '@clerk/nextjs/server';
import { redirect } from 'next/navigation';
import Link from 'next/link';
import { dynamoDbDocClient, awsConfig } from '@/lib/aws';
import { UpdateCommand } from '@aws-sdk/lib-dynamodb';
import { logger, COMPONENTS } from '@/lib/logging';

interface DesktopDonePageProps {
  searchParams: Promise<{
    nonce?: string;
  }>;
}

export default async function DesktopDonePage({ searchParams }: DesktopDonePageProps) {
  const { userId } = await auth();
  const { nonce } = await searchParams;

  // If no session, redirect to sign in
  if (!userId) {
    redirect('/');
  }

  const user = await currentUser();

  // If no nonce, show error
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
            <Link
              href="/dashboard"
              className="inline-block bg-blue-600 hover:bg-blue-700 text-white px-6 py-2 rounded-md transition-colors"
            >
              Go to Dashboard
            </Link>
          </div>
        </div>
      </div>
    );
  }

  // Update the nonce status in DynamoDB with user information
  try {
    // Validate nonce format (should be a valid UUID)
    const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;
    if (!uuidRegex.test(nonce)) {
      logger.warn(
        COMPONENTS.DESKTOP_DONE,
        `Invalid nonce format received: ${nonce} for user: ${userId}`
      );
      // Don't redirect, but log the error
    } else {
      logger.info(
        COMPONENTS.DESKTOP_DONE,
        `Updating nonce status to ready for nonce: ${nonce}, user: ${userId}`
      );

      await dynamoDbDocClient.send(new UpdateCommand({
        TableName: awsConfig.desktopLoginTableName,
        Key: { nonce },
        UpdateExpression: 'SET #status = :ready, userId = :userId, #data = :userData, readyAt = :readyAt',
        ExpressionAttributeNames: {
          '#status': 'status',
          '#data': 'data'
        },
        ExpressionAttributeValues: {
          ':ready': 'ready',
          ':userId': userId,
          ':readyAt': Date.now(),
          ':userData': {
            email: user?.emailAddresses[0]?.emailAddress || '',
            firstName: user?.firstName || '',
            lastName: user?.lastName || '',
          }
        }
      }));

      logger.info(
        COMPONENTS.DESKTOP_DONE,
        `Successfully updated nonce status to ready for nonce: ${nonce}, user: ${userId}`
      );
    }
  } catch (error) {
    logger.error(
      COMPONENTS.DESKTOP_DONE,
      `Failed to update nonce status in DynamoDB for nonce: ${nonce}, user: ${userId}`,
      error
    );
  }

  return (
    <div className="min-h-screen bg-gray-50 flex items-center justify-center">
      <div className="max-w-md mx-auto bg-white rounded-lg shadow-md p-6">
        <div className="text-center">
          <div className="text-4xl mb-4">✅</div>
          <h1 className="text-xl font-bold text-gray-900 mb-2">Authentication Successful!</h1>
          <p className="text-gray-600 mb-4">
            Welcome, {user?.emailAddresses[0]?.emailAddress || user?.firstName || 'User'}
          </p>
          
          <div className="bg-green-50 border border-green-200 rounded-lg p-4 mb-6">
            <p className="text-green-700 text-sm">
              You can now return to LibreOffice. Your desktop application should automatically 
              receive the authentication token and you&rsquo;ll be able to save documents to the cloud.
            </p>
          </div>

          <div className="space-y-3">
            <Link
              href="/dashboard"
              className="block w-full bg-blue-600 hover:bg-blue-700 text-white px-6 py-2 rounded-md transition-colors"
            >
              View My Dashboard
            </Link>
            
            <p className="text-xs text-gray-500">
              Authentication nonce: <code className="bg-gray-100 px-1 rounded">{nonce}</code>
            </p>
          </div>
        </div>
      </div>
    </div>
  );
}

// Note: In the actual implementation (Phase 4), this page will:
// 1. Update the DynamoDB desktop_login table with the user ID
// 2. Set the status to "ready" so the desktop can retrieve the token 