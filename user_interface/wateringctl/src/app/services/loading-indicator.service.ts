import { Injectable } from '@angular/core';
import { BehaviorSubject, debounceTime, delay, map, Observable } from 'rxjs';
import { ILoadingIndicatorTask } from '../models/loading-indicator-task.interface';

@Injectable({
  providedIn: 'root'
})
export class LoadingIndicatorService {

  private _taskList: ILoadingIndicatorTask[] = [];
  private _nextId = 0;

  private _tasks = new BehaviorSubject<boolean>(false);

  get isActive(): Observable<boolean> {
    return this._tasks.asObservable()
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
    this._tasks.next(true);
    return id;
  }

  finishTask(id: number) {
    const index = this._taskList.findIndex(it => it.id === id);
    if (index < 0) return;
    this._taskList.splice(index, 1);
    this._tasks.next(this._taskList.length > 0);
  }
}