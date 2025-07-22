Implementation Plan — Next.js Webapp for Auth + Cloud Docs (S3) for LibreOffice Fork

---

1. Scope & Responsibilities

* Provide user authentication (NextAuth, email + social).
* Broker secure access to S3 via short‑lived pre‑signed URLs.
* Expose a minimal REST/JSON API the desktop app (LibreOffice fork) can hit for login handoff and Save/Refresh.
* Render a mobile-friendly dashboard for browsing/previewing documents (and playing embedded MP3s).
* Store minimal metadata (user, docs, last\_modified) to speed listings and sharing later.

---

2. Core Architecture

* Next.js on Vercel (Pages or App Router; pick one and stay consistent).
* NextAuth for identity/session (JWT strategy).
* Postgres (Neon/PlanetScale/RDS) via Prisma (sessions, users, doc metadata).
* AWS S3 for binary file storage.
* AWS SDK v3 in API routes for presigning.
* Optional Redis (Upstash) for short-lived device/desktop login tokens (nonce polling).
* Desktop <-> Web auth bridge using browser-based OAuth and a polling/nonce or local-loopback callback.

---

3. Data/Objects You’ll Manage

* User table: id, email, created\_at.
* Document table: id (uuid), user\_id, filename, s3\_key, size, mime, updated\_at.
* DesktopLoginNonce table or Redis key: nonce, user\_id (nullable until login), expires\_at.
* (Optional) DocRevisions table if you want history.
* S3 key convention: `${user_id}/${doc_id}.odt` (and `.json` for metadata, `.mp3` for audio clips, etc.).

---

4. API Surface (serverless routes)

* POST /api/desktop/init-login  → returns { nonce }.
* GET  /api/desktop/token?nonce=… → returns { jwt } once user completes browser login.
* GET  /api/docs                → list docs for current session user (DB read, maybe S3 list fallback).
* POST /api/docs/presign?mode=put|get\&docId=… → returns { url, key, contentType }.
* POST /api/docs/metadata       → upsert doc metadata after successful upload.
* (Optional) GET /api/docs/\:id   → stream/transform doc for preview (or just redirect to signed GET).
  All routes enforce auth via getServerSession() or bearer JWT from desktop.

---

5. Auth Flow Variants
   A) Regular Web/Mobile:

* User hits /login → NextAuth providers → session cookie (httpOnly).
* All dashboard pages gate on session.

B) Desktop App (device-code style):

* Desktop calls /api/desktop/init-login → gets nonce and opens system browser: [https://app.vercel.app/desktop-login?nonce=XYZ](https://app.vercel.app/desktop-login?nonce=XYZ)
* User logs in (NextAuth). After success, web page POSTs to /api/desktop/bind-nonce with session user → store user\_id against nonce in Redis.
* Desktop polls /api/desktop/token?nonce=XYZ. When ready, server issues a signed short-lived JWT (different from NextAuth cookie) for API use.
* Desktop stores that JWT locally and uses it on every presign/metadata call (Authorization: Bearer).

(Alternative: run a local HTTP listener and use OAuth redirect there. This plan keeps web simple.)

---

6. Document Save/Refresh Flow (Desktop)
   Save to Cloud:

* Desktop: call /api/docs/presign?mode=put\&docId=… → receive signed PUT URL.
* Desktop: PUT file bytes to S3 directly.
* Desktop: POST /api/docs/metadata with size, name, updated\_at.
  Refresh from Cloud:
* Desktop: call /api/docs/presign?mode=get\&docId=… → receive signed GET URL.
* Desktop: GET and overwrite local doc.

---

7. Mobile Dashboard UX

* /dashboard lists docs (query DB; fallback: S3 ListObjectsV2 if DB missing).
* /doc/\[id] displays title, last modified, play button if associated MP3 exists.
* For MP3: request GET presign and stream via HTML5 audio.
* Responsive CSS (Tailwind) + no heavy client libs.

---

8. Security & Config

* AWS IAM user with policy: s3\:PutObject/GetObject/ListBucket limited to your bucket (but only from backend).
* Vercel env vars: NEXTAUTH\_SECRET, NEXTAUTH\_URL, DB\_URL, AWS\_ACCESS\_KEY\_ID, AWS\_SECRET\_ACCESS\_KEY, AWS\_REGION, S3\_BUCKET, REDIS\_URL (if used), EMAIL SMTP creds.
* Short presign expiry (<= 60s). Desktop should upload immediately.
* Validate docId belongs to session.user.id before presigning.
* Log all presign calls for debugging.

---

9. Human User Actions (End Users)

* Visit web app, click “Login” (email magic link or Google).
* View dashboard, tap a doc, preview/read text, play MP3.
* On desktop LibreOffice fork:

  * Click “Login to Cloud” → browser opens; user signs in.
  * Click “Save to Cloud” → file uploads.
  * Later “Refresh from Cloud” → pulls latest.

---

10. AI Coding Agent Actions (What you tell your code-gen assistant)

* Scaffold Next.js project (choose Pages or App Router, specify).
* Install and configure NextAuth with providers (email + Google) and Prisma adapter.
* Generate Prisma schema for users, sessions, documents, nonces; run migrations.
* Implement /api/desktop/init-login, /api/desktop/bind-nonce, /api/desktop/token with Redis or DB TTL records.
* Implement /api/docs, /api/docs/presign, /api/docs/metadata with auth checks.
* Implement S3 client + getSignedUrl helpers.
* Build dashboard pages: list, detail, audio player, auth guard.
* Add responsive styling (Tailwind).
* Configure Vercel project: env vars, build settings, custom domain.
* Write desktop-side HTTP spec (JSON payloads, headers) and document it.
* Add logging & basic error handling (expired nonce, missing docId, invalid token).
* Write minimal integration tests for presign routes and auth gating.
* (Optional) Add rate limiting middleware for presign endpoints.
* (Optional) Add share links & ACL groundwork (future).

---

11. Milestones
    M1: Auth + DB running locally (NextAuth sign-in works).
    M2: Presign API endpoints functional; manual curl uploads to S3 succeed.
    M3: Desktop nonce login handshake works end-to-end.
    M4: Dashboard lists docs and plays MP3.
    M5: Polish, error states, deploy to prod Vercel + AWS.

Done. Ping me when you want exact API contracts or Prisma schema.
