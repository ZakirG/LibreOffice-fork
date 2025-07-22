variable "project_name" {
  description = "Name of the project"
  type        = string
}

variable "environment" {
  description = "Environment name"
  type        = string
}

variable "s3_bucket_arn" {
  description = "ARN of the S3 bucket"
  type        = string
}

variable "documents_table_arn" {
  description = "ARN of the documents DynamoDB table"
  type        = string
}

variable "desktop_login_table_arn" {
  description = "ARN of the desktop login DynamoDB table"
  type        = string
} 