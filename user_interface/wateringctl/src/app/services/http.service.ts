import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class HttpService {

  private baseURL = 'http://192.168.1.38:80';

  constructor(
    private httpClient: HttpClient,
  ) {}

  get<R>(url: string): Observable<R> {
    return this.httpClient.get<R>(this.baseURL + url);
  }

  post<R>(url: string, body: any): Observable<R> {
    return this.httpClient.post<R>(this.baseURL + url, body);
  }

  put<R>(url: string, body: any): Observable<R> {
    return this.httpClient.put<R>(this.baseURL + url, body);
  }

  delete<R>(url: string): Observable<R> {
    return this.httpClient.delete<R>(this.baseURL + url);
  }
}
