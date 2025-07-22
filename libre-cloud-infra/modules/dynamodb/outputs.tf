output "documents_table_name" {
  description = "Name of the documents DynamoDB table"
  value       = aws_dynamodb_table.documents.name
}

output "documents_table_arn" {
  description = "ARN of the documents DynamoDB table"
  value       = aws_dynamodb_table.documents.arn
}

output "desktop_login_table_name" {
  description = "Name of the desktop login DynamoDB table"
  value       = aws_dynamodb_table.desktop_login.name
}

output "desktop_login_table_arn" {
  description = "ARN of the desktop login DynamoDB table"
  value       = aws_dynamodb_table.desktop_login.arn
} 