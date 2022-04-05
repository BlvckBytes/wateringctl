import { Component, EventEmitter, Output } from '@angular/core';
import { NgxDropzoneChangeEvent } from 'ngx-dropzone';
import { PathBarService } from 'src/app/services/path-bar.service';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';
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

  constructor(
    private fsService: WebSocketFsService,
    private pathBarService: PathBarService
  ) {}

  private uploadFile(file: IUploadFile): Promise<void> {
    return new Promise((res, rej) => {
      const reader = new FileReader();
      reader.onload = (e) => {

        // Could not read the file's contents
        if (!e.target?.result) {
          file.errorText = 'UNREADABLE'
          this.fsService.spawnFsError('file_unreadable');
          rej();
          return;
        }

        // File is about to be in uploading state
        file.state = 'uploading';

        // Call write using the content string
        this.fsService.writeFile(
          this.fsService.joinPaths(this.pathBarService.path, file.file.name),
          e.target.result as string
        ).subscribe({
          next: () => res(),
          error: (e) => {
            file.errorText = e;
            rej();
          }
        })
      }

      reader.readAsText(file.file);
    });
  }

  uploadFiles(): void {
    if (!this.valid)
      return;

    // Lock
    this.locked = true;

    // Transfer all pending files into queued state
    for (const file of this.files) {
      if (file.state == 'pending')
      file.state = 'queued';
    }

    // Upload files one by one
    (async () => {
      for (const file of this.files) {
        // Only process queued files
        if (file.state !== 'queued')
          continue;

        // Upload has been interrupped
        if (!this.locked) {
          this.resetFileStates();
          break;
        }

        try {
          await this.uploadFile(file);
          file.state = 'uploaded';
        } catch (e: any) {
          file.state = 'error';
        }
      }

      // All files done
      this.pathBarService.refresh();
      this.locked = false;
    })();
  }

  private resetFileStates() {
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

  cancelUpload(): void {
    if (this.locked)
      return;

    // Unlocking will cause the async processor to cancel at the
    // begin of the next file's upload process
    this.locked = false;
  }

  onSelect(event: NgxDropzoneChangeEvent) {
    const allowedFiles = event.addedFiles
      .filter(it => {
        if (it.size == 0)
          this.fsService.spawnFsError('file_invalid', { name: it.name });
        return it.size > 0;
      });

    this.files.push(...allowedFiles.map<IUploadFile>(f => ({
      file: f,
      state: 'pending',
    })));
  }

  onRemove(f: IUploadFile) {
    this.files.splice(this.files.indexOf(f), 1);
  }
}
