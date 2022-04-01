import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { HttpClient, HttpClientModule } from '@angular/common/http';
import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { NavigatorComponent } from './components/navigator/navigator.component';
import { PageValvesComponent } from './pages/page-valves/page-valves.component';
import { PageSchedulesComponent } from './pages/page-schedules/page-schedules.component';
import { TranslateModule, TranslateLoader } from '@ngx-translate/core';
import { TranslateHttpLoader } from '@ngx-translate/http-loader';
import { WeekdaySelectComponent } from './components/weekday-select/weekday-select.component';
import { ButtonComponent } from './components/button/button.component';
import { NotificationCenterComponent } from './components/notification-center/notification-center.component';

@NgModule({
  declarations: [
    AppComponent,
    NavigatorComponent,
    PageValvesComponent,
    PageSchedulesComponent,
    WeekdaySelectComponent,
    ButtonComponent,
    NotificationCenterComponent,
  ],
  imports: [
    BrowserModule,
    AppRoutingModule,
    HttpClientModule,
    TranslateModule.forRoot({
      loader: {
        provide: TranslateLoader,
        useFactory: (http: HttpClient) => new TranslateHttpLoader(http, '/i18n/'),
        deps: [HttpClient],
      },
    })
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
