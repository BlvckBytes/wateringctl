import { AfterViewInit, Component, ElementRef, Input, OnInit, ViewChild } from '@angular/core';
import { from } from 'rxjs';

@Component({
  selector: 'app-vert-range',
  templateUrl: './vert-range.component.html',
  styleUrls: ['./vert-range.component.scss']
})
export class VertRangeComponent implements AfterViewInit, OnInit {

  @ViewChild('evHandle') evHandle!: ElementRef;

  @Input() from: number = 0;
  @Input() to: number = 59;
  @Input() padTo: number = 2;

  values: string[] = [];
  private deltaScalerDesktop = 1/20;
  private deltaScalerMobile = 1/12;

  _currNum = this.from;
  set currNum(value: number) {
    this._currNum = value;
    this.updateValues();
  }
  get currNum() {
    return this._currNum;
  }

  ngOnInit(): void {
    this.currNum = this.from;
  }

  ngAfterViewInit(): void {
    // Scrollwheel
    this.evHandle.nativeElement.onwheel = (e: any) => {
      this.applyDeltaY(e.deltaY * this.deltaScalerDesktop);
    };

    // Touchscreen
    let prevY = 0;
    this.evHandle.nativeElement.ontouchstart = (e: any) => prevY = e.touches[0].pageY;
    this.evHandle.nativeElement.ontouchmove = (e: any) => {
      const currY = e.touches[0].pageY;
      this.applyDeltaY((prevY - currY) * this.deltaScalerMobile);
      prevY = currY;
    };
  }

  private applyDeltaY(delta: number) {
    this.currNum = (this._currNum + delta) % (this.to + 1);
    if (this._currNum < 0)
      this.currNum = this.to;
  }

  private updateValues() {
    this.values = [];

    const v = Math.floor(this.currNum);
    this.values.push(this.padNumber(v == this.from ? this.to : v - 1));
    this.values.push(this.padNumber(v));
    this.values.push(this.padNumber((v + 1) % (this.to + 1)));
  }

  private padNumber(value: number): string {
    return String(value).padStart(this.padTo, '0');
  }
}
