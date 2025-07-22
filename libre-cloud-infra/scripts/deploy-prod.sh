#!/bin/bash

set -e

echo "🚀 Deploying LibreCloud Infrastructure - Production Environment"

# Ensure we're in the right directory
cd "$(dirname "$0")/.."

# Check if AWS credentials are set
if [ -z "$AWS_ACCESS_KEY_ID" ] || [ -z "$AWS_SECRET_ACCESS_KEY" ]; then
    echo "❌ AWS credentials not set. Please set AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY"
    exit 1
fi

# Confirmation prompt for production
echo "⚠️  You are about to deploy to PRODUCTION environment."
read -p "Are you sure you want to continue? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "❌ Deployment cancelled"
    exit 1
fi

echo "📋 Initializing Terraform with prod backend..."
tofu init -backend-config=environments/prod/backend.hcl -reconfigure

echo "📋 Planning infrastructure changes..."
tofu plan -var-file=environments/prod/terraform.tfvars -out=prod.tfplan

echo "🏗️  Applying infrastructure changes..."
tofu apply prod.tfplan

echo "📊 Getting outputs..."
tofu output

echo "✅ Production environment deployed successfully!"
echo ""
echo "🔧 Next steps:"
echo "1. Update your production environment variables with the outputs above"
echo "2. Update Cognito callback URLs in AWS console for production domain"
echo "3. Deploy your Next.js application to production"

# Clean up plan file
rm -f prod.tfplan 