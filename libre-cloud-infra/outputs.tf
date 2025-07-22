output "cognito_user_pool_id" {
  description = "ID of the Cognito User Pool"
  value       = module.cognito.user_pool_id
}

output "cognito_user_pool_client_id" {
  description = "ID of the Cognito User Pool Client"
  value       = module.cognito.user_pool_client_id
}

output "cognito_user_pool_domain" {
  description = "Domain of the Cognito User Pool"
  value       = module.cognito.user_pool_domain
}

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