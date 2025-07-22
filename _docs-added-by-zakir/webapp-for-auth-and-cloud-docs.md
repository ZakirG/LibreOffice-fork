Implementation plan (no code) for the Next.js webapp that handles auth + cloud docs for the LibreOffice fork, staying AWS-first and provisioned with OpenTofu.

---

1. High-level architecture

* Frontend/UI: Next.js (App Router) deployed on Vercel.
* Auth: Amazon Cognito User Pool (OAuth2 Authorization Code + PKCE). NextAuth will use Cognito as an OAuth provider for web; desktop uses a “device login” flow you implement on top of Cognito.
* Persistence/metadata: DynamoDB (user -> document index, upload metadata, desktop auth nonces).
* Object storage: S3 bucket for documents/audio. Access only via short-lived pre-signed URLs issued by your API.
* API layer: Next.js Route Handlers (/app/api/\*) for auth callbacks, presign endpoints, document list APIs. These call AWS SDKs directly.
* Background processing (if needed later): Lambda invoked via EventBridge (OpenTofu provisioned).
* Secrets/config: AWS SSM Parameter Store; pushed to Vercel via CI at deploy time.
* Edge caching (optional later): CloudFront in front of S3 for GETs that can be public or signed URLs.
* Infra-as-code: OpenTofu modules for Cognito, S3, DynamoDB, Parameter Store, IAM roles/policies.

---

2. AWS resources to provision with OpenTofu

* S3:

  * Bucket: libre-docs (private).
  * Bucket policy denying all public access.
* DynamoDB:

  * Table: documents (PK: userId, SK: docId) with GSI on docId if needed.
  * Table: desktop\_login (PK: nonce) storing temporary login handshakes (nonce, userId, expiresAt).
* Cognito:

  * User Pool, hosted UI domain, app client (no secret, PKCE enabled).
  * Callback URLs: [https://yourapp.vercel.app/api/auth/callback/cognito](https://yourapp.vercel.app/api/auth/callback/cognito)
  * Logout URLs: [https://yourapp.vercel.app/](https://yourapp.vercel.app/)
  * Allowed OAuth scopes: openid, email, profile, and custom if needed.
* IAM:

  * Role with minimal S3 PutObject/GetObject permissions restricted by key prefix \${userId}/\* for presign commands (the presign happens server-side, but still follow least privilege).
  * Policy to allow DynamoDB CRUD on your two tables.
* SSM Parameter Store:

  * /libreapp/cognito/client\_id
  * /libreapp/cognito/pool\_id
  * /libreapp/aws/region
  * /libreapp/s3/bucket
  * SMTP creds if you add magic links, etc.
* Optionally ElastiCache (Redis) if you prefer it over DynamoDB for the desktop nonce store. DynamoDB is fine and simpler.
* (Optional) CloudFront distribution fronting S3 for GET-only doc viewing (mp3 streaming). You would still generate signed CloudFront URLs instead of S3 pre-signed if you want CDN.

---

3. Next.js project structure (no code, just paths)

* /app

  * /dashboard (lists docs)
  * /doc/\[id] (renders doc viewer)
  * /desktop-login (page to initiate desktop auth handshake)
  * /desktop-done (page hit after Cognito redirect to finalize handshake)
* /app/api

  * /auth/\[...nextauth]/route.ts (NextAuth handler using Cognito provider)
  * /presign/route.ts (POST: returns PUT/GET pre-signed URL)
  * /documents/route.ts (GET: list docs; POST: register new doc meta)
  * /desktop-token/route.ts (GET: desktop polls with nonce; returns JWT/session token once login done)
  * /desktop-init/route.ts (POST: desktop requests a nonce)
* /lib (AWS SDK wrappers, token validators)
* /infra (Optional: place OpenTofu configs here, or separate repo)
* /scripts (CI sync of SSM params to Vercel env)

---

4. Auth flows in detail

4.1 Web/mobile (browser)

* User hits /dashboard. If no session, redirect to /api/auth/signin (NextAuth → Cognito Hosted UI).
* Cognito returns code → NextAuth exchanges for tokens → session JWT stored in secure cookies.
* User now calls /api/presign etc., validated via getServerSession.

4.2 Desktop (device login style)

* LibreOffice fork launches browser to [https://yourapp.vercel.app/desktop-login?nonce={generated\_by\_desktop}](https://yourapp.vercel.app/desktop-login?nonce={generated_by_desktop})
* /desktop-login stores nonce in DynamoDB with status=pending, expiresAt \~5 min.
* Redirect user to Cognito Hosted UI.
* After login, Cognito returns to /api/auth/callback/cognito → NextAuth sets session.
* /desktop-done page detects a valid session, then writes userId into desktop\_login table for that nonce and marks status=ready. Shows “You can go back to the app”.
* Desktop polls /api/desktop-token?nonce=... every few seconds. When ready, API verifies record and returns a short-lived “desktop JWT”. Desktop stores it encrypted locally and uses it as Bearer for subsequent API calls.
* (Optional) Rotate desktop token by exposing /api/token/refresh using refresh token stored server-side.

---

5. Document “Save to Cloud” and “Refresh from Cloud”

5.1 Upload flow

* Desktop requests /api/presign?mode=put\&docId=xyz with Authorization: Bearer desktopJWT.
* Handler validates user, checks they are allowed to write docId (either new or owned).
* Generates pre-signed PUT URL for key: \${userId}/\${docId}.odt (or .odg etc.).
* Desktop uploads binary directly to S3 via HTTPS PUT.
* Desktop then POSTs /api/documents to record/update metadata (size, hash, updatedAt).

5.2 Download flow

* Desktop requests /api/presign?mode=get\&docId=xyz.
* Gets pre-signed GET; downloads file.

5.3 Mobile/web viewing

* /dashboard fetches doc list from DynamoDB.
* /doc/\[id] fetches a short-lived GET URL and either streams content or converts to a preview (you could add Lambda for HTML preview later; out of scope now).

---

6. Security & session management

* All API routes validate Cognito-issued JWT via NextAuth (server-side) or custom verifier (desktop JWT).
* Short expiry (e.g., 60s) on presigned URLs.
* DynamoDB TTL attribute on desktop\_login rows for auto-expiration.
* Encrypt desktop JWT at rest using OS keychain or simple AES with hardware-bound key if possible.
* Set CSP/secure headers in Next.js middleware.

---

7. OpenTofu provisioning workflow

* Separate IaC repo or /infra directory.
* Modules:

  * modules/cognito-user-pool
  * modules/s3-bucket
  * modules/dynamodb-table
  * modules/iam-roles
  * modules/ssm-params
* Root stacks per environment (dev, prod).
* Use OpenTofu state in an S3 backend with DynamoDB state locking.
* Pipeline:

  * GitHub Actions: run `tofu plan` on PR, `tofu apply` on main.
  * After apply, sync required outputs (client\_id, pool\_id, bucket name, region) to Vercel via `vercel env pull` or Vercel API.

---

8. CI/CD pipeline overview

* GitHub Actions (or CodeBuild):

  * Job 1: OpenTofu fmt/validate/plan/apply.
  * Job 2: Pull SSM params and set Vercel env vars.
  * Job 3: Trigger Vercel deploy via `vercel --prod`.
* Optional: run lint/test before deploy.

---

9. Observability & ops

* CloudWatch logs for Cognito/Lambda (if used) and DynamoDB metrics.
* Add simple structured logging in Next.js API routes (ship to Datadog or use Vercel Logs).
* DynamoDB alarms on throttled writes/reads.
* S3 metrics for data transfer/requests.

---

10. Local development

* Use `dotenv` for local env; create a separate Cognito user pool dev stack via OpenTofu.
* For S3/DynamoDB, either point to AWS dev resources or use LocalStack (if you want full offline).
* NextAuth dev callback: [http://localhost:3000/api/auth/callback/cognito](http://localhost:3000/api/auth/callback/cognito)
* Desktop dev: allow an HTTP redirect to localhost for testing or use the same nonce polling flow.

---

11. Future-proofing

* Multi-user doc sharing: add a documents\_shared table or a GSI on documents for docId -> permissions list.
* Collaborative editing: later integrate a CRDT service (e.g., yjs) fronted by WebSocket on AWS (API Gateway WebSockets + Lambda or self-hosted SignalR equivalent).
* AI “Smart Rewrite”: add a Lambda that calls Bedrock or OpenAI, triggered via Next.js API.

That’s the full plan. Tell me which piece you want expanded next (schemas, nonce handshake sequence, OpenTofu module variables/outputs, etc.).
