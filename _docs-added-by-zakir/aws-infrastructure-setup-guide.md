# AWS Infrastructure Setup Guide

This guide will walk you through setting up the LibreCloud AWS infrastructure using OpenTofu.

## Prerequisites

1. **AWS Account**: You need an AWS account with programmatic access
2. **AWS Credentials**: Access Key ID and Secret Access Key
3. **OpenTofu Installed**: Download from https://opentofu.org/docs/intro/install/

## Step 1: Set AWS Credentials

Before running any commands, you need to set your AWS credentials as environment variables in your terminal:

```bash
export AWS_ACCESS_KEY_ID=your_actual_access_key_id
export AWS_SECRET_ACCESS_KEY=your_actual_secret_access_key
export AWS_REGION=us-east-1
```

**Important**: Replace the placeholder values with your actual AWS credentials.

### Verify Credentials Are Set

Run this command to verify your credentials are properly set:

```bash
echo "AWS_ACCESS_KEY_ID: ${AWS_ACCESS_KEY_ID:+SET}"
echo "AWS_SECRET_ACCESS_KEY: ${AWS_SECRET_ACCESS_KEY:+SET}" 
echo "AWS_REGION: ${AWS_REGION:-NOT_SET}"
```

You should see:
```
AWS_ACCESS_KEY_ID: SET
AWS_SECRET_ACCESS_KEY: SET
AWS_REGION: us-east-1
```

## Step 2: Set Up Terraform State Backend (One-Time Setup)

Before we can use the main infrastructure configuration, we need to create the S3 bucket and DynamoDB table for storing Terraform state.

### Navigate to the Dev Environment

```bash
cd libre-cloud-infra/environments/dev
```

### Initialize and Create State Backend Resources

```bash
# Initialize the state setup configuration
tofu init

# Create only the S3 bucket and DynamoDB table for state storage
tofu apply -target=aws_s3_bucket.terraform_state -target=aws_dynamodb_table.terraform_locks
```

When prompted, type `yes` to create the resources.

This will create:
- S3 bucket: `libre-cloud-terraform-state-dev`
- DynamoDB table: `libre-cloud-terraform-locks-dev`

## Step 3: Initialize Main Infrastructure

Now go back to the root directory and initialize with the proper backend configuration:

```bash
# Go back to the root infrastructure directory
cd ../../

# Initialize with the backend configuration
tofu init -backend-config=environments/dev/backend.hcl
```

## Step 4: Plan and Apply Infrastructure

```bash
# Create a plan
tofu plan -var-file=environments/dev/terraform.tfvars

# Apply the infrastructure (creates all AWS resources)
tofu apply -var-file=environments/dev/terraform.tfvars
```

When prompted, type `yes` to create all the resources.

## What Gets Created

After successful deployment, you'll have:

### Cognito
- User Pool with hosted UI
- App Client configured for PKCE
- Domain for hosted UI

### S3
- Private bucket for document storage
- Versioning enabled
- Proper security settings

### DynamoDB
- `documents` table (PK: userId, SK: docId)
- `desktop_login` table (PK: nonce) with TTL

### IAM
- Application role with minimal permissions
- Policies for S3 and DynamoDB access

### SSM Parameters
- All configuration values stored securely
- Can be retrieved for Next.js app configuration

## Step 5: Get Outputs for Next.js App

After successful deployment, get the outputs:

```bash
tofu output
```

Copy these values to update your Next.js app's environment variables in `libre-cloud-webapp/.env.local`.

## Troubleshooting

### Error: No valid credential sources found
- Make sure you've set the AWS environment variables in the same terminal session
- Verify the credentials with the verification command above

### Error: Invalid key value / Missing region value
- This happens when you try to run `tofu init` without the backend config
- Always use: `tofu init -backend-config=environments/dev/backend.hcl`

### Error: S3 bucket already exists
- The bucket names include random suffixes to avoid conflicts
- If you still get this error, check the bucket name in the error message

## Clean Up (Optional)

To destroy all resources:

```bash
# Destroy main infrastructure
tofu destroy -var-file=environments/dev/terraform.tfvars

# Clean up state backend (only if you want to completely start over)
cd environments/dev
tofu destroy
```

**Warning**: This will permanently delete all resources and data.

## Production Deployment

For production, use the same steps but with the prod environment:

```bash
# Set up prod state backend
cd libre-cloud-infra/environments/prod
tofu init
tofu apply -target=aws_s3_bucket.terraform_state -target=aws_dynamodb_table.terraform_locks

# Deploy prod infrastructure
cd ../../
tofu init -backend-config=environments/prod/backend.hcl
tofu plan -var-file=environments/prod/terraform.tfvars
tofu apply -var-file=environments/prod/terraform.tfvars
```

Make sure to update the callback URLs in `environments/prod/terraform.tfvars` with your actual production domain. 