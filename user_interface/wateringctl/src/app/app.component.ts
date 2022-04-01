import { Component } from '@angular/core';
import { LoadingIndicatorService } from './services/loading-indicator.service';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.scss']
})
export class AppComponent {

  get isLoading(): boolean {
    return this.loadingService.isActive;
  }

  constructor(
    private loadingService: LoadingIndicatorService,
  ) {}
}
