# Google OAuth Setup Guide for LibreCloud

This guide walks you through setting up Google OAuth authentication for the LibreCloud project.

## 1. Create Google Cloud Project

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Click "Select a project" → "New Project"
3. Project name: `LibreCloud` (or your preferred name)
4. Click "Create"

## 2. Enable Google+ API

1. In your project, go to **APIs & Services** → **Library**
2. Search for "Google+ API" and click on it
3. Click **"Enable"**

## 3. Configure OAuth Consent Screen

1. Go to **APIs & Services** → **OAuth consent screen**
2. Choose **"External"** (for testing with any Google account)
3. Fill in required fields:
   - **App name**: `LibreCloud`
   - **User support email**: Your email
   - **Developer contact information**: Your email
4. Click **"Save and Continue"**
5. **Scopes**: Click **"Save and Continue"** (default scopes are fine)
6. **Test users**: Add your email for testing, click **"Save and Continue"**

## 4. Create OAuth Credentials

1. Go to **APIs & Services** → **Credentials**
2. Click **"+ Create Credentials"** → **"OAuth 2.0 Client IDs"**
3. Application type: **"Web application"**
4. Name: `LibreCloud Web Client`
5. **Authorized redirect URIs**, add these:
   - `http://localhost:3009/api/auth/callback/google`
   - `https://your-production-domain.vercel.app/api/auth/callback/google` (add your actual domain later)
6. Click **"Create"**
7. **IMPORTANT**: Copy the **Client ID** and **Client Secret** - you'll need these!

## 5. Update Environment Variables

Update your `libre-cloud-webapp/.env.local` file:

```bash
# Google OAuth Configuration
GOOGLE_CLIENT_ID=your_google_client_id_here.apps.googleusercontent.com
GOOGLE_CLIENT_SECRET=your_google_client_secret_here

# AWS Configuration (from tofu output)
AWS_REGION=us-east-1
AWS_ACCESS_KEY_ID=your_aws_access_key
AWS_SECRET_ACCESS_KEY=your_aws_secret_key

# S3 Configuration (from tofu output)
S3_BUCKET_NAME=your_s3_bucket_name

# DynamoDB Configuration (from tofu output)
DYNAMODB_DOCUMENTS_TABLE_NAME=your_documents_table_name
DYNAMODB_DESKTOP_LOGIN_TABLE_NAME=your_desktop_login_table_name

# NextAuth Configuration
NEXTAUTH_SECRET=Gzf7hAH9COoWI1bPtaSEL++DTiFyPsUfA4uJaJ1yfko=
NEXTAUTH_URL=http://localhost:3009
```

## 6. Apply Infrastructure Changes

Remove Cognito and keep AWS storage:

```bash
cd libre-cloud-infra

# Set AWS credentials first
export AWS_ACCESS_KEY_ID=your_access_key
export AWS_SECRET_ACCESS_KEY=your_secret_key
export AWS_REGION=us-east-1

# Plan and apply changes
tofu plan -var-file=environments/dev/terraform.tfvars
tofu apply -var-file=environments/dev/terraform.tfvars

# Get the AWS resource values for .env.local
tofu output
```

## 7. Test the Authentication Flow

1. **Start the dev server**: `npm run dev`
2. **Visit**: `http://localhost:3009`
3. **Click "Sign In"** → Should redirect to custom sign-in page
4. **Click "Sign in with Google"** → Should redirect to Google OAuth
5. **Complete Google sign-in** → Should redirect back to dashboard
6. **Verify**: Dashboard shows "Welcome back, [your-name]!"

## 8. Troubleshooting

### Common Issues:

**"redirect_uri_mismatch" error:**
- Verify the redirect URI in Google Console exactly matches: `http://localhost:3009/api/auth/callback/google`

**"access_denied" error:**
- Check that your email is added as a test user in OAuth consent screen

**Environment variables not loading:**
- Restart the dev server after changing `.env.local`
- Verify variable names are exactly uppercase

**AWS storage errors:**
- Run `tofu output` to get correct AWS resource names
- Verify AWS credentials are set correctly

## 9. Production Setup

For production deployment:

1. **Update Google OAuth**:
   - Add your production domain to authorized redirect URIs
   - Submit OAuth consent screen for verification (if going public)

2. **Update NextAuth URL**:
   ```bash
   NEXTAUTH_URL=https://your-production-domain.com
   ```

3. **Deploy infrastructure**:
   ```bash
   tofu apply -var-file=environments/prod/terraform.tfvars
   ```

## Benefits of This Setup

✅ **Simpler user experience** (everyone has Google account)  
✅ **No AWS Cognito complexity** or costs  
✅ **Same AWS storage** (S3, DynamoDB) for documents  
✅ **Same desktop integration** (JWT tokens work identically)  
✅ **Better security** (Google's OAuth infrastructure)  

Your document storage, upload, and LibreOffice integration will work exactly the same - only the authentication method has changed! 