# Phase 1: Next.js Web Application for Authentication and Cloud Storage

This document provides a step-by-step guide for an AI coding agent to build the backend web application that powers cloud authentication and storage for the LibreOffice fork. Each step includes prerequisites for the human user, a direct prompt for the AI agent, and verification tests.

## Milestone 1: Local Authentication & Database Setup

### Step 1.1: Project Scaffolding and Setup

**Human User Actions:**
- Ensure Node.js (v18+) and pnpm are installed.
- Create a project directory for the web app.
- Sign up for a free tier Postgres database provider (e.g., Neon).
- Copy the Postgres connection string. It will look like: `postgresql://user:password@host:port/dbname`.
- Create a new Google Cloud Platform project.
- In the GCP console, navigate to "APIs & Services" -> "Credentials".
- Create an "OAuth 2.0 Client ID" for a "Web application".
- Add `http://localhost:3000` to "Authorized JavaScript origins".
- Add `http://localhost:3000/api/auth/callback/google` to "Authorized redirect URLs".
- Copy the "Client ID" and "Client Secret".
- In your project directory, create a file named `.env.local`.
- Add the following environment variables to `.env.local`, replacing the placeholder values:
  `DATABASE_URL="your_postgres_connection_string"`
  `GOOGLE_CLIENT_ID="your_google_client_id"`
  `GOOGLE_CLIENT_SECRET="your_google_client_secret"`
  `NEXTAUTH_URL="http://localhost:3000"`
  `NEXTAUTH_SECRET="generate_a_secret_using_openssl_rand_base64_32"`

**AI Coding Agent Prompt:**
- Create a new Next.js project in the current directory using the App Router. Call 'nvm use 18' first whenever you need to make Node-related calls.
- Use `pnpm` as the package manager.
- Configure the project to use Tailwind CSS.
- Install the following dependencies: `@prisma/client`, `@next-auth/prisma-adapter`, `next-auth`, `prisma`.
- Install `prisma` as a dev dependency.
- Initialize Prisma with the `postgresql` provider.

**Human User Test:**
- Run `pnpm install` in your terminal.
- Run `pnpm dev`.
- The Next.js application should start without errors on `http://localhost:3000`.

### Step 1.2: Database Schema and UI

**Human User Actions:**
- No new actions required.

**AI Coding Agent Prompt:**
- Create a Prisma schema in `prisma/schema.prisma`.
- The schema must include models for `User`, `Account`, `Session`, `VerificationToken` as required by the NextAuth Prisma adapter.
- Add a `Document` model with the following fields:
  - `id` (String, CUID, default, id)
  - `filename` (String)
  - `s3_key` (String, unique)
  - `size` (Int)
  - `mime_type` (String)
  - `createdAt` (DateTime, default now)
  - `updatedAt` (DateTime, updated at)
  - `owner` (Relation to `User` model)
  - `ownerId` (String, foreign key)
- Add a `DesktopLoginNonce` model with the following fields:
  - `id` (String, CUID, default, id)
  - `nonce` (String, unique)
  - `expiresAt` (DateTime)
  - `user` (Relation to `User` model, optional)
  - `userId` (String, foreign key, optional)
- After defining the schema, run the command to push the schema to the database.
- Configure NextAuth in `app/api/auth/[...nextauth]/route.ts`. Use the Prisma adapter and configure the Google provider with the client ID and secret from the environment variables.
- Create a basic UI:
  - A login page (`/`) that shows a "Sign in with Google" button if the user is not authenticated.
  - A protected dashboard page (`/dashboard`) that shows the user's email and a "Sign Out" button. This page should redirect to `/` if the user is not authenticated.

**Unit Test:**
- No automated unit test for this step. Manual verification is required.

**Human User Test:**
- Run `pnpm exec prisma db push` to sync the database schema.
- Run `pnpm dev`.
- Navigate to `http://localhost:3000`.
- Click the "Sign in with Google" button and complete the authentication flow.
- You should be redirected to `/dashboard` and see your email address.
- Verify in your Postgres database that a new user has been created in the `User` table.
- Click the "Sign Out" button. You should be redirected back to the login page.

## Milestone 2: Cloud Storage Integration (S3 Pre-signed URLs)

### Step 2.1: S3 Setup and API Endpoints

**Human User Actions:**
- Sign up for an AWS account.
- Create a new S3 bucket in your preferred region (e.g., `us-east-1`).
- Create a new IAM User with "programmatic access".
- Create and attach a new policy to this user with the following JSON, replacing `your-bucket-name` with your actual S3 bucket name:
  ```json
  {
      "Version": "2012-10-17",
      "Statement": [
          {
              "Effect": "Allow",
              "Action": [
                  "s3:PutObject",
                  "s3:GetObject"
              ],
              "Resource": "arn:aws:s3:::your-bucket-name/*"
          }
      ]
  }
  ```
- Copy the "Access key ID" and "Secret access key" for the IAM user.
- Add the following variables to your `.env.local` file:
  `AWS_ACCESS_KEY_ID="your_iam_access_key"`
  `AWS_SECRET_ACCESS_KEY="your_iam_secret_key"`
  `AWS_REGION="your_s3_bucket_region"`
  `S3_BUCKET_NAME="your_bucket_name"`

**AI Coding Agent Prompt:**
- Install the AWS SDK v3 for S3: `@aws-sdk/client-s3` and `@aws-sdk/s3-request-presigner`.
- Create a new API route `app/api/docs/presign/route.ts`. This route should be protected, checking for an active NextAuth session.
- It must accept a `GET` request with two query parameters: `docId` (a string) and `mode` (either 'put' or 'get').
- Implement the logic:
  - Validate the `mode` parameter.
  - Find the document in the database using `docId` and ensure it belongs to the authenticated user.
  - Construct the S3 key as `${userId}/${docId}.odt`.
  - Depending on the `mode`, create either a `PutObjectCommand` or `GetObjectCommand`.
  - Generate a pre-signed URL using `getSignedUrl` that expires in 60 seconds.
  - Return a JSON response with the signed URL.
- Create another API route `app/api/docs/metadata/route.ts`. This route must be protected.
- It must accept a `POST` request with a JSON body containing `docId`, `filename`, `size`, and `mime_type`.
- Implement the logic to find the document by `docId` and update its metadata. If it doesn't exist, create a new document record associated with the authenticated user.

**Unit Test:**
- You can use a testing framework like Vitest or Jest.
- Write a test for the `/api/docs/presign` endpoint.
- Mock the NextAuth session to simulate an authenticated user.
- Mock the Prisma client to return a document owned by the mocked user.
- Mock the S3 `getSignedUrl` function and verify that it's called with the correct parameters (e.g., Bucket, Key, command type).
- Assert that the API returns a 200 OK status and a JSON payload containing a string URL.

**Human User Test:**
- Manually add a dummy document record to your `Document` table in the database, associating it with your user ID.
- Log into the web application.
- Use a tool like `curl` or Postman to make a `GET` request to `http://localhost:3000/api/docs/presign?mode=put&docId=your_doc_id`.
- Verify you receive a JSON object with a signed S3 URL.
- Create a small test file (e.g., `test.txt`).
- Use `curl` to upload the file to the signed URL: `curl -X PUT --upload-file test.txt "THE_SIGNED_URL"`.
- Check your S3 bucket to confirm that the file (`your_user_id/your_doc_id.odt`) has been successfully uploaded.

## Milestone 3: Desktop Client Authentication Handshake

### Step 3.1: Desktop Login API Endpoints

**Human User Actions:**
- No new actions are required. You can optionally set up a Redis instance (e.g., from Upstash) and add `REDIS_URL` to your `.env.local` for a more scalable nonce store, but the prompt will default to the Postgres database.

**AI Coding Agent Prompt:**
- Install `jsonwebtoken`.
- Create the API route `app/api/desktop/init-login/route.ts`.
- It must handle `POST` requests.
- Logic: Generate a secure, unique nonce. Store it in the `DesktopLoginNonce` table with a 5-minute expiration time. Return the nonce in the JSON response.
- Create the page `app/desktop-login/page.tsx`.
- This page must read a `nonce` from the URL query parameters. If no user is logged in, show the login buttons. If a user is logged in, it should show a message like "Authenticating..." and make a `POST` request to `/api/desktop/bind-nonce` with the nonce.
- Create the API route `app/api/desktop/bind-nonce/route.ts` to handle the `POST` from the previous step. This route will find the nonce, verify it's not expired, and associate the session user's ID with it in the database. On success, the `desktop-login` page should display "Success! You can close this window."
- Create the API route `app/api/desktop/token/route.ts`.
- It must handle `GET` requests with a `nonce` query parameter.
- Logic:
  - Find the nonce in the DB. If not found or expired, return a 404/410 error.
  - If the nonce has no `userId` yet, return a 202 Accepted status.
  - If the nonce has a `userId`, generate a short-lived JWT (1-hour expiry) containing the `userId`. Delete the nonce from the database. Return the JWT in a JSON response.

**Unit Test:**
- Write a test for the `/api/desktop/token` endpoint.
- Test the three main states: nonce does not exist, nonce exists but is pending (no `userId`), and nonce is complete (`userId` is present).
- For the success case, mock the `jsonwebtoken.sign` function and verify the database delete function is called on the nonce.

**Human User Test:**
1. Run the web app with `pnpm dev`.
2. In a terminal, get a nonce: `curl -X POST http://localhost:3000/api/desktop/init-login`. Copy the returned nonce.
3. In your browser, open `http://localhost:3000/desktop-login?nonce=THE_NONCE_YOU_GOT`.
4. Log in using Google. You should see the success message.
5. In your terminal, poll for the token: `curl http://localhost:3000/api/desktop/token?nonce=THE_NONCE_YOU_GOT`.
6. You should receive a JSON object containing a JWT.
7. Trying to use the same nonce again should result in an error. This JWT is what the desktop client will use for subsequent API calls. 