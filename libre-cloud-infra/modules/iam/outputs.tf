output "app_role_arn" {
  description = "ARN of the application IAM role"
  value       = aws_iam_role.app_role.arn
}

output "app_role_name" {
  description = "Name of the application IAM role"
  value       = aws_iam_role.app_role.name
}

output "s3_policy_arn" {
  description = "ARN of the S3 policy"
  value       = aws_iam_policy.s3_policy.arn
}

output "dynamodb_policy_arn" {
  description = "ARN of the DynamoDB policy"
  value       = aws_iam_policy.dynamodb_policy.arn
} 