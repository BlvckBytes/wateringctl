import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';

import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { NavigatorComponent } from './components/navigator/navigator.component';
import { PageValvesComponent } from './pages/page-valves/page-valves.component';
import { PageSchedulesComponent } from './pages/page-schedules/page-schedules.component';
import { WeekdaySelectComponent } from './components/weekday-select/weekday-select.component';
import { ButtonComponent } from './components/button/button.component';

@NgModule({
  declarations: [
    AppComponent,
    NavigatorComponent,
    PageValvesComponent,
    PageSchedulesComponent,
    WeekdaySelectComponent,
    ButtonComponent
  ],
  imports: [
    BrowserModule,
    AppRoutingModule
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
