output "parameter_names" {
  description = "List of all SSM parameter names"
  value = [
    aws_ssm_parameter.cognito_user_pool_id.name,
    aws_ssm_parameter.cognito_user_pool_client_id.name,
    aws_ssm_parameter.cognito_user_pool_domain.name,
    aws_ssm_parameter.s3_bucket_name.name,
    aws_ssm_parameter.documents_table_name.name,
    aws_ssm_parameter.desktop_login_table_name.name,
    aws_ssm_parameter.aws_region.name,
  ]
}

output "parameter_arns" {
  description = "Map of parameter names to ARNs"
  value = {
    cognito_user_pool_id       = aws_ssm_parameter.cognito_user_pool_id.arn
    cognito_user_pool_client_id = aws_ssm_parameter.cognito_user_pool_client_id.arn
    cognito_user_pool_domain   = aws_ssm_parameter.cognito_user_pool_domain.arn
    s3_bucket_name            = aws_ssm_parameter.s3_bucket_name.arn
    documents_table_name      = aws_ssm_parameter.documents_table_name.arn
    desktop_login_table_name  = aws_ssm_parameter.desktop_login_table_name.arn
    aws_region               = aws_ssm_parameter.aws_region.arn
  }
} 