import { AfterViewInit, Component, ElementRef, EventEmitter, Input, OnInit, Output, ViewChild } from '@angular/core';

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

  @Output() selected = new EventEmitter<number>();

  @Input() set currentValue(value: number) {
    this.currNum = value;
    this.updateValues(false);
  }

  values: string[] = [];
  private deltaScalerDesktop = 1/20;
  private deltaScalerMobile = 1/12;

  private currNum = this.from;

  ngOnInit(): void {
    this.currNum = this.from;
    this.updateValues();
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
    this.currNum = (this.currNum + delta) % (this.to + 1);
    if (this.currNum < 0)
      this.currNum = this.to;

    this.updateValues();
  }

  private updateValues(emit: boolean = true) {
    this.values = [];

    const v = Math.floor(this.currNum);
    this.values.push(this.padNumber(v == this.from ? this.to : v - 1));
    this.values.push(this.padNumber(v));
    this.values.push(this.padNumber((v + 1) % (this.to + 1)));

    if (emit)
      this.selected.emit(v);
  }

  private padNumber(value: number): string {
    return String(value).padStart(this.padTo, '0');
  }
}
