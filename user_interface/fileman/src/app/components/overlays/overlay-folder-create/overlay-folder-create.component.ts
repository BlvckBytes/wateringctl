import { Component, EventEmitter, Output } from '@angular/core';
import { FormControl, Validators } from '@angular/forms';

@Component({
  selector: 'app-overlay-folder-create',
  templateUrl: './overlay-folder-create.component.html',
  styleUrls: ['./overlay-folder-create.component.scss']
})
export class OverlayFolderCreateComponent {

  @Output() created = new EventEmitter<void>();
  folderName: FormControl;

  constructor() {
    this.folderName = new FormControl('', [Validators.required]);
  }

  createFolder() {
    if (!this.folderName.valid)
      return;

    // TODO: Actually create the folder
    this.created.next();
  }
}
