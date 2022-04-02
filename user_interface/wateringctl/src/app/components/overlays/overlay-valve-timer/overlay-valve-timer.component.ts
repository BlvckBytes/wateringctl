import { Component, Input, OnInit } from '@angular/core';
import { parseIntervalTime, stringifyIntervalTime } from 'src/app/models/interval.interface';
import { IValve } from 'src/app/models/valve.interface';
import { ValvesService } from 'src/app/services/valves.service';

@Component({
  selector: 'app-overlay-valve-timer',
  templateUrl: './overlay-valve-timer.component.html',
  styleUrls: ['./overlay-valve-timer.component.scss']
})
export class OverlayValveTimerComponent implements OnInit {

  @Input() valve?: IValve = undefined;

  timeSelection: number[] = [0, 0, 0];

  get timerParts(): number[] {
    if (!this.valve)
      return [0, 0, 0];
    return parseIntervalTime(this.valve.timer);
  }

  constructor(
    private valvesService: ValvesService,
  ) {}

  ngOnInit(): void {
    this.timeSelection = this.timerParts;
  }

  isTimerActive(): boolean {
    if (!this.valve)
      return false;
    return this.valve.timer !== '00:00:00';
  }

  stopTimer() {
    if (!this.isTimerActive() || !this.valve)
      return;

    this.valvesService.clearValveTimer(this.valve.identifier).subscribe();
  }

  startTimer() {
    if (this.isTimerActive() || !this.valve)
      return;

    this.valvesService.setValveTimer(
      this.valve.identifier,
      {
        duration: stringifyIntervalTime(this.timeSelection),
      }
    ).subscribe();
  }
}
