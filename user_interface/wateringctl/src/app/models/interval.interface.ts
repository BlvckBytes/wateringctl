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

export const compareIntervalStarts = (a: IInterval, b: IInterval): number => {
  return compareIntervalTimes(a.start, b.start);
}

export const compareIntervalTimes = (a: string, b: string): number => {
  const [a_hours, a_minutes, a_seconds] = parseIntervalTime(a);
  const [b_hours, b_minutes, b_seconds] = parseIntervalTime(b);

  if (a_hours > b_hours) return 1;
  if (a_hours < b_hours) return -1;
  if (a_minutes > b_minutes) return 1;
  if (a_minutes < b_minutes) return -1;
  if (a_seconds > b_seconds) return 1;
  if (a_seconds < b_seconds) return -1;

  return 0;
};