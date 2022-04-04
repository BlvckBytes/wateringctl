import { Injectable } from '@angular/core';
import { BehaviorSubject, debounceTime, delay, map, Observable } from 'rxjs';
import { ILoadingIndicatorTask } from '../models/loading-indicator-task.interface';

@Injectable({
  providedIn: 'root'
})
export class LoadingIndicatorService {

  private _taskList: ILoadingIndicatorTask[] = [];
  private _nextId = 0;

  private _active = new BehaviorSubject<boolean>(false);

  get isActive(): Observable<boolean> {
    return this._active.asObservable()
      .pipe(
        // Prevents the ExpressionChangedAfterItHasBeenCheckedError
        delay(0),
        // Prevents jittering
        debounceTime(50),
      );
  }

  startTask(timeout: number): number {
    const id = this._nextId++;
    this._taskList.push({
      timeoutHandle: setTimeout(<TimerHandler>(() => this.finishTask(id)), timeout),
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
}