import { Component, Input } from '@angular/core';
import { interval } from 'rxjs';
import { IInterval, parseIntervalTime } from 'src/app/models/interval.interface';

@Component({
  selector: 'app-timespan',
  templateUrl: './timespan.component.html',
  styleUrls: ['./timespan.component.scss']
})
export class TimespanComponent {

  @Input() interval!: IInterval;

  trimIntervalTime(time: string): string {
    const parsed = parseIntervalTime(time);
    const parts = time.split(':');
    return (
      parts[0] + ':' + parts[1] + (parsed[2] > 0 ? `:${parts[2]}` : '')
    );
  }

  formatTimeString(seconds: number): string {
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = Math.floor((seconds % 3600) % 60);

    return (
      (h > 0 ? `${h}h` : '') +
      (m > 0 ? ` ${m}m` : '') +
      (s > 0 ? ` ${s}s` : '') 
    ).trim();
  }

  calcIntervalDur(interval: IInterval): string {
    const start = parseIntervalTime(interval.start);
    const end = parseIntervalTime(interval.end);
    return this.formatTimeString(
      (end[2] - start[2]) +
      (end[1] - start[1]) * 60 +
      (end[0] - start[0]) * 3600
    );
  }
}
