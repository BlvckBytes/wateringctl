import { Component } from '@angular/core';
import { LanguageService } from './services/language.service';
import { LoadingIndicatorService } from './services/loading-indicator.service';
import { WebSocketFsService } from './services/web-socket-fs.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent {

  loading = false;

  constructor(
    loadingService: LoadingIndicatorService,
    langService: LanguageService,
    fsService: WebSocketFsService,
  ) {
    loadingService.isActive.subscribe(v => this.loading = v);
    langService.initialize();
    fsService.connect();
  }
}
