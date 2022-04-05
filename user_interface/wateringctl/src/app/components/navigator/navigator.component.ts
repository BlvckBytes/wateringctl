import { Component, OnDestroy } from '@angular/core';
import { NavigationEnd, Router } from '@angular/router';
import { ITranslationLanguage } from 'src/app/models/translation-language.interface';
import { LanguageService } from 'src/app/services/language.service';
import { SubSink } from 'subsink';
import { INavigatorLink } from './navigatior-link.interface';

@Component({
  selector: 'app-navigator',
  templateUrl: './navigator.component.html',
  styleUrls: ['./navigator.component.scss']
})
export class NavigatorComponent implements OnDestroy {

  private _subs = new SubSink();

  currLang: ITranslationLanguage;
  currUrl = '/';
  links: INavigatorLink[] = [
    { link: '/valves', icon: 'valve.svg' },
    { link: '/schedules', icon: 'clock.svg' }
  ];

  constructor(
    router: Router,
    private langService: LanguageService,
  ) {
    this._subs.sink = router.events.subscribe(re => {
      if (!(re instanceof NavigationEnd)) return;
      this.currUrl = re.urlAfterRedirects;
    });

    this.currLang = langService.selectedLanguage();
  }

  nextLanguage() {
    this.langService.nextLanguage();
    this.currLang = this.langService.selectedLanguage();
  }

  ngOnDestroy() {
    this._subs.unsubscribe();
  }
}
