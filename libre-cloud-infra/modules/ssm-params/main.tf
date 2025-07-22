resource "aws_ssm_parameter" "cognito_user_pool_id" {
  name  = "/${var.project_name}/${var.environment}/cognito/user-pool-id"
  type  = "String"
  value = var.cognito_user_pool_id

  tags = {
    Name        = "${var.project_name}-cognito-user-pool-id-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_ssm_parameter" "cognito_user_pool_client_id" {
  name  = "/${var.project_name}/${var.environment}/cognito/client-id"
  type  = "String"
  value = var.cognito_user_pool_client_id

  tags = {
    Name        = "${var.project_name}-cognito-client-id-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_ssm_parameter" "cognito_user_pool_domain" {
  name  = "/${var.project_name}/${var.environment}/cognito/domain"
  type  = "String"
  value = var.cognito_user_pool_domain

  tags = {
    Name        = "${var.project_name}-cognito-domain-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_ssm_parameter" "s3_bucket_name" {
  name  = "/${var.project_name}/${var.environment}/s3/bucket-name"
  type  = "String"
  value = var.s3_bucket_name

  tags = {
    Name        = "${var.project_name}-s3-bucket-name-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_ssm_parameter" "documents_table_name" {
  name  = "/${var.project_name}/${var.environment}/dynamodb/documents-table"
  type  = "String"
  value = var.documents_table_name

  tags = {
    Name        = "${var.project_name}-documents-table-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_ssm_parameter" "desktop_login_table_name" {
  name  = "/${var.project_name}/${var.environment}/dynamodb/desktop-login-table"
  type  = "String"
  value = var.desktop_login_table_name

  tags = {
    Name        = "${var.project_name}-desktop-login-table-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_ssm_parameter" "aws_region" {
  name  = "/${var.project_name}/${var.environment}/aws/region"
  type  = "String"
  value = var.aws_region

  tags = {
    Name        = "${var.project_name}-aws-region-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
} 