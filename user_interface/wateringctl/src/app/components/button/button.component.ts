import { Component, HostBinding, Input } from '@angular/core';
import { ButtonColor } from './button-color.type';

@Component({
  selector: 'app-button',
  templateUrl: './button.component.html',
  styleUrls: ['./button.component.scss']
})
export class ButtonComponent {

  _icon?: string;
  @Input() set icon(value: string) {
    this._icon = value;
    this.iconOnly = this._text === undefined;
  }

  _text?: string;
  @Input() set text(value: string) {
    this._text = value;
    this.iconOnly = false;
  }

  @HostBinding('style.background')
  colorValue?: string;

  @HostBinding('class.--icon-only')
  iconOnly = true;

  @Input() set color(value: ButtonColor) {
    this.colorValue = `var(--${value})`;
  }
}
