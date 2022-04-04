export class ITranslationLanguage {

  constructor(
    public internalName: string,
    public displayName: string,
    public icon: string,
    public isActive: boolean,
  ) {}
}