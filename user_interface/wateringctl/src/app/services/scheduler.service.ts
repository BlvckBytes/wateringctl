import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import { IInterval } from '../models/interval.interface';
import { IIntervalRequest } from '../models/request/interval-request.interface';
import { IScheduledDayRequest } from '../models/request/scheduled-day-request.interface';
import { IScheduledDay } from '../models/scheduled-day.interface';
import { ESchedulerWeekday } from '../models/scheduler-weekday.enum';
import { HttpService } from './http.service';

@Injectable({
  providedIn: 'root'
})
export class SchedulerService {

  constructor(
    private httpService: HttpService,
  ) {}

  getDaysSchedule(
    day: ESchedulerWeekday
  ): Observable<IScheduledDay> {
    return this.httpService.get(`/scheduler/${day}`);
  }

  putDaysSchedule(
    day: ESchedulerWeekday,
    schedule: IScheduledDayRequest
  ): Observable<IScheduledDay> {
    return this.httpService.put(`/scheduler/${day}`, schedule);
  }

  putDaysIndexedInterval(
    day: ESchedulerWeekday,
    index: number,
    interval: IIntervalRequest
  ): Observable<IInterval> {
    return this.httpService.put(`/scheduler/${day}/${index}`, interval);
  }

  deleteDaysIndexedInterval(
    day: ESchedulerWeekday,
    index: number,
  ): Observable<void> {
    return this.httpService.delete(`/scheduler/${day}/${index}`);
  }
}
