import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';
import { Location } from '@angular/common'
import * as CodeMirror from 'codemirror';
import { getFileName } from 'src/app/models/fs-file.interface';

@Component({
  selector: 'app-page-edit',
  templateUrl: './page-edit.component.html',
  styleUrls: ['./page-edit.component.scss']
})
export class PageEditComponent implements OnInit {

  contents: string = '';
  saveable = false;

  private _fileName: string = '';
  get fileName(): string {
    return getFileName(this._fileName);
  }

  editorOptions = {
    lineNumbers: true,
    lineWrapping: true,
    mode: '',
    theme: 'material',
  };

  constructor(
    private route: ActivatedRoute,
    private loc: Location,
    private fsService: WebSocketFsService,
  ) {}

  private loadContents(file: string) {
    // Keep name for displaying
    this._fileName = file;

    // Set the editor-mode based on the file's extension
    const extension = this._fileName.substring(this._fileName.lastIndexOf('.') + 1);
    this.editorOptions.mode = CodeMirror.findModeByExtension(extension)?.mode || '';

    // Load file contents
    this.fsService.readFile(file).subscribe(bin => {
      this.contents = new TextDecoder().decode(bin);
    });
  }
  
  ngOnInit(): void {
    this.route.params.subscribe(params => {
      this.fsService.connected$.subscribe(() => this.loadContents(params['name']));
    });
  }

  contentChanged(value: string) {
    this.contents = value;
    this.saveable = true;
  }

  back() {
    this.loc.back();
  }

  save() {
    if (!this.saveable)
      return;
    
    this.fsService.writeFile(this._fileName, this.contents, true).subscribe(() => {
      this.saveable = false;
    });
  }
}
