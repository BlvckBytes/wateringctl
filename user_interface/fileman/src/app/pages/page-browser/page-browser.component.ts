import { Component, OnDestroy } from '@angular/core';
import { Router } from '@angular/router';
import { ISortableColumn } from 'src/app/components/sortable-table/sortable-column.interface';
import { ISortableRow } from 'src/app/components/sortable-table/sortable-row.interface';
import { formatFileSize, getFileName, IFSFile } from 'src/app/models/fs-file.interface';
import { PathBarService } from 'src/app/services/path-bar.service';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';
import { SubSink } from 'subsink';
import { BrowserFileActionsComponent } from './browser-file-actions/browser-file-actions.component';
import { BrowserFileIconComponent } from './browser-file-icon/browser-file-icon.component';

@Component({
  selector: 'app-page-browser',
  templateUrl: './page-browser.component.html',
  styleUrls: ['./page-browser.component.scss']
})
export class PageBrowserComponent implements OnDestroy {

  private _subs = new SubSink();
  private _files: IFSFile[] = [];

  rows: ISortableRow[] = [];
  columns: ISortableColumn<any>[] = [
    <ISortableColumn<BrowserFileIconComponent>>{
      name: 'browser_table.columns.type', translate: true, sortable: true,
      sortFn: (a, b) => {
        if (a.isDirectory === b.isDirectory) return 0;
        return a.isDirectory ? 1 : -1;
      }
    },
    { name: 'browser_table.columns.name', translate: true, sortable: true, break: true },
    <ISortableColumn<number>>{
      name: 'browser_table.columns.size', translate: true, sortable: true,
      renderFn: (v) => formatFileSize(v),
      sortFn: (a, b) => {
        if (a == b) return 0;
        return a > b ? 1 : -1;
      }
    },
    { name: 'browser_table.columns.actions', translate: true },
  ];

  constructor(
    private pathBarService: PathBarService,
    fsService: WebSocketFsService,
    private router: Router,
  ) {
    this._subs.sink = pathBarService.path$.subscribe(path => {
      fsService.connected$.subscribe(() => {
        fsService.listDirectory(path).subscribe(files => {
          this._files = files;
          this.adaptData();
        });
      });
    });
  }

  private adaptData() {
    this.rows = this._files.map(file => (<ISortableRow>{
      clicked: () => this.fileClicked(file),
      cells: [
        { value: BrowserFileIconComponent, isValueComponent: true, inputs: { isDirectory: file.isDirectory } },
        { value: getFileName(file.name) },
        { value: file.size },
        { value: BrowserFileActionsComponent, isValueComponent: true, inputs: { file: file }},
      ]
    }));
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

  ngOnDestroy() {
    this._subs.unsubscribe();
  }
}
