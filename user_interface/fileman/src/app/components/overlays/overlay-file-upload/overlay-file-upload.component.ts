import { Component, EventEmitter, Output } from '@angular/core';
import { NgxDropzoneChangeEvent } from 'ngx-dropzone';
import { IUploadFile } from './upload-file.interface';

@Component({
  selector: 'app-overlay-file-upload',
  templateUrl: './overlay-file-upload.component.html',
  styleUrls: ['./overlay-file-upload.component.scss']
})
export class OverlayFileUploadComponent {

  @Output() uploaded = new EventEmitter<void>();
  files: IUploadFile[] = [];

  get valid(): boolean {
    return this.files
      .filter(it => it.state === 'pending')
      .length > 0;
  }

  get cancelable(): boolean {
    return this.files
      .filter(it => (
        it.state === 'queued' ||
        it.state === 'uploading'
      ))
      .length > 0;
  }

  locked = false;

  constructor() {}

  uploadFiles(): void {
    if (!this.valid)
      return;

    // Set all files into queued state
    for (const file of this.files)
      file.state = 'queued';
  }

  cancelUpload(): void {
    if (!this.cancelable)
      return;

    // Re-set file states as far as possible
    for (const file of this.files) {
      // Keep uploaded and error states
      if (
        file.state === 'uploaded' ||
        file.state === 'error'
      )
        continue;

      // Uploading file got canceled
      if (file.state === 'uploading') {
        file.state = 'canceled';
        continue;
      }

      // Re-set into pending state
      file.state = 'pending';
    }
  }

  onSelect(event: NgxDropzoneChangeEvent) {
    this.files.push(...event.addedFiles.map<IUploadFile>(f => ({
      file: f,
      state: 'pending',
    })));
  }

  onRemove(f: IUploadFile) {
    this.files.splice(this.files.indexOf(f), 1);
  }
}
