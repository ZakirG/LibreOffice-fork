# Implementation Phase 1: Authentication and Save to Cloud

This document contains step-by-step prompts for implementing features 1 and 2 from the PRD: Modern authentication and Save to Cloud functionality.

```
✅ PHASE 1: AWS Infrastructure Setup with OpenTofu

Human Actions Required:
- Create AWS account and configure AWS CLI with credentials. Done, we have AWS_ACCESS_KEY and AWS_SECRET_ACCESS_KEY in .env
- Install OpenTofu (https://opentofu.org/docs/intro/install/). Done
- Create a new directory called "libre-cloud-infra" outside the LibreOffice core directory. Done
- Set environment variables: export AWS_REGION=us-east-1 (or your preferred region). Done, see .env file for AWS_REGION

Prompt:
Create an OpenTofu infrastructure project in the current directory with the following structure:
- /modules/cognito-user-pool: Cognito User Pool with hosted UI, PKCE enabled, callback URLs for localhost:3000 and production Vercel domain
- /modules/s3-bucket: Private S3 bucket named "libre-docs-${random_suffix}" with versioning enabled, all public access blocked
- /modules/dynamodb: Two DynamoDB tables - "documents" (PK: userId, SK: docId) and "desktop_login" (PK: nonce) with TTL on expiresAt
- /modules/iam: IAM role with minimal S3 and DynamoDB permissions using least privilege principle
- /modules/ssm-params: SSM Parameter Store entries for all config values
- /environments/dev: Development environment configuration
- /environments/prod: Production environment configuration
- Configure remote state backend using S3 + DynamoDB state locking
- Include proper outputs for all resource IDs and ARNs that the Next.js app will need
- Add validation for required variables and proper resource dependencies

Unit Test:
Run "tofu plan" in both dev and prod environments - should show valid plan with no errors

Human Test:
Run "tofu apply" in dev environment and verify in AWS console:
- Cognito User Pool exists with hosted UI configured. Yes, i see libre-cloud-dev in AWS console.
- S3 bucket exists and is properly secured (no public access). Yes, i see libre-cloud-docs-dev-2aycu2ho
- DynamoDB tables exist with correct schema
- IAM roles have appropriate permissions
- SSM parameters are populated

---

✅ PHASE 2: Next.js Project Setup

Human Actions Required:
- Create Vercel account and install Vercel CLI
- Get the SSM parameter values from AWS and prepare them as environment variables
- Install Node.js 18+ and npm

Prompt:
Create a new Next.js 14 project with App Router in a directory called "libre-cloud-webapp" with this structure:
- Use TypeScript, ESLint, Tailwind CSS
- Install dependencies: next-auth, @aws-sdk/client-cognito-identity-provider, @aws-sdk/client-s3, @aws-sdk/client-dynamodb, @aws-sdk/lib-dynamodb, @aws-sdk/s3-request-presigner, bcryptjs, jsonwebtoken, uuid
- Configure app directory structure:
  - /app/page.tsx: Landing page with sign-in button
  - /app/dashboard/page.tsx: Protected dashboard to list user documents
  - /app/doc/[id]/page.tsx: Document viewer page
  - /app/desktop-login/page.tsx: Desktop authentication initiation page
  - /app/desktop-done/page.tsx: Desktop authentication completion page
- Create /lib directory with:
  - /lib/aws.ts: AWS SDK client configurations
  - /lib/auth.ts: NextAuth configuration with Cognito provider
  - /lib/dynamodb.ts: DynamoDB helper functions
  - /lib/s3.ts: S3 helper functions for presigned URLs
  - /lib/jwt.ts: JWT validation utilities for desktop tokens
- Set up environment variables in .env.local for development
- Configure Tailwind for a clean, modern UI
- Add proper TypeScript types for all data structures

Unit Test:
Create a simple test that verifies AWS SDK clients can be instantiated with the environment variables

Human Test:
Run "npm run dev" and verify:
- Home page loads without errors
- Tailwind styles are working
- No console errors in browser
- Can navigate to all routes (they should render basic placeholder content)

---

PHASE 3: NextAuth Integration with Cognito

Human Actions Required:
- Update Cognito User Pool callback URLs in AWS console to include your development and production domains
- Set NEXTAUTH_SECRET environment variable to a random 32-character string
- Set NEXTAUTH_URL environment variable

Prompt:
Implement complete NextAuth integration with Amazon Cognito:
- Configure NextAuth in /app/api/auth/[...nextauth]/route.ts with Cognito provider
- Set up proper session management with JWT strategy
- Create authentication middleware in middleware.ts to protect dashboard and doc routes
- Implement sign-in/sign-out functionality with proper error handling
- Add user session components and navigation
- Style the authentication pages using Tailwind CSS with a clean, modern design
- Add proper TypeScript types for user sessions and Cognito responses
- Handle authentication errors gracefully with user-friendly messages
- Implement automatic token refresh if possible
- Add loading states for authentication flows

Unit Test:
Write a test that mocks the Cognito provider and verifies NextAuth configuration is valid

Human Test:
Run the app and test the complete authentication flow:
- Click sign-in and verify redirect to Cognito Hosted UI
- Complete sign-in with a test user account (create one in Cognito console if needed)
- Verify successful redirect back to dashboard
- Test sign-out functionality
- Verify protected routes redirect to sign-in when not authenticated
- Check that user session persists across browser refresh

---

PHASE 4: Desktop Authentication API Endpoints

Human Actions Required:
- Test the web authentication flow first to ensure Cognito is working properly
- Prepare a simple HTTP client tool (like curl or Postman) for testing API endpoints

Prompt:
Implement the desktop authentication flow API endpoints:
- /app/api/desktop-init/route.ts: POST endpoint that generates a unique nonce, stores it in DynamoDB with status "pending" and 5-minute expiration, returns nonce and login URL
- /app/api/desktop-token/route.ts: GET endpoint that checks nonce status in DynamoDB, returns desktop JWT when status is "ready"
- Update /app/desktop-login/page.tsx: Accept nonce parameter, store in DynamoDB, redirect to Cognito
- Update /app/desktop-done/page.tsx: Detect authenticated session, update nonce status to "ready" with userId in DynamoDB
- Implement JWT creation for desktop clients with appropriate expiration (1 hour)
- Add proper error handling and validation for all endpoints
- Use crypto.randomUUID() for nonce generation
- Add rate limiting to prevent abuse
- Implement proper CORS headers for desktop client requests
- Add comprehensive logging for debugging

Unit Test:
Write tests for each API endpoint:
- Test nonce generation and storage
- Test nonce retrieval and JWT generation
- Test invalid nonce handling
- Test expired nonce cleanup

Human Test:
Test the complete desktop authentication flow:
- POST to /api/desktop-init and verify you get a nonce and login URL
- Open the login URL in browser and complete authentication
- Poll /api/desktop-token with the nonce until you receive a JWT
- Verify the JWT contains correct user information
- Test with invalid/expired nonces to ensure proper error handling

---

PHASE 5: Document Storage API

Human Actions Required:
- Verify S3 bucket permissions are correctly configured
- Test that your AWS credentials can generate presigned URLs

Prompt:
Implement the document storage API endpoints:
- /app/api/presign/route.ts: POST endpoint that validates JWT, generates presigned PUT/GET URLs for S3 based on mode parameter, enforces user-specific key prefixes (userId/docId.ext)
- /app/api/documents/route.ts: GET endpoint to list user documents from DynamoDB, POST endpoint to register document metadata after successful upload
- Add proper JWT validation middleware for desktop authentication
- Implement S3 key structure: {userId}/{docId}.{extension}
- Add document metadata schema: docId, userId, fileName, fileSize, uploadedAt, lastModified
- Set presigned URL expiration to 60 seconds for security
- Add file type validation (only allow common document formats: .odt, .ods, .odp, .doc, .docx, etc.)
- Implement proper error handling for S3 operations
- Add file size limits (e.g., 50MB max)
- Include comprehensive request/response logging

Unit Test:
Write tests for:
- Presigned URL generation with valid JWT
- Document metadata storage and retrieval
- File type validation
- JWT validation and error cases
- User isolation (users can't access other users' documents)

Human Test:
Test the document storage flow:
- Use a valid desktop JWT to request a presigned PUT URL
- Upload a test document directly to S3 using the presigned URL
- Verify the document appears in S3 with correct key structure
- Register the document metadata via POST /api/documents
- Verify document appears in GET /api/documents response
- Test presigned GET URL by downloading the document
- Test error cases: invalid JWT, expired URLs, unauthorized access

---

PHASE 6: Dashboard UI Implementation

Human Actions Required:
- Prepare some test documents in various formats for testing the document list
- Ensure you have completed authentication setup so you can access protected routes

Prompt:
Implement a complete dashboard UI for managing cloud documents:
- Create /app/dashboard/page.tsx with a clean, responsive design using Tailwind CSS
- Display user information and sign-out option in header
- Show list of user documents with: filename, upload date, file size, actions (download, delete)
- Add file upload functionality with drag-and-drop support
- Implement document download using presigned GET URLs
- Add document deletion with confirmation dialog
- Include loading states and proper error handling
- Add search/filter functionality for documents
- Implement responsive design that works on mobile devices
- Add empty state when user has no documents
- Include file type icons for different document formats
- Add file upload progress indicators
- Implement client-side file validation before upload

Unit Test:
Write component tests for:
- Document list rendering
- File upload functionality
- Search/filter features
- Error state handling

Human Test:
Test the complete dashboard experience:
- Sign in and verify dashboard loads with your user info
- Upload various document types via drag-and-drop and file picker
- Verify documents appear in the list with correct metadata
- Test download functionality
- Test delete functionality with confirmation
- Test search/filter features
- Verify responsive design on different screen sizes
- Test error states (network failures, file too large, etc.)

---

PHASE 7: LibreOffice C++ Cloud Authentication Handler

Human Actions Required:
- Ensure you have a working LibreOffice development environment
- Install libcurl development headers on your system
- Verify you can build LibreOffice core successfully

Prompt:
Implement the LibreOffice cloud authentication integration:
- Add new menu item "Login to Cloud" in sw/uiconfig/swriter/menubar/menubar.xml
- Create new UNO command .uno:LoginToCloud and define slot SID_LOGIN_TO_CLOUD in sfx2
- Implement CloudAuthHandler class in sfx2/source/control/ to handle authentication flow
- Implement CloudApiClient class in sfx2/source/control/ for HTTP communication using libcurl
- Add authentication dialog using VCL components to show "Complete login in browser" message
- Implement nonce generation and API communication with the Next.js backend
- Add background thread for polling /api/desktop-token endpoint
- Store JWT securely in LibreOffice configuration using officecfg
- Handle SolarMutex correctly for thread-safe UI updates
- Add proper error handling and user feedback
- Implement platform-agnostic browser opening using sal (System Abstraction Layer)
- Follow LibreOffice coding conventions and patterns

Unit Test:
Write C++ unit tests for:
- CloudApiClient HTTP communication
- JWT storage and retrieval
- Nonce generation and validation
- Error handling scenarios

Human Test:
Build and run LibreOffice, then test the authentication:
- Open LibreOffice Writer
- Verify "Login to Cloud" appears in File menu
- Click "Login to Cloud" and verify browser opens to correct URL
- Complete authentication in browser
- Verify LibreOffice shows success message and dialog closes
- Check that JWT is stored in LibreOffice configuration
- Restart LibreOffice and verify login state persists

---

PHASE 8: LibreOffice Save to Cloud Implementation

Human Actions Required:
- Ensure the authentication handler from Phase 7 is working correctly
- Have some test documents ready for uploading

Prompt:
Implement the "Save to Cloud" functionality in LibreOffice:
- Add "Save to Cloud" menu item in File menu alongside existing save options
- Create new UNO command .uno:SaveToCloud with corresponding slot
- Implement SaveToCloudHandler class that uses CloudApiClient to get presigned PUT URLs
- Modify SfxMedium class or create wrapper to intercept cloud:// URL scheme
- Implement direct HTTP PUT to S3 using document byte stream
- Add progress dialog for upload operations
- Handle upload failures gracefully with retry options
- Generate unique document IDs and maintain local mapping
- Add metadata upload after successful S3 upload
- Implement "Refresh from Cloud" functionality with conflict resolution
- Add proper error handling for network issues and authentication failures
- Follow LibreOffice document handling patterns and conventions

Unit Test:
Write C++ unit tests for:
- Document serialization for upload
- Presigned URL request handling
- Upload progress tracking
- Error recovery mechanisms

Human Test:
Test the complete Save to Cloud workflow:
- Open a document in LibreOffice Writer
- Make some changes to the document
- Click File > Save to Cloud
- Verify upload progress dialog appears
- Check that document appears in the web dashboard
- Modify the document and save to cloud again
- Test "Refresh from Cloud" to download latest version
- Test error cases: no internet, authentication expired, server errors
- Verify document integrity after upload/download cycle

---

PHASE 9: Integration Testing and Polish

Human Actions Required:
- Set up a production-like testing environment
- Prepare various document types and sizes for comprehensive testing

Prompt:
Perform comprehensive integration testing and polish the entire system:
- Test complete user journey from authentication to document management
- Implement proper error messaging throughout the system
- Add loading states and progress indicators where needed
- Optimize API response times and add caching where appropriate
- Implement proper logging and monitoring
- Add user onboarding flow and help documentation
- Test edge cases: large files, network interruptions, concurrent uploads
- Verify security: JWT expiration, presigned URL security, user isolation
- Add proper input validation and sanitization throughout
- Implement graceful degradation for offline scenarios
- Test cross-platform compatibility (Windows, macOS, Linux)
- Add automated health checks for API endpoints

Unit Test:
Create comprehensive integration tests covering:
- End-to-end authentication flow
- Document upload/download cycles
- Error recovery scenarios
- Security boundary testing

Human Test:
Perform thorough manual testing:
- Test on multiple operating systems and browsers
- Upload documents of various sizes and formats
- Test concurrent access from desktop and web
- Verify security by attempting unauthorized access
- Test system under load with multiple documents
- Verify all error messages are user-friendly and actionable
- Test the complete workflow with fresh user accounts
- Verify proper cleanup of temporary resources
- Test document integrity across multiple save/load cycles 