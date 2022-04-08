import { Component, OnDestroy } from '@angular/core';
import { FormControl, Validators } from '@angular/forms';
import { KeyEventsService } from 'src/app/services/key-events.service';
import { OverlaysService } from 'src/app/services/overlays.service';
import { PathBarService } from 'src/app/services/path-bar.service';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';
import { SubSink } from 'subsink';

@Component({
  selector: 'app-overlay-file-create',
  templateUrl: './overlay-file-create.component.html',
  styleUrls: ['./overlay-file-create.component.scss']
})
export class OverlayFileCreateComponent implements OnDestroy {

  private _subs = new SubSink();
  fileName: FormControl;

  constructor(
    private pathBarService: PathBarService,
    private fsService: WebSocketFsService,
    private overlaysService: OverlaysService,
    private keysService: KeyEventsService,
  ) {
    this.fileName = new FormControl('', [Validators.required]);

    this._subs.sink = this.keysService.key$.subscribe(e => {
      if (e.key !== 'Enter') return;
      this.createFile();
    });
  }

  ngOnDestroy(): void {
    this._subs.unsubscribe();
  }

  createFile() {
    if (!this.fileName.valid)
      return;

    const path = this.fsService.joinPaths(this.pathBarService.path, this.fileName.value);
    this.fsService.writeFile(path, false, new Blob([' ']))
      .subscribe(() => {
        this.pathBarService.refresh();
        this.overlaysService.destroyLatest();
      });
  }
}
