import { Component } from '@angular/core';
import { FormControl, Validators } from '@angular/forms';
import { OverlaysService } from 'src/app/services/overlays.service';
import { PathBarService } from 'src/app/services/path-bar.service';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';

@Component({
  selector: 'app-overlay-file-create',
  templateUrl: './overlay-file-create.component.html',
  styleUrls: ['./overlay-file-create.component.scss']
})
export class OverlayFileCreateComponent {

  fileName: FormControl;

  constructor(
    private pathBarService: PathBarService,
    private fsService: WebSocketFsService,
    private overlaysService: OverlaysService,
  ) {
    this.fileName = new FormControl('', [Validators.required]);
  }

  createFile() {
    if (!this.fileName.valid)
      return;

    const path = this.fsService.joinPaths(this.pathBarService.path, this.fileName.value);
    this.fsService.writeFile(path, ' ', false)
      .subscribe(() => {
        this.pathBarService.refresh();
        this.overlaysService.destroyLatest();
      });
  }
}
