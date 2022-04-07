import { Component, OnDestroy } from '@angular/core';
import { SubSink } from 'subsink';
import { LanguageService } from './services/language.service';
import { LoadingIndicatorService } from './services/loading-indicator.service';
import { WebSocketFsService } from './services/web-socket-fs.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent implements OnDestroy {

  private _subs = new SubSink();

  loading = false;
  progress = 0;

  constructor(
    loadingService: LoadingIndicatorService,
    langService: LanguageService,
    fsService: WebSocketFsService,
  ) {
    this._subs.sink = loadingService.isActive.subscribe(v => this.loading = v);
    this._subs.sink = loadingService.progress.subscribe(v => this.progress = v);

    langService.initialize();
    fsService.connect();
  }

  ngOnDestroy(): void {
    this._subs.unsubscribe();
  }
}
