import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class HttpService {

  private _baseURL = 'http://192.168.1.38:80/api';
  private _wsURL = 'ws://192.168.1.38:80/wse';

  get baseURL(): string {
    return this._baseURL;
  }

  get wsURL(): string {
    return this._wsURL;
  }

  constructor(
    private httpClient: HttpClient,
  ) {
    if (window.location.hostname !== 'localhost')
    {
      const ep = `${window.location.hostname}:${window.location.port}`;
      this._baseURL = `http://${ep}/api`;
      this._wsURL = `ws://${ep}/wse`;
    }
  }

  get<R>(url: string): Observable<R> {
    return this.httpClient.get<R>(this._baseURL + url);
  }

  post<R>(url: string, body: any): Observable<R> {
    return this.httpClient.post<R>(this._baseURL + url, body);
  }

  put<R>(url: string, body: any): Observable<R> {
    return this.httpClient.put<R>(this._baseURL + url, body);
  }

  delete<R>(url: string): Observable<R> {
    return this.httpClient.delete<R>(this._baseURL + url);
  }
}
