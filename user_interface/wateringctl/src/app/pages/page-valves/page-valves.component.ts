import { Component } from '@angular/core';
import { map } from 'rxjs';
import { OverlayValveAliasEditComponent } from 'src/app/components/overlays/overlay-valve-alias-edit/overlay-valve-alias-edit.component';
import { compareValveIds, IValve } from 'src/app/models/valve.interface';
import { OverlaysService } from 'src/app/services/overlays.service';
import { ValvesService } from 'src/app/services/valves.service';

@Component({
  selector: 'app-page-valves',
  templateUrl: './page-valves.component.html',
  styleUrls: ['./page-valves.component.scss']
})
export class PageValvesComponent {

  valveIdSortAsc = true;
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
  ) {
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

  private saveAlias(valve: IValve, newAlias: string) {
    this.valvesService.putValve(valve.identifier, {
      alias: newAlias,
      disabled: valve.disabled,
    }).subscribe(() => this.loadValves());
  }
}
