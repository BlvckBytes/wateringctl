import { Component } from '@angular/core';
import { FormControl, Validators } from '@angular/forms';
import { OverlaysService } from 'src/app/services/overlays.service';
import { PathBarService } from 'src/app/services/path-bar.service';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';

@Component({
  selector: 'app-overlay-folder-create',
  templateUrl: './overlay-folder-create.component.html',
  styleUrls: ['./overlay-folder-create.component.scss']
})
export class OverlayFolderCreateComponent {

  folderName: FormControl;

  constructor(
    private pathBarService: PathBarService,
    private fsService: WebSocketFsService,
    private overlaysService: OverlaysService,
  ) {
    this.folderName = new FormControl('', [Validators.required]);
  }

  createFolder() {
    if (!this.folderName.valid)
      return;

    this.fsService.createDirectory(this.pathBarService.path, this.folderName.value)
      .subscribe(() => {
        this.pathBarService.refresh();
        this.overlaysService.destroyLatest();
      });
  }
}
