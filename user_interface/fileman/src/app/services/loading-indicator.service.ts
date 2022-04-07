import { Injectable } from '@angular/core';
import { BehaviorSubject, debounceTime, delay, map, Observable, tap } from 'rxjs';
import { ILoadingIndicatorTask } from '../models/loading-indicator-task.interface';

@Injectable({
  providedIn: 'root'
})
export class LoadingIndicatorService {

  private _taskList: ILoadingIndicatorTask[] = [];
  private _nextId = 0;

  private _active = new BehaviorSubject<boolean>(false);
  private _progress = new BehaviorSubject<number>(0);

  get progress(): Observable<number> {
    return this._progress.asObservable();
  }

  get isActive(): Observable<boolean> {
    return this._active.asObservable()
      .pipe(
        // Prevents the ExpressionChangedAfterItHasBeenCheckedError
        delay(0),
        // Prevents jittering
        debounceTime(50),

        // Reset the progress on close
        tap(it => {
          if (!it)
            this._progress.next(0);
        })
      );
  }

  startTask(timeout?: number): number {
    const id = this._nextId++;
    this._taskList.push({
      timeoutHandle: timeout === undefined ? undefined : setTimeout(<TimerHandler>(() => this.finishTask(id)), timeout),
      id,
    });
    this._active.next(true);
    return id;
  }

  finishTask(id: number | undefined) {
    if (id === undefined) return;

    const index = this._taskList.findIndex(it => it.id === id);
    if (index < 0) return;

    clearTimeout(this._taskList[index].timeoutHandle);

    this._taskList.splice(index, 1);
    this._active.next(this._taskList.length > 0);
  }

  setProgress(progress: number) {
    this._progress.next(progress);
  }
}