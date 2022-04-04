import { Injectable } from '@angular/core';
import { BehaviorSubject } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class PathBarService {

  // Start out at root (/)
  private _path = new BehaviorSubject<string>('/');

  get path$() {
    return this._path.asObservable();
  }

  get path(): string {
    return this._path.value;
  }

  refresh() {
    this._path.next(this.path);
  }

  navigateTo(path: string) {
    // Only emit on deltas
    if (this._path.value === path)
      return;

    this._path.next(path);
  }
}