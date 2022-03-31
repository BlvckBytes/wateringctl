import { Component, OnDestroy } from '@angular/core';
import { NavigationEnd, Router } from '@angular/router';
import { SubSink } from 'subsink';
import { INavigatorLink } from './navigatior-link.interface';

@Component({
  selector: 'app-navigator',
  templateUrl: './navigator.component.html',
  styleUrls: ['./navigator.component.scss']
})
export class NavigatorComponent implements OnDestroy {

  private subs = new SubSink();

  currUrl = '/';
  links: INavigatorLink[] = [
    { link: '/valves', icon: 'valve.svg' },
    { link: '/schedules', icon: 'clock.svg' }
  ];

  constructor(
    router: Router
  ) {
    this.subs.sink = router.events.subscribe(re => {
      if (!(re instanceof NavigationEnd)) return;
      this.currUrl = re.urlAfterRedirects;
    });
  }

  ngOnDestroy() {
    this.subs.unsubscribe();
  }
}
