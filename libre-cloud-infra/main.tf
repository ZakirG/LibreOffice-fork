provider "aws" {
  region = var.aws_region
}

# Cognito User Pool Module
module "cognito" {
  source = "./modules/cognito-user-pool"
  
  project_name  = var.project_name
  environment   = var.environment
  callback_urls = var.callback_urls
  logout_urls   = var.logout_urls
}

# S3 Bucket Module
module "s3" {
  source = "./modules/s3-bucket"
  
  project_name = var.project_name
  environment  = var.environment
}

# DynamoDB Module
module "dynamodb" {
  source = "./modules/dynamodb"
  
  project_name = var.project_name
  environment  = var.environment
}

# IAM Module
module "iam" {
  source = "./modules/iam"
  
  project_name             = var.project_name
  environment              = var.environment
  s3_bucket_arn           = module.s3.bucket_arn
  documents_table_arn     = module.dynamodb.documents_table_arn
  desktop_login_table_arn = module.dynamodb.desktop_login_table_arn
}

# SSM Parameters Module
module "ssm_params" {
  source = "./modules/ssm-params"
  
  project_name                = var.project_name
  environment                 = var.environment
  cognito_user_pool_id       = module.cognito.user_pool_id
  cognito_user_pool_client_id = module.cognito.user_pool_client_id
  cognito_user_pool_domain   = module.cognito.user_pool_domain
  s3_bucket_name             = module.s3.bucket_name
  documents_table_name       = module.dynamodb.documents_table_name
  desktop_login_table_name   = module.dynamodb.desktop_login_table_name
  aws_region                 = var.aws_region
} 