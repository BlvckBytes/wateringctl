import { Component } from '@angular/core';
import { IFSFile } from 'src/app/models/fs-file.interface';
import { PathBarService } from 'src/app/services/path-bar.service';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';

@Component({
  selector: 'app-page-browser',
  templateUrl: './page-browser.component.html',
  styleUrls: ['./page-browser.component.scss']
})
export class PageBrowserComponent {

  files: IFSFile[] = [
    { isDirectory: false, name: '/a.txt', size: 10 },
    { isDirectory: false, name: '/b.txt', size: 1500 },
    { isDirectory: false, name: '/c.txt', size: 10238941 },
    { isDirectory: true, name: '/dir_a', size: 88182 },
    { isDirectory: true, name: '/dir_b', size: 9876781218 },
  ];

  constructor(
    private pathBarService: PathBarService,
    private fsService: WebSocketFsService,
  ) {
    pathBarService.path$.subscribe(async (path) => {
      fsService.connected$.subscribe(async (stat) => {
        if (!stat) return;
        this.files = await fsService.listDirectory(path);
      });
    });
  }

  fileClicked(file: IFSFile) {
    // Navigate into that directory
    if (file.isDirectory) {
      this.pathBarService.navigateTo(file.name);
      return;
    }

    // TODO: Open the file in the editor
  }

  getFileName(file: IFSFile): string {
    // No slash (should never occur)
    if (!file.name.includes('/'))
      return file.name;

    // Get substring after last slash
    return file.name.substring(file.name.lastIndexOf('/') + 1);
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
}
