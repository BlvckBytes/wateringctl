import { HttpErrorResponse, HttpEvent, HttpHandler, HttpInterceptor, HttpRequest, HttpResponse } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { TranslateService } from '@ngx-translate/core';
import { catchError, Observable, tap, throwError } from 'rxjs';
import { LoadingIndicatorService } from '../services/loading-indicator.service';
import { NotificationsService } from '../services/notifications.service';

@Injectable()
export class ErrorHandlerHttpInterceptor implements HttpInterceptor {

  constructor(
    private notificationsService: NotificationsService,
    private tranService: TranslateService,
    private loadingService: LoadingIndicatorService,
  ) {}

  intercept(req: HttpRequest<any>, next: HttpHandler): Observable<HttpEvent<any>> {
    this.loadingService.pushTask();

    return next.handle(req).pipe(
      catchError((err: any) => {
        if (err instanceof HttpErrorResponse) {
          this.loadingService.popTask();
          const { code } = err.error;

          const key = `server_errors.${code || 'default'}`;
          let trans = this.tranService.instant(key);
          const headline = this.tranService.instant('server_errors.headline');

          if (trans == key)
            trans = this.tranService.instant('server_errors.default');

          this.notificationsService.publish({
            headline,
            text: trans,
            color: 'warning',
            timeout: 5000,
            buttons: [],
            icon: 'warning.svg'
          });
        }

        return throwError(() => err);
      }),
      tap(it => {
        if (it instanceof HttpResponse)
          this.loadingService.popTask();
      }),
    );
  }
}