import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable, takeUntil, takeWhile } from 'rxjs';
import { IFSFile } from '../models/fs-file.interface';
import { EWSFSResp } from '../models/ws-fs-response.enum';
import { LoadingIndicatorService } from './loading-indicator.service';

type MessageRecipient = (data: Blob) => void;

@Injectable({
  providedIn: 'root'
})
export class WebSocketFsService {

  private _path: string;
  private _ws?: WebSocket | null = null;

  private _taskStack: number[] = [];

  // Current incoming message recipient callback
  private _recipient: MessageRecipient | null = null;

  // En- and decoder instances for sending and receiving
  private _encoder = new TextEncoder();
  private _decoder = new TextDecoder();

  // Whether or not an active connection currently exists
  private _connected = new BehaviorSubject<boolean>(false);

  get connected$() {
    return this._connected
      .pipe(
        takeWhile(it => it === false, true)
      );
  }

  constructor(
    private loadingService: LoadingIndicatorService,
  ) {
    this._path = 'ws://192.168.1.38:80/api/fs';

    if (window.location.hostname !== 'localhost')
      this._path = `ws://${window.location.hostname}:${window.location.port}/api/fs`;
  }

  /*
  ============================================================================
                                    FETCH                                     
  ============================================================================
  */

  listDirectory(path: string): Observable<IFSFile[]> {
    return new Observable(obs => {
      this._recipient = (async (data) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const jsn = JSON.parse(await data.text());
        obs.next(jsn.items as IFSFile[]);
      });

      this.send(this._encoder.encode(`FETCH;${path};true`));
      this._taskStack.push(this.loadingService.startTask(5000));
    });
  }

  /*
  ============================================================================
                                    WRITE                                     
  ============================================================================
  */

  createDirectory(path: string, directory: string): Observable<void> {
    return new Observable(obs => {
      this._recipient = (async (data) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const resp = this._decoder.decode(await data.arrayBuffer());
        if (resp == EWSFSResp.WSFS_DIR_CREATED)
          obs.next();
        else
          obs.error(resp);
      });

      this.send(this._encoder.encode(`WRITE;${path}/${directory};true`));
      this._taskStack.push(this.loadingService.startTask(5000));
    });
  }

  /*
  ============================================================================
                                     DELETE                                     
  ============================================================================
  */

  deleteDirectory(path: string): Observable<void> {
    return new Observable(obs => {
      this._recipient = (async (data) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const resp = this._decoder.decode(await data.arrayBuffer());
        if (resp == EWSFSResp.WSFS_DELETED)
          obs.next();
        else
          obs.error(resp);
      });

      this.send(this._encoder.encode(`DELETE;${path};true`));
      this._taskStack.push(this.loadingService.startTask(5000));
    });
  }

  deleteFile(path: string): Observable<void> {
    return new Observable(obs => {
      this._recipient = (async (data) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const resp = this._decoder.decode(await data.arrayBuffer());
        if (resp == EWSFSResp.WSFS_DELETED)
          obs.next();
        else
          obs.error(resp);
      });

      this.send(this._encoder.encode(`DELETE;${path};false`));
      this._taskStack.push(this.loadingService.startTask(5000));
    });
  }

  disconnect() {
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

  private send(data: Uint8Array) {
    if (!this._ws)
      return;

    this._ws.send(data);
  }

  private onReceive(e: MessageEvent) {
    // No one's listening
    if (!this._recipient)
      return;

    // Shouldn't receive any non-binary data
    if (!(e.data instanceof Blob))
      return;

    this._recipient(e.data);
  }

  connect(): Promise<void> {
    return new Promise((res, rej) => {
      this._ws = new WebSocket(this._path);
      this._ws.onmessage = (e: MessageEvent) => this.onReceive(e);

      // Connection closed, re-connect
      this._ws.onclose = () => {
        this._connected.next(false);
        this.disconnect();
        this.connect();
      }

      this._ws.onopen = () => {
        this._connected.next(true);
        res();
      };

      this._ws.onerror = () => rej();
    });
  }
}