import { IInterval } from './interval.interface';

export interface IScheduledDay {
  disabled: boolean;
  intervals: IInterval[];
}