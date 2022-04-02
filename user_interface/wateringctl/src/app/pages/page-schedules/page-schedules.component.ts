import { Component, OnDestroy } from '@angular/core';
import { TranslateService } from '@ngx-translate/core';
import { BehaviorSubject, map, Observable, Observer } from 'rxjs';
import { OverlayIntervalTargetEditComponent } from 'src/app/components/overlays/overlay-interval-target-edit/overlay-interval-target-edit.component';
import { OverlayIntervalTimestampsEditComponent } from 'src/app/components/overlays/overlay-interval-timestamps-edit/overlay-interval-timestamps-edit.component';
import { compareIntervalStarts, IInterval, isIntervalEmpty, setIntervalEmpty } from 'src/app/models/interval.interface';
import { IScheduledDay } from 'src/app/models/scheduled-day.interface';
import { ESchedulerWeekday } from 'src/app/models/scheduler-weekday.enum';
import { IStatePersistable } from 'src/app/models/state-persistable.interface';
import { EWebSocketEventType } from 'src/app/models/web-socket-event-type.enum';
import { IWebSocketEvent } from 'src/app/models/web-socket-event.interface';
import { ComponentStateService } from 'src/app/services/component-state.service';
import { NotificationsService } from 'src/app/services/notifications.service';
import { OverlaysService } from 'src/app/services/overlays.service';
import { SchedulerService } from 'src/app/services/scheduler.service';
import { ValvesService } from 'src/app/services/valves.service';
import { WebSocketEventsService } from 'src/app/services/web-socket-events.service';
import { SubSink } from 'subsink';

@Component({
  selector: 'app-page-schedules',
  templateUrl: './page-schedules.component.html',
  styleUrls: ['./page-schedules.component.scss']
})
export class PageSchedulesComponent implements IStatePersistable, OnDestroy {

  private _subs = new SubSink();

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

  private loadScheduleObs: Partial<Observer<any>> = {
    next: () => this.loadSchedule(false),
    error: () => this.loadSchedule(false),
  };

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

  set selectedDay(value: string) {
    this._currentDay = value;
    this.loadSchedule();
  }

  constructor(
    private schedulerService: SchedulerService,
    private valveService: ValvesService,
    private stateService: ComponentStateService,
    private overlayService: OverlaysService,
    private notificationsService: NotificationsService,
    private tranService: TranslateService,
    eventService: WebSocketEventsService,
  ) {
    this.stateService.load(this);
    this._subs.sink = eventService.events.subscribe(e => this.handleWSE(e));
  }

  private findInterval(wse: IWebSocketEvent, action: (interval: IInterval, args: string[]) => void) {
    const day = wse.args[0] as ESchedulerWeekday;
    const sched = this._currentSchedule.value;

    // Do nothing for non-selected days
    if (day !== this._currentDay || !sched)
      return;

    // Index not yet known, out of sync, refresh
    const index = Number.parseInt(wse.args[1]);
    const targ = sched.intervals.find(it => it.index === index);

    if (!targ) {
      this.loadSchedule(false);
      return;
    }

    action(targ, wse.args.slice(2));
  }

  private handleWSE(wse: IWebSocketEvent) {
    if (
      wse.type === EWebSocketEventType.WSE_DAY_DISABLE_ON ||
      wse.type === EWebSocketEventType.WSE_DAY_DISABLE_OFF
    ) {
      const day = wse.args[0] as ESchedulerWeekday;
      if (day !== this._currentDay || !this._currentSchedule.value)
        return;

      this._currentSchedule.value.disabled = wse.type === EWebSocketEventType.WSE_DAY_DISABLE_ON;
    }

    if (
      wse.type === EWebSocketEventType.WSE_INTERVAL_DISABLE_ON ||
      wse.type === EWebSocketEventType.WSE_INTERVAL_DISABLE_OFF
    )
      this.findInterval(wse, i => i.disabled = wse.type === EWebSocketEventType.WSE_INTERVAL_DISABLE_ON);

    if (wse.type === EWebSocketEventType.WSE_INTERVAL_START_CHANGE)
      this.findInterval(wse, (i, a) => i.start = a[0]);

    if (wse.type === EWebSocketEventType.WSE_INTERVAL_END_CHANGE)
      this.findInterval(wse, (i, a) => i.end = a[0]);

    if (wse.type === EWebSocketEventType.WSE_INTERVAL_IDENTIFIER_CHANGE)
      this.findInterval(wse, (i, a) => i.identifier = Number.parseInt(a[0]));

    if (wse.type === EWebSocketEventType.WSE_INTERVAL_DELETED)
      this.findInterval(wse, i => setIntervalEmpty(i));
  }

  ngOnDestroy(): void {
    this._subs.unsubscribe();
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

  resolveValveAlias(interval?: IInterval): string {
    return this.valveService.resolveValveAlias(interval);
  }

  isCurrentActiveDayDisabled(): boolean {
    return this._currentSchedule.value?.disabled || false;
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
    }).subscribe(this.loadScheduleObs);
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
    }).subscribe(this.loadScheduleObs);
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

  toggleDayDisabled() {
    const schedule = this._currentSchedule.value;
    if (schedule === null)
      return;

    this.schedulerService.putDaysSchedule(this._currentDay as ESchedulerWeekday, {
      disabled: !schedule.disabled,
    },).subscribe(this.loadScheduleObs);
  }

  createInterval() {
    const schedule = this._currentSchedule.value;
    if (schedule === null)
      return;

    const nextEmpty = schedule.intervals.find(it => isIntervalEmpty(it));
    if (!nextEmpty) {
      this.notificationsService.publish({
        headline: this.tranService.instant('server_errors.headline'),
        text: this.tranService.instant('server_errors.NO_INT_SLOTS'),
        color: 'warning',
        timeout: 5000,
        buttons: [],
        icon: 'warning.svg'
      });
      return;
    }

    const now = new Date();
    this.schedulerService.putDaysIndexedInterval(
      this._currentDay as ESchedulerWeekday,
      nextEmpty.index,
      {
        identifier: 0,
        disabled: false,
        start: `${now.getHours()}:${now.getMinutes()}:00`,
        end: `${now.getHours()}:${now.getMinutes() + 5}:00`,
      }
    ).subscribe(this.loadScheduleObs);
  }

  private deleteIntervalConfirm(interval: IInterval) {
    this.schedulerService.deleteDaysIndexedInterval(
      this._currentDay as ESchedulerWeekday,
      interval.index
    ).subscribe(this.loadScheduleObs);
  }

  deleteInterval(interval: IInterval) {
    const handle = this.notificationsService.publish({
      headline: this.tranService.instant('delete_confirmation.headline'),
      text: this.tranService.instant('delete_confirmation.text'),
      color: 'warning',
      buttons: [
        {
          text: this.tranService.instant('delete_confirmation.yes'),
          callback: () => {
            this.deleteIntervalConfirm(interval);
            this.notificationsService.destroy(handle);
          },
        },
        {
          text: this.tranService.instant('delete_confirmation.no'),
          callback: () => {
            this.notificationsService.destroy(handle);
          },
        }
      ],
      icon: 'warning.svg'
    });
  }

  toggleIntervalDisable(interval: IInterval) {
    this.schedulerService.putDaysIndexedInterval(
      this._currentDay as ESchedulerWeekday,
      interval.index,
      {
        ...interval,
        disabled: !interval.disabled,
      }
    ).subscribe(this.loadScheduleObs);
  }
}
