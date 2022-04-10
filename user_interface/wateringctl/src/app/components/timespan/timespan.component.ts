import { Component, EventEmitter, Input, OnInit, Output } from '@angular/core';
import { calcIntervalDurSecs, IInterval, parseIntervalTime } from 'src/app/models/interval.interface';

export type TimespanSelection = 'start' | 'end';

@Component({
  selector: 'app-timespan',
  templateUrl: './timespan.component.html',
  styleUrls: ['./timespan.component.scss']
})
export class TimespanComponent implements OnInit {

  @Input() interval!: IInterval;
  @Input() selectable: boolean = false;
  @Output() selected = new EventEmitter<TimespanSelection>();

  private _selection: TimespanSelection = 'start';
  set selection(value: TimespanSelection) {
    this._selection = value;

    if (this.selectable)
      this.selected.emit(value);
  }
  get selection() {
    return this._selection;
  }

  ngOnInit(): void {
    this.selection = 'start';
  }

  trimIntervalTime(time: string): string {
    const parsed = parseIntervalTime(time);
    const parts = time.split(':');
    return (
      parts[0] + ':' + parts[1] + (parsed[2] > 0 ? `:${parts[2]}` : '')
    );
  }

  calcIntervalDur(interval: IInterval): string {
    return this.formatTimeString(calcIntervalDurSecs(interval));
  }

  private formatTimeString(seconds: number): string {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = Math.floor((seconds % 3600) % 60);

    return (
      (h > 0 ? `${h}h` : '') +
      (m > 0 ? ` ${m}m` : '') +
      (s > 0 || (h <= 0 && m <= 0) ? ` ${s}s` : '') 
    ).trim();
  }
}
