terraform {
  backend "s3" {
    # These values will be set via backend config files in environments
    bucket         = null
    key            = null
    region         = null
    dynamodb_table = null
    encrypt        = true
  }
} 