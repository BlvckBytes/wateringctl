export interface IUploadFile {
  file: File;
  state: 'pending' | 'queued' | 'uploading' | 'uploaded';
}