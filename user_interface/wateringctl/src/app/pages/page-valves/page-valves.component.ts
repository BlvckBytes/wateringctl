import { Component } from '@angular/core';
import { map } from 'rxjs';
import { compareValveIds } from 'src/app/models/valve.interface';
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
  ) {
    this.valvesService.getAllValves().subscribe();
  }
}
