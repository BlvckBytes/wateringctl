import { Component, Input } from '@angular/core';

@Component({
  selector: 'app-overlay-valve-alias-edit',
  templateUrl: './overlay-valve-alias-edit.component.html',
  styleUrls: ['./overlay-valve-alias-edit.component.scss']
})
export class OverlayValveAliasEditComponent {

  @Input() alias: string | null = null;

}
