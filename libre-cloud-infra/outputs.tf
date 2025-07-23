# Cognito outputs removed - using Google OAuth instead

output "s3_bucket_name" {
  description = "Name of the S3 bucket for documents"
  value       = module.s3.bucket_name
}

output "s3_bucket_arn" {
  description = "ARN of the S3 bucket for documents"
  value       = module.s3.bucket_arn
}

output "dynamodb_documents_table_name" {
  description = "Name of the DynamoDB documents table"
  value       = module.dynamodb.documents_table_name
}

output "dynamodb_desktop_login_table_name" {
  description = "Name of the DynamoDB desktop login table"
  value       = module.dynamodb.desktop_login_table_name
}

output "iam_role_arn" {
  description = "ARN of the IAM role for the application"
  value       = module.iam.app_role_arn
}

output "aws_region" {
  description = "AWS region"
  value       = var.aws_region
} 