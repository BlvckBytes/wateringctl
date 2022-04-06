import { AfterViewInit, Directive, ElementRef, Input } from '@angular/core';

@Directive({
  selector: '[autofocus]'
})
export class AutofocusDirective implements AfterViewInit {

  @Input() autofocus = false;
  constructor(private host: ElementRef) {}

  ngAfterViewInit() {
    if (this.autofocus)
      this.host.nativeElement.focus();
  }
}