import { HttpErrorResponse, HttpEvent, HttpHandler, HttpInterceptor, HttpRequest } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { TranslateService } from '@ngx-translate/core';
import { catchError, Observable, throwError } from 'rxjs';
import { NotificationsService } from '../services/notifications.service';

@Injectable()
export class ErrorHandlerHttpInterceptor implements HttpInterceptor {

  constructor(
    private notificationsService: NotificationsService,
    private tranService: TranslateService,
  ) {}

  intercept(req: HttpRequest<any>, next: HttpHandler): Observable<HttpEvent<any>> {
    return next.handle(req).pipe(
      catchError((err: any) => {
        if (err instanceof HttpErrorResponse) {
          const { code } = err.error;
          if (!code) return throwError(() => err);

          const key = `server_errors.${code}`;
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
      })
    );
  }
}