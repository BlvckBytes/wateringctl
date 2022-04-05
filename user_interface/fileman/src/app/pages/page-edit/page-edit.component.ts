import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';
import { Location } from '@angular/common'
import * as CodeMirror from 'codemirror';

@Component({
  selector: 'app-page-edit',
  templateUrl: './page-edit.component.html',
  styleUrls: ['./page-edit.component.scss']
})
export class PageEditComponent implements OnInit {

  contents: string = '';
  fileName: string = '';

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
    this.fileName = file;

    // Set the editor-mode based on the file's extension
    const extension = this.fileName.substring(this.fileName.lastIndexOf('.') + 1);
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

  back() {
    this.loc.back();
  }
}
