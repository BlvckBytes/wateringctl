import { Injectable } from '@angular/core';
import { BehaviorSubject, delay, map, Observable } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class LoadingIndicatorService {

  private _tasks = new BehaviorSubject<number>(0);

  get isActive(): Observable<boolean> {
    return this._tasks.asObservable()
      .pipe(
        // Prevents the ExpressionChangedAfterItHasBeenCheckedError
        delay(0),
        map(it => it != 0),
      );
  }

  pushTask() {
    this._tasks.next(this._tasks.value + 1);
  }

  popTask() {
    this._tasks.next(this._tasks.value - 1);
  }
}