import { Component, Input } from '@angular/core';
import { IFSFile } from 'src/app/models/fs-file.interface';
import { NotificationsService } from 'src/app/services/notifications.service';
import { PathBarService } from 'src/app/services/path-bar.service';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';

@Component({
  selector: 'app-browser-file-actions',
  templateUrl: './browser-file-actions.component.html',
  styleUrls: ['./browser-file-actions.component.scss']
})
export class BrowserFileActionsComponent {

  @Input() file?: IFSFile = undefined;

  constructor(
    private notificationsService: NotificationsService,
    private pathBarService: PathBarService,
    private fsService: WebSocketFsService,
  ) {}

  private confirmedFileDeletion() {
    if (!this.file)
      return;
      
    const obs = this.file.isDirectory ? this.fsService.deleteDirectory(this.file.name) : this.fsService.deleteFile(this.file.name);
    obs.subscribe(() => this.pathBarService.refresh());
  }

  deleteFile() {
    if (!this.file)
      return;

    this.notificationsService.publish({
      headline: 'Confirm Action',
      text: 'Are you sure that you want to delete this file?',
      color: 'warning',
      icon: 'warning.svg',
      destroyOnButtons: true,
      buttons: [
        {
          text: 'Yes',
          callback: () => this.confirmedFileDeletion(),
        },
        {
          text: 'No'
        }
      ]
    });
  }
}
