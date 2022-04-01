import { Component } from '@angular/core';
import { BehaviorSubject, map, Observable } from 'rxjs';
import { compareIntervalStarts, IInterval, isIntervalEmpty, parseIntervalTime } from 'src/app/models/interval.interface';
import { IScheduledDay } from 'src/app/models/scheduled-day.interface';
import { ESchedulerWeekday } from 'src/app/models/scheduler-weekday.enum';
import { SchedulerService } from 'src/app/services/scheduler.service';
import { ValvesService } from 'src/app/services/valves.service';

@Component({
  selector: 'app-page-schedules',
  templateUrl: './page-schedules.component.html',
  styleUrls: ['./page-schedules.component.scss']
})
export class PageSchedulesComponent {

  private _currentSchedule = new BehaviorSubject<IScheduledDay | null>(null);

  timestampSortAsc = true;
  get currentActiveIntervals$(): Observable<IInterval[] | null> {
    return this._currentSchedule.asObservable().pipe(
      map(it => {
        if (!it?.intervals) return null;

        let preproc = it?.intervals
          ?.sort(compareIntervalStarts)
          ?.filter(it => !isIntervalEmpty(it));

        if (!this.timestampSortAsc)
          preproc = preproc?.reverse();

        return preproc;
      })
    );
  }

  set selectedDay(value: string) {
    this._currentSchedule.next(null);
    this.valveService.getAllValves().subscribe(v => {
      this.schedulerService.getDaysSchedule(value as ESchedulerWeekday).subscribe(v => {
        this._currentSchedule.next(v);
      });
    });
  }

  constructor(
    private schedulerService: SchedulerService,
    private valveService: ValvesService,
  ) {}

  trimIntervalTime(time: string): string {
    const parsed = parseIntervalTime(time);
    const parts = time.split(':');
    return (
      parts[0] + ':' + parts[1] + (parsed[2] > 0 ? `:${parts[2]}` : '')
    );
  }

  formatTimeString(seconds: number): string {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = Math.floor((seconds % 3600) % 60);

    return (
      (h > 0 ? `${h}h` : '') +
      (m > 0 ? ` ${m}m` : '') +
      (s > 0 ? ` ${s}s` : '') 
    ).trim();
  }

  calcIntervalDur(interval: IInterval): string {
    const start = parseIntervalTime(interval.start);
    const end = parseIntervalTime(interval.end);
    return this.formatTimeString(
      (end[2] - start[2]) +
      (end[1] - start[1]) * 60 +
      (end[0] - start[0]) * 3600
    );
  }

  resolveValve(interval: IInterval): string {
    return this.valveService.allValves.value
      ?.find(it => it.identifier === interval.identifier)
      ?.alias || '?';
  }
}
