import { Injectable } from '@angular/core';

@Injectable({
  providedIn: 'root'
})
export class LoadingIndicatorService {

  private _tasks: number = 0;

  get isActive(): boolean {
    return this._tasks != 0;
  }

  pushTask() {
    this._tasks += 1;
  }

  popTask() {
    this._tasks -= 1;
  }
}