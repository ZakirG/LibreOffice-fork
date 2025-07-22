data "aws_iam_policy_document" "app_assume_role" {
  statement {
    actions = ["sts:AssumeRole"]

    principals {
      type        = "Service"
      identifiers = ["lambda.amazonaws.com", "ecs-tasks.amazonaws.com"]
    }

    principals {
      type        = "AWS"
      identifiers = ["*"]
    }

    condition {
      test     = "StringEquals"
      variable = "aws:RequestedRegion"
      values   = [data.aws_region.current.name]
    }
  }
}

data "aws_iam_policy_document" "s3_policy" {
  statement {
    sid    = "AllowS3GetPutDelete"
    effect = "Allow"
    
    actions = [
      "s3:GetObject",
      "s3:PutObject",
      "s3:DeleteObject",
      "s3:GetObjectVersion",
      "s3:DeleteObjectVersion"
    ]
    
    resources = [
      "${var.s3_bucket_arn}/*"
    ]
  }

  statement {
    sid    = "AllowS3ListBucket"
    effect = "Allow"
    
    actions = [
      "s3:ListBucket",
      "s3:GetBucketLocation"
    ]
    
    resources = [
      var.s3_bucket_arn
    ]
  }
}

data "aws_iam_policy_document" "dynamodb_policy" {
  statement {
    sid    = "AllowDynamoDBAccess"
    effect = "Allow"
    
    actions = [
      "dynamodb:GetItem",
      "dynamodb:PutItem",
      "dynamodb:UpdateItem",
      "dynamodb:DeleteItem",
      "dynamodb:Query",
      "dynamodb:Scan"
    ]
    
    resources = [
      var.documents_table_arn,
      "${var.documents_table_arn}/index/*",
      var.desktop_login_table_arn,
      "${var.desktop_login_table_arn}/index/*"
    ]
  }
}

resource "aws_iam_role" "app_role" {
  name               = "${var.project_name}-app-role-${var.environment}"
  assume_role_policy = data.aws_iam_policy_document.app_assume_role.json

  tags = {
    Name        = "${var.project_name}-app-role-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_iam_policy" "s3_policy" {
  name        = "${var.project_name}-s3-policy-${var.environment}"
  description = "Policy for S3 access to documents bucket"
  policy      = data.aws_iam_policy_document.s3_policy.json

  tags = {
    Name        = "${var.project_name}-s3-policy-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_iam_policy" "dynamodb_policy" {
  name        = "${var.project_name}-dynamodb-policy-${var.environment}"
  description = "Policy for DynamoDB access to application tables"
  policy      = data.aws_iam_policy_document.dynamodb_policy.json

  tags = {
    Name        = "${var.project_name}-dynamodb-policy-${var.environment}"
    Environment = var.environment
    Project     = var.project_name
  }
}

resource "aws_iam_role_policy_attachment" "app_role_s3" {
  role       = aws_iam_role.app_role.name
  policy_arn = aws_iam_policy.s3_policy.arn
}

resource "aws_iam_role_policy_attachment" "app_role_dynamodb" {
  role       = aws_iam_role.app_role.name
  policy_arn = aws_iam_policy.dynamodb_policy.arn
}

data "aws_region" "current" {} 