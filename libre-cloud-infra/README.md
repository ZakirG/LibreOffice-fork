# LibreCloud Infrastructure

This directory contains the OpenTofu (Terraform) infrastructure code for the LibreCloud project.

## Architecture

The infrastructure includes:

- **Cognito User Pool**: For authentication with hosted UI and PKCE enabled
- **S3 Bucket**: Private bucket for document storage with versioning
- **DynamoDB Tables**: 
  - `documents` table (PK: userId, SK: docId)
  - `desktop_login` table (PK: nonce) with TTL
- **IAM Roles**: Minimal permissions for S3 and DynamoDB access
- **SSM Parameters**: Configuration values for the Next.js app

## Directory Structure

```
├── modules/
│   ├── cognito-user-pool/    # Cognito User Pool with hosted UI
│   ├── s3-bucket/            # S3 bucket with security settings
│   ├── dynamodb/             # DynamoDB tables
│   ├── iam/                  # IAM roles and policies
│   └── ssm-params/           # SSM Parameter Store values
├── environments/
│   ├── dev/                  # Development environment
│   └── prod/                 # Production environment
├── main.tf                   # Main configuration
├── variables.tf              # Input variables
├── outputs.tf                # Output values
└── backend.tf                # Backend configuration
```

## Prerequisites

1. AWS CLI configured with appropriate credentials
2. OpenTofu installed (https://opentofu.org/docs/intro/install/)
3. Set environment variables:
   ```bash
   export AWS_ACCESS_KEY_ID=your_key
   export AWS_SECRET_ACCESS_KEY=your_secret
   export AWS_REGION=us-east-1
   ```

## Setup Instructions

### 1. Set up Terraform state backend (one-time setup)

For development environment:
```bash
cd environments/dev
tofu init
tofu apply -target=aws_s3_bucket.terraform_state -target=aws_dynamodb_table.terraform_locks
```

For production environment:
```bash
cd environments/prod
tofu init
tofu apply -target=aws_s3_bucket.terraform_state -target=aws_dynamodb_table.terraform_locks
```

### 2. Initialize and apply the infrastructure

For development:
```bash
cd ../../  # Back to root
tofu init -backend-config=environments/dev/backend.hcl
tofu plan -var-file=environments/dev/terraform.tfvars
tofu apply -var-file=environments/dev/terraform.tfvars
```

For production:
```bash
tofu init -backend-config=environments/prod/backend.hcl
tofu plan -var-file=environments/prod/terraform.tfvars
tofu apply -var-file=environments/prod/terraform.tfvars
```

## Helper Scripts

Use the provided scripts to manage environments:

```bash
# Development
./scripts/deploy-dev.sh

# Production  
./scripts/deploy-prod.sh
```

## Outputs

After applying, the following outputs will be available:

- `cognito_user_pool_id`: Cognito User Pool ID
- `cognito_user_pool_client_id`: Cognito User Pool Client ID
- `cognito_user_pool_domain`: Cognito User Pool Domain
- `s3_bucket_name`: S3 bucket name for documents
- `dynamodb_documents_table_name`: Documents table name
- `dynamodb_desktop_login_table_name`: Desktop login table name
- `iam_role_arn`: IAM role ARN for the application

## SSM Parameters

All configuration values are stored in SSM Parameter Store with the pattern:
`/libre-cloud/{environment}/{service}/{parameter}`

Example:
- `/libre-cloud/dev/cognito/user-pool-id`
- `/libre-cloud/dev/s3/bucket-name`
- `/libre-cloud/dev/dynamodb/documents-table`

## Security

- All S3 buckets have public access blocked
- DynamoDB tables use encryption at rest
- IAM roles follow least privilege principle
- Cognito is configured with PKCE for security

## Cleanup

To destroy the infrastructure:

```bash
tofu destroy -var-file=environments/dev/terraform.tfvars
```

**Warning**: This will permanently delete all resources and data. 