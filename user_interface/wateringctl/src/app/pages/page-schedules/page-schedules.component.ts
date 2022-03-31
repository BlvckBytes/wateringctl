import { Component, OnInit } from '@angular/core';

@Component({
  selector: 'app-page-schedules',
  templateUrl: './page-schedules.component.html',
  styleUrls: ['./page-schedules.component.scss']
})
export class PageSchedulesComponent implements OnInit {

  set selectedDay(value: string) {
    console.log(value);
  }

  constructor() { }

  ngOnInit(): void {
  }

}
