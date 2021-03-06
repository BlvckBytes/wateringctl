import { Injectable } from '@angular/core';
import { TranslateService } from '@ngx-translate/core';
import { BehaviorSubject, catchError, filter, Observable, Subscriber, take, throwError } from 'rxjs';
import { IFSFile } from '../models/fs-file.interface';
import { EWSFSResp } from '../models/ws-fs-response.enum';
import { LoadingIndicatorService } from './loading-indicator.service';
import { NotificationsService } from './notifications.service';

type MessageRecipient = (data: Blob, text: string) => Promise<void>;

@Injectable({
  providedIn: 'root'
})
export class WebSocketFsService {

  private _path: string;
  private _ws?: WebSocket | null = null;

  private _taskStack: number[] = [];
  private _taskTimeout = 5000;
  private _taskTimeoutWriting = 20000;

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
    private notificationsService: NotificationsService,
    private tranService: TranslateService,
  ) {
    this._path = 'ws://192.168.1.38:80/api/fs';

    // if (window.location.hostname !== 'localhost')
    //   this._path = `ws://${window.location.hostname}:${window.location.port}/api/fs`;
  }


  /*
  ============================================================================
                             Request Interceptor                              
  ============================================================================
  */

  spawnFsError(error: string, params?: any) {
    const headline = this.tranService.instant("fs_resp_err.headline");
    const text_key = `fs_resp_err.${error}`;
    let text = this.tranService.instant(text_key, params);

    if (text === text_key)
      text = this.tranService.instant("fs_resp_err.default");

    this.notificationsService.publish({
      headline,
      text,
      icon: 'warning.svg',
      color: 'warning',
      timeout: 2500,
    });
  }

  private interceptedResponse<T>(obs: (sub: Subscriber<T>) => void): Observable<T> {
    return new Observable(obs)
      .pipe(
        catchError<any, any>(e => {
          const ed = Array.isArray(e) ? e : [e];
          this.spawnFsError(ed[0], { detail: ed.slice(1) });
          return throwError(() => e);
        })
      );
  }

  spawnFsSuccess(success: string, params?: any) {
    const headline = this.tranService.instant("fs_resp_succ.headline");
    const text_key = `fs_resp_succ.${success}`;
    let text = this.tranService.instant(text_key, params);

    if (text === text_key)
      text = this.tranService.instant("fs_resp_succ.default");

    this.notificationsService.publish({
      headline,
      text,
      icon: 'tick.svg',
      color: 'success',
      timeout: 2500,
    });
  }

  /*
  ============================================================================
                                    UNTAR                                     
  ============================================================================
  */

  untar(path: string): Observable<void> {
    return this.interceptedResponse(sub => {
      this._recipient = async (_, resp) => {
        this.loadingService.finishTask(this._taskStack.pop());

        const res = resp.split(';');
        if (res[0] === EWSFSResp.WSFS_UNTARED)
          this.spawnFsSuccess(res[0]);
        else
          sub.error(res);

        this._recipient = null;
        sub.next();
      };

      this.send(this._encoder.encode(`UNTAR;${path}`));
      this._taskStack.push(this.loadingService.startTask());
      this.loadingService.setProgress(1);
    });
  }

  /*
  ============================================================================
                                    FETCH                                     
  ============================================================================
  */

  listDirectory(path: string): Observable<IFSFile[]> {
    return this.interceptedResponse(sub => {
      this._recipient = async (_, resp) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        const jsn = JSON.parse(resp);
        sub.next(jsn.items as IFSFile[]);
      };

      this.send(this._encoder.encode(`FETCH;${path};true`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeout));
    });
  }

  readFile(path: string): Observable<Uint8Array> {
    return this.interceptedResponse(sub => {
      let firstRecv = true;
      let recvSize: number | null = null;
      let fileConts = new Uint8Array(0);

      this._recipient = async (data, resp) => {

        // Receive header
        if (firstRecv) {
          const [code, size] = resp.split(';');

          // Unsuccessful
          if (code !== EWSFSResp.WSFS_FILE_FOUND) {
            sub.error(code);
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
          sub.next(fileConts);
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
    return this.interceptedResponse(sub => {
      this._recipient = async (_, resp) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        if (resp == EWSFSResp.WSFS_DIR_CREATED) {
          this.spawnFsSuccess(resp);
          sub.next();
        } else
          sub.error(resp);
      };

      this.send(this._encoder.encode(`WRITE;${this.joinPaths(path, directory)};true`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeout));
    });
  }

  writeFile(path: string, overwrite = false, file: Blob): Observable<void> {
    return this.interceptedResponse(sub => {

      const size = file.size;
      const sliceSize = 1024;
      const slices = Math.ceil(size / sliceSize);

      const sendNextSlice = async () => {
        // Get the next slice
        const currSize = currSlice === slices - 1 ? size % sliceSize : sliceSize;
        const currOffs = currSlice * sliceSize;
        const sliceData = await file.slice(currOffs, currOffs + currSize).arrayBuffer();

        // Send the slice
        this.send(new Uint8Array(sliceData));
        this.loadingService.setProgress(Math.floor((currSlice + 1) / slices * 100));

        // Return true if all slices have been sent
        return ++currSlice == slices;
      };

      let done = false;
      let currSlice = 0;
      this._recipient = async (_, resp) => {
        
        // Send the first slice after the file itself has been created
        if (resp == EWSFSResp.WSFS_FILE_CREATED)
          done = await sendNextSlice();
        
        // Iterate through all slices
        else if (resp == EWSFSResp.WSFS_FILE_APPENDED) {
          if (!done)
            done = await sendNextSlice();

          // Done
          else {
            this.spawnFsSuccess(EWSFSResp.WSFS_FILE_APPENDED);
            this.loadingService.finishTask(this._taskStack.pop());
            sub.next();
          }
        }

        // Non-success response
        else {
          this.loadingService.finishTask(this._taskStack.pop());
          sub.error(resp);
        }
      };

      this.send(this._encoder.encode(`${overwrite ? 'OVERWRITE' : 'WRITE'};${path};false;${size}`));
      this._taskStack.push(this.loadingService.startTask());
    });
  }

  /*
  ============================================================================
                                     DELETE                                     
  ============================================================================
  */

  deleteDirectory(path: string): Observable<void> {
    return this.interceptedResponse(sub => {
      this._recipient = async (_, resp) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        if (resp == EWSFSResp.WSFS_DELETED) {
          this.spawnFsSuccess(resp);
          sub.next();
        } else
          sub.error(resp);
      };

      this.send(this._encoder.encode(`DELETE;${path};true`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeout));
    });
  }

  deleteFile(path: string): Observable<void> {
    return this.interceptedResponse(sub => {
      this._recipient = async (_, resp) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        if (resp == EWSFSResp.WSFS_DELETED) {
          this.spawnFsSuccess(resp);
          sub.next();
        } else
          sub.error(resp);
      };

      this.send(this._encoder.encode(`DELETE;${path};false`));
      this._taskStack.push(this.loadingService.startTask(this._taskTimeout));
    });
  }

  /*
  ============================================================================
                                    UPDATE                                    
  ============================================================================
  */

  updateFirmware(path: string): Observable<void> {
    return this.interceptedResponse(sub => {
      this._recipient = async (_, resp) => {
        this.loadingService.finishTask(this._taskStack.pop());

        this._recipient = null;
        if (resp == EWSFSResp.WSFS_UPDATED) {
          this.spawnFsSuccess(resp);
          sub.next();
        } else
          sub.error(resp);
      };

      this.send(this._encoder.encode(`UPDATE;${path}`));
      this._taskStack.push(this.loadingService.startTask());
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
    console.log('inbound:', data_str.length > cap ? data_str.substring(0, cap) : data_str);

    // Is a progress response, catch here and forward to the overlay
    if (data_str.startsWith(EWSFSResp.WSFS_PROGRESS)) {
      const [_, progress] = data_str.split(';');
      this.loadingService.setProgress(Number.parseInt(progress));
      return;
    }

    await this._recipient(e.data, data_str);
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