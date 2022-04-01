import { Component } from '@angular/core';
import { LoadingIndicatorService } from './services/loading-indicator.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent {

  loading = false;

  constructor(
    loadingService: LoadingIndicatorService,
  ) {
    loadingService.isActive.subscribe(v => this.loading = v);
  }
}
