variable "project_name" {
  description = "Name of the project"
  type        = string
}

variable "environment" {
  description = "Environment name"
  type        = string
}

variable "cognito_user_pool_id" {
  description = "Cognito User Pool ID"
  type        = string
}

variable "cognito_user_pool_client_id" {
  description = "Cognito User Pool Client ID"
  type        = string
}

variable "cognito_user_pool_domain" {
  description = "Cognito User Pool Domain"
  type        = string
}

variable "s3_bucket_name" {
  description = "S3 bucket name"
  type        = string
}

variable "documents_table_name" {
  description = "Documents DynamoDB table name"
  type        = string
}

variable "desktop_login_table_name" {
  description = "Desktop login DynamoDB table name"
  type        = string
}

variable "aws_region" {
  description = "AWS region"
  type        = string
} 