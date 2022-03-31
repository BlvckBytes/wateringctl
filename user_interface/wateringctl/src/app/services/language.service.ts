import { Injectable } from '@angular/core';
import { TranslateService } from '@ngx-translate/core';
import { ITranslationLanguage } from '../models/translation-language.interface';

@Injectable({
  providedIn: 'root',
})
export class LanguageService {

  private languages: ITranslationLanguage[];
  private langKey = 'wctl-lang';

  constructor(
    private tranService: TranslateService,
  ) {
    this.languages = [
      { internalName: 'en', displayName: 'English', icon: "flag-en.svg", isActive: true },
      { internalName: 'de', displayName: 'Deutsch', icon: "flag-de.svg", isActive: false },
    ];

    // Register known languages
    tranService.addLangs(this.languages.map(it => it.internalName));
    this.decideInitialLang();
  }

  decideInitialLang() {
    const chosenLang = localStorage.getItem(this.langKey);
    if (chosenLang) {
      this.chooseLanguage(chosenLang);
      return;
    }

    // Use browser language if known, else fall back to default language
    // Don't save this "implicit" choice, to allow for future auto-detect
    const browserLang = this.tranService.getBrowserLang() || 'en';
    if (this.languages.some(it => it.internalName === browserLang))
      this.chooseLanguage(browserLang, false);
    else
      this.chooseLanguage(this.tranService.defaultLang, false);
  }

  chooseLanguage(internalName: string, saveLocal: boolean = true) {
    const target = this.languages.find(it => it.internalName === internalName);

    // Language unknown
    if (!target) return;
    this.tranService.use(internalName);
    this.languages.forEach(it => it.isActive = false);
    target.isActive = true;

    // Save to local storage
    if (saveLocal)
      localStorage.setItem(this.langKey, internalName);
  }

  nextLanguage(saveLocal: boolean = true) {
    let i = 0;
    for (; i < this.languages.length; i += 1)
      if (this.languages[i].isActive) break;

    i = (i + 1) % this.languages.length;
    this.chooseLanguage(this.languages[i].internalName, saveLocal);
  }

  listLanguages(): ITranslationLanguage[] {
    return this.languages.slice();
  }

  selectedLanguage(): ITranslationLanguage {
    return this.languages.find(it => it.isActive)!;
  }
}