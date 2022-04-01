import { ChangeDetectorRef, Component, Input, OnInit } from '@angular/core';
import { calcIntervalDurSecs, IInterval, parseIntervalTime, stringifyIntervalTime } from 'src/app/models/interval.interface';
import { OverlaysService } from 'src/app/services/overlays.service';
import { ValvesService } from 'src/app/services/valves.service';
import { TimespanSelection } from '../../timespan/timespan.component';

@Component({
  selector: 'app-overlay-interval-timestamps-edit',
  templateUrl: './overlay-interval-timestamps-edit.component.html',
  styleUrls: ['./overlay-interval-timestamps-edit.component.scss']
})
export class OverlayIntervalTimestampsEditComponent implements OnInit {

  @Input() interval?: IInterval = undefined;
  @Input() saved: (start: string, end: string) => void = () => {};

  displayInterval?: IInterval = undefined;
  hoursValue: number = 0;
  minutesValue: number = 0;
  secondsValue: number = 0;

  private timespanSelection: TimespanSelection = 'start';

  constructor(
    private valveService: ValvesService,
    private cdRef: ChangeDetectorRef,
    private overlaysService: OverlaysService,
  ) { }

  ngOnInit(): void {
    if (this.interval) {
      this.displayInterval = { ...this.interval };
    }
  }

  private patchTimePart(selection: TimespanSelection, part: number, value: number) {
    if (this.displayInterval) {
      const parts = parseIntervalTime(this.displayInterval[selection]);
      parts[part] = value;
      this.displayInterval[selection] = stringifyIntervalTime(parts);
    }
  }

  hoursChanged(value: number) {
    this.patchTimePart(this.timespanSelection, 0, value);
  }

  minutesChanged(value: number) {
    this.patchTimePart(this.timespanSelection, 1, value);
  }

  secondsChanged(value: number) {
    this.patchTimePart(this.timespanSelection, 2, value);
  }

  resolveValveAlias(interval?: IInterval): string {
    return this.valveService.resolveValveAlias(interval);
  }

  timespanSelected(selection: TimespanSelection) {
    this.timespanSelection = selection;

    if (this.displayInterval) {
      const [h, m, s] = parseIntervalTime(selection === 'start' ? this.displayInterval.start : this.displayInterval.end);
      this.hoursValue = h;
      this.minutesValue = m;
      this.secondsValue = s;
      this.cdRef.detectChanges();
    }
  }

  isTimespanValid(): boolean {
    if (!this.displayInterval)
      return false;
    return calcIntervalDurSecs(this.displayInterval) > 0;
  }

  save() {
    if (!this.isTimespanValid())
      return;
    
    if (this.displayInterval) {
      this.saved(this.displayInterval.start, this.displayInterval.end);
      this.overlaysService.destroyLatest();
    }
  }
}
