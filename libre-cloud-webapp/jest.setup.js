// Optional: configure or set up a testing framework before each test.
// If you delete this file, remove `setupFilesAfterEnv` from `jest.config.js`

// Used for __tests__/testing-library.js
// Learn more: https://github.com/testing-library/jest-dom
import '@testing-library/jest-dom'

// Mock environment variables for testing
process.env.NODE_ENV = 'test'
process.env.AWS_REGION = 'us-east-1'
process.env.AWS_ACCESS_KEY_ID = 'test-access-key'
process.env.AWS_SECRET_ACCESS_KEY = 'test-secret-key'
process.env.DYNAMODB_DOCUMENTS_TABLE_NAME = 'test-documents'
process.env.DYNAMODB_DESKTOP_LOGIN_TABLE_NAME = 'test-desktop-login'
process.env.NEXTAUTH_URL = 'http://localhost:3009'
process.env.NEXTAUTH_SECRET = 'test-secret' 