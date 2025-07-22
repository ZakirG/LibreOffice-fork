#!/bin/bash

set -e

echo "🚀 Deploying LibreCloud Infrastructure - Development Environment"

# Ensure we're in the right directory
cd "$(dirname "$0")/.."

# Check if AWS credentials are set
if [ -z "$AWS_ACCESS_KEY_ID" ] || [ -z "$AWS_SECRET_ACCESS_KEY" ]; then
    echo "❌ AWS credentials not set. Please set AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY"
    exit 1
fi

echo "📋 Initializing Terraform with dev backend..."
tofu init -backend-config=environments/dev/backend.hcl -reconfigure

echo "📋 Planning infrastructure changes..."
tofu plan -var-file=environments/dev/terraform.tfvars -out=dev.tfplan

echo "🏗️  Applying infrastructure changes..."
tofu apply dev.tfplan

echo "📊 Getting outputs..."
tofu output

echo "✅ Development environment deployed successfully!"
echo ""
echo "🔧 Next steps:"
echo "1. Update your Next.js app's environment variables with the outputs above"
echo "2. Update Cognito callback URLs in AWS console if needed"
echo "3. Deploy your Next.js application to Vercel"

# Clean up plan file
rm -f dev.tfplan 