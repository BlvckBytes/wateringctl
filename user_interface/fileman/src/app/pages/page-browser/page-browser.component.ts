import { Component, OnDestroy } from '@angular/core';
import { Router } from '@angular/router';
import { getFileName, IFSFile } from 'src/app/models/fs-file.interface';
import { NotificationsService } from 'src/app/services/notifications.service';
import { PathBarService } from 'src/app/services/path-bar.service';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';
import { SubSink } from 'subsink';

@Component({
  selector: 'app-page-browser',
  templateUrl: './page-browser.component.html',
  styleUrls: ['./page-browser.component.scss']
})
export class PageBrowserComponent implements OnDestroy {

  private _subs = new SubSink();

  files: IFSFile[] = [];

  constructor(
    private pathBarService: PathBarService,
    private fsService: WebSocketFsService,
    private router: Router,
    private notificationsService: NotificationsService,
  ) {
    this._subs.sink = pathBarService.path$.subscribe(path => {
      fsService.connected$.subscribe(() => {
        fsService.listDirectory(path).subscribe(files => this.files = files);
      });
    });
  }

  private confirmedFileDeletion(file: IFSFile) {
    const obs = file.isDirectory ? this.fsService.deleteDirectory(file.name) : this.fsService.deleteFile(file.name);
    obs.subscribe(() => this.pathBarService.refresh());
  }

  deleteFile(file: IFSFile) {
    this.notificationsService.publish({
      headline: 'Confirm Action',
      text: 'Are you sure that you want to delete this file?',
      color: 'warning',
      icon: 'warning.svg',
      destroyOnButtons: true,
      buttons: [
        {
          text: 'Yes',
          callback: () => this.confirmedFileDeletion(file),
        },
        {
          text: 'No'
        }
      ]
    });
  }

  fileClicked(file: IFSFile) {
    // Navigate into that directory
    if (file.isDirectory) {
      this.pathBarService.navigateTo(file.name);
      return;
    }

    // Edit file
    this.router.navigate(['/edit', file.name]);
  }

  fileName(file: IFSFile) {
    return getFileName(file.name);
  }

  formatFileSize(size: number): string {
    const gib = size / (1024 ** 3);
    if (gib >= 1)
      return `${Math.round(gib * 100) / 100} GiB`;

    const mib = size / (1024 ** 2);
    if (mib >= 1)
      return `${Math.round(mib * 100) / 100} MiB`;

    const kib = size / (1024);
    if (kib >= 1)
      return `${Math.round(kib * 100) / 100} KiB`;

    return `${size} Bytes`;
  }

  ngOnDestroy() {
    this._subs.unsubscribe();
  }
}
