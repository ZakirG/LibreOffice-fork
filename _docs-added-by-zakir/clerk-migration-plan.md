# Implementation Plan: Migrating from NextAuth.js to Clerk

This document outlines the step-by-step plan to replace the current NextAuth.js authentication implementation with Clerk in the `libre-cloud-webapp`. The goal is to simplify the auth stack, improve user experience with pre-built components, and leverage Clerk's robust user management, while retaining our existing AWS backend for document storage.

### Analysis of Current NextAuth Implementation

The current setup uses `next-auth` with a Google provider. Key files to be replaced or refactored include:
-   **Core Logic**: `src/app/api/auth/[...nextauth]/route.ts` and `src/lib/auth.ts`.
-   **Middleware**: `middleware.ts` which protects routes.
-   **UI Components**: `src/components/navigation.tsx` (uses `useSession`), `src/components/session-provider.tsx`.
-   **Auth Pages**: `src/app/auth/signin/page.tsx` and `src/app/auth/error/page.tsx`.
-   **Protected Pages**: `src/app/dashboard/page.tsx` (uses `getServerSession`).
-   **Desktop Auth Flow**: `src/app/desktop-login/page.tsx`, `src/app/desktop-done/page.tsx`, and the JWT generation logic in `src/lib/jwt.ts`.

---

### PHASE 1: Clerk Project Setup & Initial Integration

**Goal:** Remove NextAuth and install Clerk, setting up the basic provider and environment variables.

**Human Actions Required:**
-   Create a new application in your [Clerk Dashboard](https://dashboard.clerk.com/).
-   In the Clerk Dashboard, navigate to **"Providers"** and ensure **"Google"** is enabled.
-   From the Clerk Dashboard, copy the **Publishable Key** and **Secret Key**.

**Prompt Actions:**

1.  **Dependency Management:**
    -   Run `npm uninstall next-auth`.
    -   Run `npm install @clerk/nextjs`.

2.  **Environment Variables:**
    -   Update `libre-cloud-webapp/.env.local` to remove all `GOOGLE_*` and `NEXTAUTH_*` variables.
    -   Add the new Clerk environment variables:
        ```env
        NEXT_PUBLIC_CLERK_PUBLISHABLE_KEY=your_publishable_key
        CLERK_SECRET_KEY=your_secret_key
        ```

3.  **Clerk Provider Setup:**
    -   Delete the `src/components/session-provider.tsx` file.
    -   Modify `src/app/layout.tsx` to wrap the `<body>` content with the `<ClerkProvider>` component.

        ```tsx
        // src/app/layout.tsx
        import { ClerkProvider } from '@clerk/nextjs'

        export default function RootLayout({ children }) {
          return (
            <ClerkProvider>
              <html lang="en">
                <body>{children}</body>
              </html>
            </ClerkProvider>
          );
        }
        ```

---

### PHASE 2: Middleware and UI Component Replacement

**Goal:** Replace NextAuth middleware and React components with Clerk's equivalents to handle user sessions and UI state.

**Prompt Actions:**

1.  **Middleware Replacement:**
    -   Rewrite `libre-cloud-webapp/middleware.ts` to use Clerk's `authMiddleware`.
    -   Define public and protected routes. The `/desktop-login` and `/desktop-done` pages should be public to allow the desktop flow to work for signed-out users.

        ```ts
        // middleware.ts
        import { authMiddleware } from "@clerk/nextjs/server";

        export default authMiddleware({
          publicRoutes: [
            "/",
            "/auth/signin", // To be replaced by Clerk's pages
            "/auth/error",
            "/desktop-login",
            "/desktop-done",
            "/api/desktop-init",
            "/api/desktop-token"
          ],
        });

        export const config = {
          matcher: ["/((?!.*\\..*|_next).*)", "/", "/(api|trpc)(.*)"],
        };
        ```

2.  **Navigation Component Update:**
    -   Refactor `src/components/navigation.tsx`.
    -   Remove `useSession` and `signOut` hooks from `next-auth/react`.
    -   Use Clerk's `<SignedIn>`, `<SignedOut>`, and `<UserButton />` components to manage the display of user state.
    -   The "Sign In" button can be replaced with Clerk's `<SignInButton />` or a link to `/sign-in`.

3.  **Cleanup Auth Pages:**
    -   Delete the entire `src/app/auth` directory. Clerk provides its own hosted pages for sign-in, sign-up, and user profile management. We will create routes for them in the next phase.

---

### PHASE 3: Page Protection and Clerk-Hosted Pages

**Goal:** Protect pages using Clerk's helpers and create routes for Clerk's UI components.

**Prompt Actions:**

1.  **Update Dashboard Page:**
    -   Modify `src/app/dashboard/page.tsx` to use Clerk's `auth` and `currentUser` helpers instead of `getServerSession`.

2.  **Create Clerk Page Routes:**
    -   Clerk needs specific routes to render its sign-in, sign-up, and user profile components. Create the following files:
        -   `src/app/(auth)/sign-in/[[...sign-in]]/page.tsx`
        -   `src/app/(auth)/sign-up/[[...sign-up]]/page.tsx`
        -   `src/app/user/[[...user-profile]]/page.tsx`
    -   These files will contain the corresponding Clerk components (`<SignIn />`, `<SignUp />`, `<UserProfile />`).

---

### PHASE 4: Re-implementing Desktop Authentication

**Goal:** Adapt the desktop login flow to use Clerk for authentication while still providing a custom JWT to the LibreOffice client.

**Human Actions Required:**
- In the Clerk Dashboard, navigate to **"JWT Templates"** and create a new template.
- Select the **"Blank"** template.
- Set a name (e.g., "Desktop Client Token").
- In the claims section, add the following custom claims:
  ```json
  {
    "userId": "{{user.id}}",
    "email": "{{user.primary_email_address}}",
    "type": "desktop"
  }
  ```
- Set the token lifetime (e.g., 1 hour).
- Save the template and copy its **ID**.

**Prompt Actions:**

1.  **Update `.env.local`:**
    -   Add the Clerk JWT template ID to your environment variables.
        ```env
        CLERK_JWT_TEMPLATE_ID=your_jwt_template_id
        ```

2.  **Refactor Desktop Auth API:**
    -   The `/api/desktop-init/route.ts` remains unchanged.
    -   The `/app/desktop-done/page.tsx` page will now have a Clerk session instead of a NextAuth session. Its logic will be updated to store the Clerk `userId` in the DynamoDB `desktop_login` table against the nonce.
    -   The `/api/desktop-token/route.ts` will be rewritten. When polled, it will:
        1.  Verify the `nonce` in DynamoDB.
        2.  If the `nonce` status is "ready", retrieve the associated Clerk `userId`.
        3.  Use the Clerk Backend SDK (`clerkClient.users.createSessionToken`) to generate a new JWT for that `userId` using the JWT Template created above.
        4.  Return this new JWT to the LibreOffice client.

3.  **Cleanup Old JWT Logic:**
    -   The `generateDesktopJWT` function in `src/lib/jwt.ts` can be removed. The validation logic can be kept to validate the Clerk-generated token.

---

### PHASE 5: Cleanup and Final Testing

**Goal:** Remove all remaining traces of NextAuth and perform a full end-to-end test.

**Prompt Actions:**

1.  **Delete Unused Files:**
    -   Delete `src/app/api/auth/[...nextauth]/route.ts`.
    -   Delete `src/lib/auth.ts`.
    -   Review `package.json` to ensure `next-auth` is gone.

2.  **Code Review:**
    -   Search the codebase for any remaining imports from `"next-auth"` or `"next-auth/react"` and remove them.

**Human Test:**
-   **Web Flow:**
    1.  Navigate to `http://localhost:3009`.
    2.  Click "Sign In" and verify you are taken to the Clerk-hosted sign-in page.
    3.  Sign in with Google.
    4.  Verify you are redirected to the dashboard and your user information is displayed correctly.
    5.  Test signing out.
-   **Desktop Flow:**
    1.  Simulate a `POST` request to `/api/desktop-init`.
    2.  Open the returned URL and sign in using Clerk.
    3.  Verify the "Done" page appears.
    4.  Poll the `/api/desktop-token` endpoint with the nonce.
    5.  Verify a valid JWT is returned.
    6.  Inspect the JWT (e.g., at jwt.io) to ensure it contains the correct custom claims (`userId`, `email`, `type: 'desktop'`).

This plan provides a clear path to a more robust and maintainable authentication system using Clerk, while preserving the core application logic and AWS backend. 