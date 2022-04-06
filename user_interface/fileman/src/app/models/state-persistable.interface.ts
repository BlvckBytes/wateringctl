export interface IStatePersistable<T> {
  /**
   * Get the current state
   */
  get state(): T;

  /**
   * Load into state
   */
  set state(v: T);

  /**
   * Unique token representing this state
   */
  stateToken?: string;
}