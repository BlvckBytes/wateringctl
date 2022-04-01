import { Component, Input } from '@angular/core';
import { FormControl, Validators } from '@angular/forms';
import { IInterval } from 'src/app/models/interval.interface';
import { IValve } from 'src/app/models/valve.interface';
import { KeyEventsService } from 'src/app/services/key-events.service';
import { OverlaysService } from 'src/app/services/overlays.service';
import { SubSink } from 'subsink';

@Component({
  selector: 'app-overlay-interval-target-edit',
  templateUrl: './overlay-interval-target-edit.component.html',
  styleUrls: ['./overlay-interval-target-edit.component.scss']
})
export class OverlayIntervalTargetEditComponent {

  private subs = new SubSink();

  newTarget: FormControl;
  @Input() interval?: IInterval = undefined;
  @Input() availableValves: IValve[] = [];
  @Input() saved: (newTarget: string) => void = () => {};

  constructor(
    private overlaysService: OverlaysService,
    keysService: KeyEventsService,
  ) {
    this.newTarget = new FormControl('', [
      Validators.required, Validators.minLength(3), Validators.maxLength(16)
    ]);

    this.subs.sink = keysService.key$.subscribe(e => {
      if (e.key !== 'Enter') return;
      this.save();
    });
  }
  
  save() {
    if (!this.newTarget.valid)
      return;

    this.saved?.(this.newTarget.value);
    this.overlaysService.destroyLatest();
  }

  resolveValve(interval?: IInterval): string {
    return this.availableValves
      .find(it => it.identifier === interval?.identifier)
      ?.alias || '?';
  }
}
