import { Component, HostBinding, Input } from '@angular/core';
import { ButtonColor } from './button-color.type';

@Component({
  selector: 'app-button',
  templateUrl: './button.component.html',
  styleUrls: ['./button.component.scss']
})
export class ButtonComponent {

  @Input() icon?: string;
  @Input() text?: string;

  @HostBinding('style.background')
  colorValue?: string;

  @Input() set color(value: ButtonColor) {
    this.colorValue = `var(--${value})`;
  }
}
