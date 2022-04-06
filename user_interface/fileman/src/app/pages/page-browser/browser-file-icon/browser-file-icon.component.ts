import { Component, Input } from '@angular/core';

@Component({
  selector: 'app-browser-file-icon',
  templateUrl: './browser-file-icon.component.html',
  styleUrls: ['./browser-file-icon.component.scss']
})
export class BrowserFileIconComponent {

  @Input() isDirectory = false;

}
