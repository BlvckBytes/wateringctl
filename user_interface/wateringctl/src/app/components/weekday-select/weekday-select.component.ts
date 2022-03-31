import { Component, EventEmitter, OnInit, Output } from '@angular/core';
import { ESchedulerWeekday } from 'src/app/models/scheduler-weekday.enum';

@Component({
  selector: 'app-weekday-select',
  templateUrl: './weekday-select.component.html',
  styleUrls: ['./weekday-select.component.scss']
})
export class WeekdaySelectComponent implements OnInit {

  days: string[];
  selectedDay: string;

  @Output() selection: EventEmitter<ESchedulerWeekday>;

  constructor() {
    this.selection = new EventEmitter();
    this.days = Object.values<string>(ESchedulerWeekday).filter(value => typeof value === 'string');
    this.selectedDay = this.days[new Date().getDay() - 1];
  }

  selectDay(day: string): void {
    this.selectedDay = day;
    this.selection.emit(day as ESchedulerWeekday);
  }

  ngOnInit(): void {
    this.selectDay(this.selectedDay);
  }
}
