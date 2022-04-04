import { Injectable } from '@angular/core';

@Injectable({
  providedIn: 'root'
})
export class WebSocketFsService {

  private _path: string;
  private _ws?: WebSocket;

  constructor(
  ) {
    this._path = ''; // TODO: Implement path from window.url generator
    this.connect();
  }

  private disconnect() {
    // Clear out callback bindings
    if (this._ws)
    {
      this._ws.onclose = null;
      this._ws.onopen = null;
      this._ws.onmessage = null;
      this._ws.onerror = null;
    }

    // Clear socket handle
    this._ws?.close();
    this._ws = undefined;
  }

  private onReceive(e: MessageEvent) {

  }

  private connect() {
    this._ws = new WebSocket(this._path);
    this._ws.onmessage = (e: MessageEvent) => this.onReceive(e);

    // Connection closed
    this._ws.onclose = () => {
      this.disconnect();
      this.connect();
    }

    this._ws.onopen = () => {};
  }
}