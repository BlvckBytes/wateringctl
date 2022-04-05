import { Injectable } from '@angular/core';
import { BehaviorSubject, filter, Observable, take } from 'rxjs';
import { IFSFile } from '../models/fs-file.interface';
import { EWSFSResp } from '../models/ws-fs-response.enum';
import { LoadingIndicatorService } from './loading-indicator.service';

type MessageRecipient = (data: Blob) => Promise<void>;

@Injectable({
  providedIn: 'root'
})
export class WebSocketFsService {

  private _path: string;
  private _ws?: WebSocket | null = null;

  private _taskStack: number[] = [];
  private _taskTimeout = 5000;
  private _taskTimeoutWriting = 10000;

  // Current incoming message recipient callback
  private _recipient: MessageRecipient | null = null;

  // Encoder instance for sending data
  private _encoder = new TextEncoder();

  // Whether or not an active connection currently exists
  private _connected = new BehaviorSubject<boolean>(false);

  get connected$() {
    return this._connected
      .pipe(
        filter(it => it === true),
        take(1)
      );
  }

  constructor(
    private loadingService: LoadingIndicatorService,
  ) {
    this._path = 'ws://192.168.1.38:80/api/fs';

    // if (window.location.hostname !== 'localhost')
    //   this._path = `ws://${window.location.hostname}:${window.location.port}/api/fs`;
  }

  /*
  ============================================================================
                                    FETCH                                     
  ============================================================================
  */

  listDirectory(path: string): Observable<IFSFile[]> {
    return new Observable(obs => {
      this._recipient = async (data) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const jsn = JSON.parse(await data.text());
        obs.next(jsn.items as IFSFile[]);
      };

      this.send(this._encoder.encode(`FETCH;${path};true`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeout));
    });
  }

  readFile(path: string): Observable<Uint8Array> {
    return new Observable(obs => {
      let firstRecv = true;
      let recvSize: number | null = null;
      let fileConts = new Uint8Array(0);

      this._recipient = async (data) => {

        // Receive header
        if (firstRecv) {
          const header = await data.text();
          const [code, size] = header.split(';');

          // Unsuccessful
          if (code !== EWSFSResp.WSFS_FILE_FOUND) {
            obs.error(code);
            this._recipient = null;
            return;
          }

          // Cache size param
          recvSize = parseInt(size);
          firstRecv = false;
          return;
        }

        // Collect data into buffer
        const bin = new Uint8Array(await data.arrayBuffer());
        fileConts = new Uint8Array([...fileConts, ...bin]);

        // Done!
        if (fileConts.byteLength === recvSize) {
          this._recipient = null;
          this.loadingService.finishTask(this._taskStack.pop());
          obs.next(fileConts);
          return;
        }
      };

      this.send(this._encoder.encode(`FETCH;${path};false`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeout));
    });
  }

  /*
  ============================================================================
                              WRITE / OVERWRITE                               
  ============================================================================
  */

  joinPaths(a: string, b: string): string {
    const aTrailing = a.charAt(a.length - 1) == '/';
    const bLeading = b.charAt(0) == '/';

    if (aTrailing && bLeading)
      return a + b.substring(1);

    else if (!aTrailing && !bLeading)
      return a + '/' + b;
      
    return a + b;
  }

  createDirectory(path: string, directory: string): Observable<void> {
    return new Observable(obs => {
      this._recipient = async (data) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const resp = await data.text();
        if (resp == EWSFSResp.WSFS_DIR_CREATED)
          obs.next();
        else
          obs.error(resp);
      };

      this.send(this._encoder.encode(`WRITE;${this.joinPaths(path, directory)};true`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeout));
    });
  }

  writeFile(path: string, contents: string, overwrite = false): Observable<void> {
    return new Observable(obs => {
      this._recipient = async (data) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const resp = await data.text();
        if (resp == EWSFSResp.WSFS_FILE_CREATED)
          obs.next();
        else
          obs.error(resp);
      };

      this.send(this._encoder.encode(`${overwrite ? 'OVERWRITE' : 'WRITE'};${path};false;${contents}`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeoutWriting));
    });
  }

  /*
  ============================================================================
                                     DELETE                                     
  ============================================================================
  */

  deleteDirectory(path: string): Observable<void> {
    return new Observable(obs => {
      this._recipient = async (data) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const resp = await data.text();
        if (resp == EWSFSResp.WSFS_DELETED)
          obs.next();
        else
          obs.error(resp);
      };

      this.send(this._encoder.encode(`DELETE;${path};true`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeout));
    });
  }

  deleteFile(path: string): Observable<void> {
    return new Observable(obs => {
      this._recipient = async (data) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const resp = await data.text();
        if (resp == EWSFSResp.WSFS_DELETED)
          obs.next();
        else
          obs.error(resp);
      };

      this.send(this._encoder.encode(`DELETE;${path};false`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeout));
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

    const data_str = new TextDecoder().decode(data);
    const cap = 100;
    console.log('outbound:', data_str.length > cap ? data_str.substring(0, cap) : data_str);
  }

  private async onReceive(e: MessageEvent) {
    // No one's listening
    if (!this._recipient)
      return;

    // Shouldn't receive any non-binary data
    if (!(e.data instanceof Blob))
      return;

    const data_str = await e.data.text();
    const cap = 100;
    console.log(data_str.length > cap ? data_str.substring(0, cap) : data_str);

    await this._recipient(e.data);
  }

  connect(): Promise<void> {
    return new Promise((res, rej) => {
      this._ws = new WebSocket(this._path);
      this._ws.onmessage = async (e: MessageEvent) => await this.onReceive(e);

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