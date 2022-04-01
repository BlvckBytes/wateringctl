import { Component, Input } from '@angular/core';
import { FormControl, Validators } from '@angular/forms';
import { OverlaysService } from 'src/app/services/overlays.service';

@Component({
  selector: 'app-overlay-valve-alias-edit',
  templateUrl: './overlay-valve-alias-edit.component.html',
  styleUrls: ['./overlay-valve-alias-edit.component.scss']
})
export class OverlayValveAliasEditComponent {

  newAlias: FormControl;
  @Input() alias: string | null = null;
  @Input() relayId: number | null = null;
  @Input() saved: (newAlias: string) => void = () => {};

  constructor(
    private overlaysService: OverlaysService,
  ) {
    this.newAlias = new FormControl('', [
      Validators.required, Validators.minLength(3), Validators.maxLength(16)
    ]);
  }

  save() {
    if (!this.newAlias.valid)
      return;

    this.saved?.(this.newAlias.value);
    this.overlaysService.destroyLatest();
  }
}
