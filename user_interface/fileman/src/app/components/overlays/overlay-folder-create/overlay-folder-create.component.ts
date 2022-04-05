import { Component, OnDestroy } from '@angular/core';
import { FormControl, Validators } from '@angular/forms';
import { KeyEventsService } from 'src/app/services/key-events.service';
import { OverlaysService } from 'src/app/services/overlays.service';
import { PathBarService } from 'src/app/services/path-bar.service';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';
import { SubSink } from 'subsink';

@Component({
  selector: 'app-overlay-folder-create',
  templateUrl: './overlay-folder-create.component.html',
  styleUrls: ['./overlay-folder-create.component.scss']
})
export class OverlayFolderCreateComponent implements OnDestroy {

  private _subs = new SubSink();
  folderName: FormControl;

  constructor(
    private pathBarService: PathBarService,
    private fsService: WebSocketFsService,
    private overlaysService: OverlaysService,
    private keysService: KeyEventsService,
  ) {
    this.folderName = new FormControl('', [Validators.required]);

    this._subs.sink = this.keysService.key$.subscribe(e => {
      if (e.key !== 'Enter') return;
      this.createFolder();
    });
  }

  ngOnDestroy(): void {
    this._subs.unsubscribe();
  }

  createFolder() {
    console.log('create folder');
    if (!this.folderName.valid)
      return;

    this.fsService.createDirectory(this.pathBarService.path, this.folderName.value)
      .subscribe(() => {
        this.pathBarService.refresh();
        this.overlaysService.destroyLatest();
      });
  }
}
