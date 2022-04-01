import { Injectable } from '@angular/core';
import { BehaviorSubject } from 'rxjs';
import { INotification } from '../models/notification.interface';

@Injectable({
  providedIn: 'root'
})
export class NotificationsService {

  private items: INotification[] = [];
  private _items$ = new BehaviorSubject(this.items);

  get items$() {
    return this._items$.asObservable();
  }

  destroy(item: INotification) {
    // Check if still exists
    const index = this.items.indexOf(item);
    if (index < 0) return;

    // Remove and update
    this.items.splice(index, 1);
    this._items$.next(this.items);
  }

  publish(item: INotification): INotification {
    this.items.push(item);
    this._items$.next(this.items);

    // Apply timeout if set
    if (item.timeout)
      setTimeout(() => this.destroy(item), item.timeout);

    return item;
  }
}
