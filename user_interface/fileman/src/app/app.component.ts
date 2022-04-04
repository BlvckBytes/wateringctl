import { Component } from '@angular/core';
import { LanguageService } from './services/language.service';
import { WebSocketFsService } from './services/web-socket-fs.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent {

  constructor(
    langService: LanguageService,
    fsService: WebSocketFsService,
  ) {
    langService.initialize();
    fsService.connect();
  }
}
