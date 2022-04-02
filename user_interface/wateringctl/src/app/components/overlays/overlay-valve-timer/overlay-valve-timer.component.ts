import { Component, Input, OnInit } from '@angular/core';
import { parseIntervalTime } from 'src/app/models/interval.interface';
import { IValve } from 'src/app/models/valve.interface';

@Component({
  selector: 'app-overlay-valve-timer',
  templateUrl: './overlay-valve-timer.component.html',
  styleUrls: ['./overlay-valve-timer.component.scss']
})
export class OverlayValveTimerComponent implements OnInit {

  @Input() valve?: IValve = undefined;

  get timerParts(): number[] {
    if (!this.valve)
      return [0, 0, 0];
    return parseIntervalTime(this.valve.timer);
  }

  constructor() { }

  ngOnInit(): void {
  }

}
