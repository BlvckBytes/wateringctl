export interface IInterval {
  start: string;
  end: string;
  identifier: number;
  index: number;
  active: boolean;
}

export const isIntervalEmpty = (interval: IInterval): boolean => {
  return (
    interval.start === '00:00:00'
    && interval.end === '00:00:00'
    && interval.identifier == 0
    && interval.active == false
  );
};

export const parseIntervalTime = (time: string): number[] => {
  return time.split(':').map(it => Number.parseInt(it));
};