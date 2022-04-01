import { Component } from '@angular/core';
import { BehaviorSubject, map, Observable } from 'rxjs';
import { OverlayIntervalTargetEditComponent } from 'src/app/components/overlays/overlay-interval-target-edit/overlay-interval-target-edit.component';
import { compareIntervalStarts, IInterval, isIntervalEmpty, parseIntervalTime } from 'src/app/models/interval.interface';
import { IScheduledDay } from 'src/app/models/scheduled-day.interface';
import { ESchedulerWeekday } from 'src/app/models/scheduler-weekday.enum';
import { IStatePersistable } from 'src/app/models/state-persistable.interface';
import { ComponentStateService } from 'src/app/services/component-state.service';
import { NotificationsService } from 'src/app/services/notifications.service';
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
    private notificationService: NotificationsService,
    private stateService: ComponentStateService,
    private overlayService: OverlaysService,
  ) {
    this.stateService.load(this);
  }

  notificationTest() {
    this.notificationService.publish({
      headline: "Test",
      text: "This is an example of a notification",
      color: "warning",
      buttons: [],
      icon: "trash.svg",
      timeout: 5000,
    });
  }

  resolveValve(interval: IInterval): string {
    return this.valveService.allValves.value
      ?.find(it => it.identifier === interval.identifier)
      ?.alias || '?';
  }

  editTarget(interval: IInterval) {
    this.overlayService.publish({
      component: OverlayIntervalTargetEditComponent,
      inputs: {
        interval,
        availableValves: this.valveService.allValves.value,
      },
      userClosable: true,
    });
  }
}
