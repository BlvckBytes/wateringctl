import { AfterViewInit, ChangeDetectorRef, Component, DoCheck, HostBinding, HostListener, Input, OnDestroy, OnInit, ViewChild } from '@angular/core';
import { AbstractControl, AsyncValidatorFn, ControlValueAccessor, FormControl, NG_VALUE_ACCESSOR, ValidationErrors } from '@angular/forms';
import { BehaviorSubject, merge, Observable, of } from 'rxjs';
import { distinctUntilChanged, skip, startWith } from 'rxjs/operators';
import { SubSink } from 'subsink';

@Component({
  selector: 'app-textbox',
  templateUrl: './textbox.component.html',
  styleUrls: ['./textbox.component.scss'],
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: TextboxComponent,
      multi: true,
    }
  ]
})
export class TextboxComponent implements AfterViewInit, DoCheck, OnDestroy, OnInit {

  @Input() placeholder?: string;
  @Input() icon?: string;
  @Input() type = 'text';
  @Input() showValidation = true;
  @Input() clearable = false;
  @Input() control!: FormControl;
  @Input() focused = false;
  @Input() typeahead: string[] = [];

  // #region Class binding flags

  @Input()
  @HostBinding('class.--locked')
  locked = false;

  @Input()
  @HostBinding('class.--grayed')
  grayed = false;

  @Input()
  @HostBinding('class.--volatile-placeholder')
  volatilePlaceholder = false;

  @Input()
  @HostBinding('class.--dark')
  dark = false;

  @HostBinding('class.--has-content')
  hasContent = false;

  @HostBinding('class.--valid')
  isValid = false;

  @HostBinding('class.--invalid')
  isInvalid = false;

  // #endregion

  private subs = new SubSink();
  private touched$ = new BehaviorSubject(false);

  suggestions: string[] = [];

  constructor (
    private cd: ChangeDetectorRef,
  ) {}

  // #region Lifecycle hooks

  ngDoCheck() {
    // Check if touched has changed
    if (this.touched$.value !== this.control.touched)
      this.touched$.next(true);
  }

  ngAfterViewInit() {
    this.subs.sink = merge(
      // Only call when touched changed
      this.touched$.pipe(
        startWith(false),
        distinctUntilChanged(),
        skip(1),
      ),
      // Also call on value or validity changes
      this.control.valueChanges,
      this.control.statusChanges,
    ).subscribe(() => this.updateModifiers());
  }

  private matchesSuggestion() {
    return this.typeahead.find(it => it === this.control.value) !== undefined;
  }

  private suggestionValidator(): AsyncValidatorFn {
    return (_: AbstractControl): Observable<ValidationErrors | null> => {
      if (this.matchesSuggestion())
        return of(null);

      return of({
        suggestionmismatch: true
      });
    };
  }

  ngOnInit(): void {
    this.updateModifiers();

    // Apply typeahead validator if applicable
    if (this.typeahead.length > 0)
      this.control.addAsyncValidators(this.suggestionValidator());
  }

  ngOnDestroy() {
    this.subs.unsubscribe();
  }

  // #endregion

  // #region Modifying classes

  /**
   * Update modifier class flags based on textbox's state
   */
  private updateModifiers() {
    // Neither empty nor null
    const currValue = this.control.value;
    this.hasContent = currValue !== '' && this.control.value !== null;

    // Needs to be touched or have content in order to be applied
    this.isValid = (this.control.touched || this.hasContent) && this.control.valid && this.showValidation;
    this.isInvalid = (this.control.touched || this.hasContent) && !this.control.valid && this.showValidation;

    // Only suggest when content is present
    this.suggestions = [];
    if (this.hasContent) {
      // Don't suggest when an ignore-case match is already typed out
      if (!this.matchesSuggestion())
        this.suggestions = this.typeahead
          .filter(it => it.toLowerCase().includes(currValue.toLowerCase()));
    }

    this.cd.detectChanges();
  }
  // #endregion

  // #region Events
  @HostListener('keydown', ['$event'])
  onKeyDown(e: any) {
    if (e.key !== 'Escape' || !this.clearable) return;
    this.clear();
  }

  // #endregion

  clear() {
    this.control.setValue('');
  }

  applySuggestion(suggestion: string) {
    this.control.setValue(suggestion);
  }

  listErrors(): string[] {
    return Object.keys(this.control.errors || {});
  }

  errorDetails(error: string): any {
    return this.control.errors?.[error];
  }
}
