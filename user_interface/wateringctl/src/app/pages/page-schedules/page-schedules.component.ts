import { Component } from '@angular/core';
import { BehaviorSubject, map, Observable } from 'rxjs';
import { OverlayIntervalTargetEditComponent } from 'src/app/components/overlays/overlay-interval-target-edit/overlay-interval-target-edit.component';
import { OverlayIntervalTimestampsEditComponent } from 'src/app/components/overlays/overlay-interval-timestamps-edit/overlay-interval-timestamps-edit.component';
import { compareIntervalStarts, IInterval, isIntervalEmpty } from 'src/app/models/interval.interface';
import { IScheduledDay } from 'src/app/models/scheduled-day.interface';
import { ESchedulerWeekday } from 'src/app/models/scheduler-weekday.enum';
import { IStatePersistable } from 'src/app/models/state-persistable.interface';
import { ComponentStateService } from 'src/app/services/component-state.service';
import { OverlaysService } from 'src/app/services/overlays.service';
import { SchedulerService } from 'src/app/services/scheduler.service';
import { ValvesService } from 'src/app/services/valves.service';

@Component({
  selector: 'app-page-schedules',
  templateUrl: './page-schedules.component.html',
  styleUrls: ['./page-schedules.component.scss']
})
export class PageSchedulesComponent implements IStatePersistable {

  // #region State persisting

  stateToken = 'page-schedules';

  get state(): any {
    return {
      timestampSortAsc: this._timestampSortAsc
    };
  }

  set state(v: any) {
    if (!v) return;
    this._timestampSortAsc = v.timestampSortAsc;
  }

  // #endregion

  private _currentSchedule = new BehaviorSubject<IScheduledDay | null>(null);
  private _currentDay: string = "";

  _timestampSortAsc = true;
  get timestampSortAsc() {
    return this._timestampSortAsc;
  }
  set timestampSortAsc(v: boolean) {
    this._timestampSortAsc = v;
    this.stateService.save(this);
  }

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

  private loadSchedule(clear: boolean = true) {
    if (clear)
      this._currentSchedule.next(null);

    this.valveService.getAllValves().subscribe(v => {
      this.schedulerService.getDaysSchedule(this._currentDay as ESchedulerWeekday).subscribe(v => {
        this._currentSchedule.next(v);
      });
    });
  }

  set selectedDay(value: string) {
    this._currentDay = value;
    this.loadSchedule();
  }

  constructor(
    private schedulerService: SchedulerService,
    private valveService: ValvesService,
    private stateService: ComponentStateService,
    private overlayService: OverlaysService,
  ) {
    this.stateService.load(this);
  }

  resolveValveAlias(interval?: IInterval): string {
    return this.valveService.resolveValveAlias(interval);
  }

  private saveTarget(interval: IInterval, newTarget: string) {
    const targetValve = this.valveService.allValves.value
      ?.find(it => it.alias.toLowerCase() === newTarget.toLowerCase());

    if (!targetValve)
      return;

    this.schedulerService.putDaysIndexedInterval(this._currentDay as ESchedulerWeekday, interval.index, {
      start: interval.start,
      end: interval.end,
      identifier: targetValve.identifier,
      disabled: interval.disabled,
    }).subscribe(() => this.loadSchedule(false));
  }

  editTarget(interval: IInterval) {
    this.overlayService.publish({
      component: OverlayIntervalTargetEditComponent,
      inputs: {
        interval,
        availableValves: this.valveService.allValves.value,
        saved: (newTarget: string) => this.saveTarget(interval, newTarget),
      },
      userClosable: true,
    });
  }

  private saveTimespans(interval: IInterval, start: string, end: string) {
    this.schedulerService.putDaysIndexedInterval(this._currentDay as ESchedulerWeekday, interval.index, {
      identifier: interval.identifier,
      disabled: interval.disabled,
      start, end,
    }).subscribe(() => this.loadSchedule(false));
  }

  editTimespans(interval: IInterval) {
    this.overlayService.publish({
      component: OverlayIntervalTimestampsEditComponent,
      inputs: {
        interval,
        saved: (start: string, end: string) => this.saveTimespans(interval, start, end),
      },
      userClosable: true,
    });
  }
}
