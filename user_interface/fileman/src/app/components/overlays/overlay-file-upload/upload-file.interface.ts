export interface IUploadFile {
  file: File;
  state: 'pending' | 'queued' | 'uploading' | 'uploaded' | 'error' | 'canceled';
  errorText?: string | undefined;
}