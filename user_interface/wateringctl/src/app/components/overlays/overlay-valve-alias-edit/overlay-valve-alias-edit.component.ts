import { Component, Input, OnDestroy } from '@angular/core';
import { FormControl, Validators } from '@angular/forms';
import { IValve } from 'src/app/models/valve.interface';
import { KeyEventsService } from 'src/app/services/key-events.service';
import { OverlaysService } from 'src/app/services/overlays.service';
import { SubSink } from 'subsink';

@Component({
  selector: 'app-overlay-valve-alias-edit',
  templateUrl: './overlay-valve-alias-edit.component.html',
  styleUrls: ['./overlay-valve-alias-edit.component.scss']
})
export class OverlayValveAliasEditComponent implements OnDestroy {

  private _subs = new SubSink();

  newAlias: FormControl;
  @Input() valve?: IValve = undefined;
  @Input() saved: (newAlias: string) => void = () => {};

  constructor(
    private overlaysService: OverlaysService,
    keysService: KeyEventsService,
  ) {
    this.newAlias = new FormControl('', [
      Validators.required, Validators.minLength(3), Validators.maxLength(16)
    ]);

    this._subs.sink = keysService.key$.subscribe(e => {
      if (e.key !== 'Enter') return;
      this.save();
    });
  }

  ngOnDestroy(): void {
    this._subs.unsubscribe();
  }

  save() {
    if (!this.newAlias.valid)
      return;

    this.saved?.(this.newAlias.value);
    this.overlaysService.destroyLatest();
  }
}
