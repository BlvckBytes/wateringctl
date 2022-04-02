export interface IValve {
  alias: string;
  disabled: boolean;
  state: boolean;
  identifier: number;
  timer: string;
}

export const compareValveIds = (a: IValve, b: IValve): number => {
  if (a.identifier > b.identifier)
    return 1;
  if (a.identifier < b.identifier)
    return -1;
  return 0;
}