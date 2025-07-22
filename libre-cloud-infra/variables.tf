variable "environment" {
  description = "Environment name (dev/prod)"
  type        = string
  validation {
    condition     = contains(["dev", "prod"], var.environment)
    error_message = "Environment must be either 'dev' or 'prod'."
  }
}

variable "project_name" {
  description = "Name of the project"
  type        = string
  default     = "libre-cloud"
}

variable "aws_region" {
  description = "AWS region"
  type        = string
  default     = "us-east-1"
}

variable "callback_urls" {
  description = "List of callback URLs for Cognito"
  type        = list(string)
  default     = ["http://localhost:3000/api/auth/callback/cognito"]
}

variable "logout_urls" {
  description = "List of logout URLs for Cognito"
  type        = list(string)
  default     = ["http://localhost:3000"]
} 