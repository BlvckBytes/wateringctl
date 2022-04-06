import { AfterViewInit, Directive, EventEmitter, Output } from '@angular/core';

@Directive({
  selector: '[ngViewInit]'
})
export class NgViewInitDirective implements AfterViewInit {

  @Output() ngViewInit = new EventEmitter<void>();

  ngAfterViewInit(): void {
    this.ngViewInit.next();
  }
}