import { getSignedUrl } from '@aws-sdk/s3-request-presigner';
import { PutObjectCommand, GetObjectCommand, DeleteObjectCommand } from '@aws-sdk/client-s3';
import { s3Client, awsConfig } from './aws';

export interface PresignedUrlOptions {
  userId: string;
  docId: string;
  fileName?: string;
  contentType?: string;
  operation: 'put' | 'get';
  expiresIn?: number; // seconds
}

export async function generatePresignedUrl({
  userId,
  docId,
  fileName,
  contentType,
  operation,
  expiresIn = 60, // 1 minute default
}: PresignedUrlOptions): Promise<string> {
  const key = `${userId}/${docId}`;

  if (operation === 'put') {
    const command = new PutObjectCommand({
      Bucket: awsConfig.s3BucketName,
      Key: key,
      ContentType: contentType,
      Metadata: {
        userId,
        docId,
        fileName: fileName || docId,
        uploadedAt: new Date().toISOString(),
      },
    });

    return await getSignedUrl(s3Client, command, { expiresIn });
  } else {
    const command = new GetObjectCommand({
      Bucket: awsConfig.s3BucketName,
      Key: key,
    });

    return await getSignedUrl(s3Client, command, { expiresIn });
  }
}

export async function deleteS3Object(userId: string, docId: string): Promise<void> {
  const key = `${userId}/${docId}`;
  
  const command = new DeleteObjectCommand({
    Bucket: awsConfig.s3BucketName,
    Key: key,
  });

  await s3Client.send(command);
}

export function getS3Key(userId: string, docId: string): string {
  return `${userId}/${docId}`;
}

export function parseS3Key(key: string): { userId: string; docId: string } | null {
  const parts = key.split('/');
  if (parts.length !== 2) return null;
  
  return {
    userId: parts[0],
    docId: parts[1],
  };
}

// Validate file types
const ALLOWED_DOCUMENT_TYPES = [
  // LibreOffice formats
  'application/vnd.oasis.opendocument.text', // .odt
  'application/vnd.oasis.opendocument.spreadsheet', // .ods
  'application/vnd.oasis.opendocument.presentation', // .odp
  'application/vnd.oasis.opendocument.graphics', // .odg
  
  // Microsoft Office formats
  'application/vnd.openxmlformats-officedocument.wordprocessingml.document', // .docx
  'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet', // .xlsx
  'application/vnd.openxmlformats-officedocument.presentationml.presentation', // .pptx
  'application/msword', // .doc
  'application/vnd.ms-excel', // .xls
  'application/vnd.ms-powerpoint', // .ppt
  
  // Other formats
  'application/pdf', // .pdf
  'text/plain', // .txt
  'text/rtf', // .rtf
  
  // Audio formats (for the music feature)
  'audio/mpeg', // .mp3
  'audio/wav', // .wav
  'audio/flac', // .flac
  'audio/ogg', // .ogg
];

export function isAllowedFileType(contentType: string): boolean {
  return ALLOWED_DOCUMENT_TYPES.includes(contentType);
}

export function getFileExtension(contentType: string): string {
  const extensionMap: Record<string, string> = {
    'application/vnd.oasis.opendocument.text': '.odt',
    'application/vnd.oasis.opendocument.spreadsheet': '.ods',
    'application/vnd.oasis.opendocument.presentation': '.odp',
    'application/vnd.oasis.opendocument.graphics': '.odg',
    'application/vnd.openxmlformats-officedocument.wordprocessingml.document': '.docx',
    'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet': '.xlsx',
    'application/vnd.openxmlformats-officedocument.presentationml.presentation': '.pptx',
    'application/msword': '.doc',
    'application/vnd.ms-excel': '.xls',
    'application/vnd.ms-powerpoint': '.ppt',
    'application/pdf': '.pdf',
    'text/plain': '.txt',
    'text/rtf': '.rtf',
    'audio/mpeg': '.mp3',
    'audio/wav': '.wav',
    'audio/flac': '.flac',
    'audio/ogg': '.ogg',
  };

  return extensionMap[contentType] || '';
} 