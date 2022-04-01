import { Component } from '@angular/core';
import { map, Observer } from 'rxjs';
import { OverlayValveAliasEditComponent } from 'src/app/components/overlays/overlay-valve-alias-edit/overlay-valve-alias-edit.component';
import { OverlayValveTimerComponent } from 'src/app/components/overlays/overlay-valve-timer/overlay-valve-timer.component';
import { IStatePersistable } from 'src/app/models/state-persistable.interface';
import { compareValveIds, IValve } from 'src/app/models/valve.interface';
import { ComponentStateService } from 'src/app/services/component-state.service';
import { OverlaysService } from 'src/app/services/overlays.service';
import { ValvesService } from 'src/app/services/valves.service';

@Component({
  selector: 'app-page-valves',
  templateUrl: './page-valves.component.html',
  styleUrls: ['./page-valves.component.scss']
})
export class PageValvesComponent implements IStatePersistable {

  // #region State persisting

  stateToken = 'page-valves';

  get state(): any {
    return {
      valveIdSortAsc: this._valveIdSortAsc
    };
  }

  set state(v: any) {
    if (!v) return;
    this._valveIdSortAsc = v.valveIdSortAsc;
  }

  // #endregion

  private loadValvesObs: Partial<Observer<any>> = {
    next: () => this.loadValves(),
    error: () => this.loadValves(),
  };

  _valveIdSortAsc = true;
  get valveIdSortAsc() {
    return this._valveIdSortAsc;
  }
  set valveIdSortAsc(v: boolean) {
    this._valveIdSortAsc = v;
    this.stateService.save(this);
  }

  get valves$() {
    return this.valvesService.allValves.asObservable().pipe(
      map(it => {
        let sorted = it
          ?.sort(compareValveIds) || null;

        if (!this.valveIdSortAsc)
          sorted = sorted?.reverse() || null;

        return sorted;
      })
    );
  }

  constructor(
    private valvesService: ValvesService,
    private overlaysService: OverlaysService,
    private stateService: ComponentStateService,
  ) {
    this.stateService.load(this);
    this.loadValves();
  }

  private loadValves() {
    this.valvesService.getAllValves().subscribe();
  }

  editAlias(valve: IValve) {
    this.overlaysService.publish({
      component: OverlayValveAliasEditComponent,
      inputs: {
        valve,
        saved: (newAlias: string) => this.saveAlias(valve, newAlias),
      },
      userClosable: true,
    });
  }

  editTimer(valve: IValve) {
    this.overlaysService.publish({
      component: OverlayValveTimerComponent,
      inputs: {
        valve
      },
      userClosable: true,
    });
  }

  private saveAlias(valve: IValve, newAlias: string) {
    this.valvesService.putValve(valve.identifier, {
      alias: newAlias,
      disabled: valve.disabled,
    }).subscribe(this.loadValvesObs);
  }

  toggleDisable(valve: IValve) {
    this.valvesService.putValve(
      valve.identifier, {
        disabled: !valve.disabled,
        alias: valve.alias,
      }
    ).subscribe(this.loadValvesObs);
  }

  toggleState(valve: IValve) {
    if (valve.state)
      this.valvesService.deactivateValve(valve.identifier).subscribe(this.loadValvesObs);
    else
      this.valvesService.activateValve(valve.identifier).subscribe(this.loadValvesObs);
  }
}
