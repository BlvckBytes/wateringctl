import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { PageSchedulesComponent } from './pages/page-schedules/page-schedules.component';
import { PageValvesComponent } from './pages/page-valves/page-valves.component';

const routes: Routes = [
  { path: 'schedules', component: PageSchedulesComponent },
  { path: 'valves', component: PageValvesComponent },
  { path: '**', redirectTo: 'schedules' }
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }
