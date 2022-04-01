import { Component, Input, OnInit } from '@angular/core';
import { IInterval } from 'src/app/models/interval.interface';
import { ValvesService } from 'src/app/services/valves.service';

@Component({
  selector: 'app-overlay-interval-timestamps-edit',
  templateUrl: './overlay-interval-timestamps-edit.component.html',
  styleUrls: ['./overlay-interval-timestamps-edit.component.scss']
})
export class OverlayIntervalTimestampsEditComponent implements OnInit {

  @Input() interval?: IInterval = undefined;

  constructor(
    private valveService: ValvesService,
  ) { }

  ngOnInit(): void {
  }

  resolveValveAlias(interval?: IInterval): string {
    return this.valveService.resolveValveAlias(interval);
  }

  save() {

  }
}
