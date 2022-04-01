import { Component, Input } from '@angular/core';
import { FormControl, Validators } from '@angular/forms';
import { IInterval } from 'src/app/models/interval.interface';
import { IValve } from 'src/app/models/valve.interface';
import { KeyEventsService } from 'src/app/services/key-events.service';
import { OverlaysService } from 'src/app/services/overlays.service';
import { ValvesService } from 'src/app/services/valves.service';
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
    private valveService: ValvesService,
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

  generateTargetTypeahead(): string[] {
    return this.availableValves.map(it => it.alias);
  }

  resolveValveAlias(interval?: IInterval): string {
    return this.valveService.resolveValveAlias(interval);
  }
}
