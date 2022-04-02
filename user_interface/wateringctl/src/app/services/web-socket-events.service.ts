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

  private _connTestStr = '<conn_test>';
  private _connTestTimeout?: number | undefined;
  private _connectionPoller?: number | undefined;
  private _connTimeout = 1500;

  get events() {
    return this._events.asObservable();
  }

  constructor(
    private httpService: HttpService,
  ) {
    const url = this.httpService.baseURL
      .replace('https', 'ws')
      .replace('http', 'ws');

    this._path = `${url}/wse`;
    this.connect();

    this._events.subscribe(v => console.log(v));
  }

  private async parseMessage(ev: MessageEvent) {
    // Read data as string, account for binary
    let data = ev.data as string;
    if (ev.data instanceof Blob)
      data = await ev.data.text();

    // Connection test response, clear timeout
    if (data === this._connTestStr) {
      clearInterval(this._connTestTimeout);

      // Re-test again
      this._connectionPoller = setTimeout(<TimerHandler>(() => this.ensureConnection()), this._connTimeout);
      return;
    }

    const delim = data.indexOf(';');
    const type = data.substring(0, delim);
    const args = data.substring(delim + 1).split(';');

    this._events.next({
      type: type as EWebSocketEventType,
      args,
    });
  }

  private connect() {
    // Stop polling while trying to connect
    if (this._connectionPoller)
      clearTimeout(this._connectionPoller);

    // Close previous connection, if applicable
    if (this._ws)
      this._ws.close();

    // Connection timeout
    const retry = setTimeout(() => this.connect(), this._connTimeout);

    // Create a new websocket that directly feeds into the local subject
    this._ws = new WebSocket(this._path);
    this._ws.onmessage = (e: MessageEvent) => this.parseMessage(e);

    this._ws.onopen = () => {
      clearTimeout(retry);
      this.ensureConnection();
    };
  }

  private ensureConnection() {
    // Reconnect if there is no active connection
    if (!this._ws || this._ws.readyState !== WebSocket.OPEN)
    {
      this.connect();
      return;
    }

    this._ws.send(this._connTestStr);
    this._connTestTimeout = setTimeout(<TimerHandler>(() => this.connect()), this._connTimeout);
  }
}