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

  locked = false;

  constructor() {}

  uploadFiles(): void {
    if (!this.valid)
      return;
  }

  cancelUpload(): void {

  }

  onSelect(event: NgxDropzoneChangeEvent) {
    this.files.push(...event.addedFiles.map<IUploadFile>(f => ({
      file: f,
      state: 'pending'
    })));
  }

  onRemove(f: IUploadFile) {
    this.files.splice(this.files.indexOf(f), 1);
  }
}
