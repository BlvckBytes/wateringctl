export interface IStatePersistable {
  /**
   * Get the current state
   */
  get state(): any;

  /**
   * Load into state
   */
  set state(v: any);

  /**
   * Unique token representing this state
   */
  stateToken: string;
}