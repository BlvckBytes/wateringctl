import { Injectable } from '@angular/core';
import { IStatePersistable } from '../models/state-persistable.interface';

@Injectable({
  providedIn: 'root'
})
export class ComponentStateService {

  private storageKey = 'component-state';
  private state: any;

  constructor() {
    this.state = JSON.parse(
      window.localStorage.getItem(this.storageKey) || '{}',
      this.parseReviver
    );
  }

  /**
   * Save a component's state persistently
   */
  save<T>(persistable: IStatePersistable<T>) {
    // No token - cannot be saved
    if (!persistable.stateToken)
      return;

    this.state[persistable.stateToken] = persistable.state;
    window.localStorage.setItem(this.storageKey, JSON.stringify(this.state, this.stringifyReplacer));
  }

  /**
   * Load a component's state from persistence
   */
  load<T>(persistable: IStatePersistable<T>) {
    // No token - cannot be loaded
    if (!persistable.stateToken)
      return;

    persistable.state = this.state[persistable.stateToken];
  }

  /**
   * Revive any previously stringified objects when parsing JSON
   */
  private parseReviver(_: any, value: any) {
    // Return value as is
    return value;
  }

  /**
   * Stringify objects before putting them into JSON
   */
  private stringifyReplacer(_: string, value: any) {
    // Turn moments or dates into their ISO representation
    if (value._isAMomentObject || value instanceof Date) return value.toISOString();

    // Return value as is
    return value;
  }
}