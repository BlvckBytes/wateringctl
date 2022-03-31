import { Component } from '@angular/core';
import { ValvesService } from 'src/app/services/valves.service';

@Component({
  selector: 'app-page-valves',
  templateUrl: './page-valves.component.html',
  styleUrls: ['./page-valves.component.scss']
})
export class PageValvesComponent {

  get valves$() {
    return this.valvesService.allValves.asObservable();
  }

  constructor(
    private valvesService: ValvesService,
  ) {
    this.valvesService.getAllValves().subscribe();
  }
}
