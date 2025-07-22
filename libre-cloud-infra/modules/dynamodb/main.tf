resource "aws_dynamodb_table" "documents" {
  name           = "${var.project_name}-documents-${var.environment}"
  billing_mode   = "PAY_PER_REQUEST"
  hash_key       = "userId"
  range_key      = "docId"

  attribute {
    name = "userId"
    type = "S"
  }

  attribute {
    name = "docId"
    type = "S"
  }

  # Global Secondary Index for querying by docId
  global_secondary_index {
    name               = "docId-index"
    hash_key           = "docId"
    projection_type    = "ALL"
  }

  point_in_time_recovery {
    enabled = true
  }

  server_side_encryption {
    enabled = true
  }

  tags = {
    Name        = "${var.project_name}-documents-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_dynamodb_table" "desktop_login" {
  name         = "${var.project_name}-desktop-login-${var.environment}"
  billing_mode = "PAY_PER_REQUEST"
  hash_key     = "nonce"

  attribute {
    name = "nonce"
    type = "S"
  }

  # TTL configuration for automatic expiration
  ttl {
    attribute_name = "expiresAt"
    enabled        = true
  }

  point_in_time_recovery {
    enabled = true
  }

  server_side_encryption {
    enabled = true
  }

  tags = {
    Name        = "${var.project_name}-desktop-login-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
} 