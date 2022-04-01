import { Component, Input, OnInit } from '@angular/core';
import { IValve } from 'src/app/models/valve.interface';

@Component({
  selector: 'app-overlay-valve-timer',
  templateUrl: './overlay-valve-timer.component.html',
  styleUrls: ['./overlay-valve-timer.component.scss']
})
export class OverlayValveTimerComponent implements OnInit {

  @Input() valve?: IValve = undefined;

  constructor() { }

  ngOnInit(): void {
  }

}
