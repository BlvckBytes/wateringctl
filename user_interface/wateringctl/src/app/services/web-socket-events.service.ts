import { Injectable } from '@angular/core';
import { Subject } from 'rxjs';
import { EWebSocketEventType } from '../models/web-socket-event-type.enum';
import { IWebSocketEvent } from '../models/web-socket-event.interface';
import { HttpService } from './http.service';

@Injectable({
  providedIn: 'root'
})
export class WebSocketEventsService {

  private _path: string;
  private _ws?: WebSocket;
  private _events = new Subject<IWebSocketEvent>();
  private _connectionPoller?: number | undefined;

  get events() {
    return this._events.asObservable();
  }

  constructor(
    private httpService: HttpService,
  ) {
    this._path = `${this.httpService.baseURL.replace('http', 'ws')}/wse`;
    this.connect();
  }

  private async parseMessage(ev: MessageEvent) {
    // Read data as string, account for binary
    let data = ev.data as string;
    if (ev.data instanceof Blob)
      data = await ev.data.text();

    const parts = data.split(';', 2);

    this._events.next({
      type: parts[0] as EWebSocketEventType,
      arg: parts.length > 1 ? parts[1] : '',
    });
  }

  private connect() {
    // Stop polling while trying to connect
    if (this._connectionPoller)
      clearInterval(this._connectionPoller);

    // Close previous connection, if applicable
    if (this._ws)
      this._ws.close();

    // Create a new websocket that directly feeds into the local subject
    this._ws = new WebSocket(this._path);
    this._ws.onerror = (e: any) => this._events.error(e);
    this._ws.onmessage = (e: MessageEvent) => this.parseMessage(e);

    // Process connection timeout
    const retry = setTimeout(() => this.ensureConnection(), 5000);
    this._ws.onopen = () => {
      clearTimeout(retry);

      // Start polling for an ensured connection
      this._connectionPoller = setInterval(<TimerHandler>(() => this.ensureConnection()), 500);
    };
  }

  private ensureConnection() {
    // Reconnect if there is no active connection
    if (!this._ws || this._ws.readyState !== WebSocket.OPEN)
      this.connect();
  }
}