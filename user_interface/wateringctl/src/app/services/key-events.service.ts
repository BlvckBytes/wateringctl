import { Injectable } from '@angular/core';
import { Subject } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class KeyEventsService {

  private _key$ = new Subject<KeyboardEvent>();

  get key$() {
    return this._key$.asObservable();
  }

  constructor() {
    window.addEventListener('keydown', e => this._key$.next(e));
  }
}