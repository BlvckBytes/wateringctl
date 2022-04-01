import { AfterViewInit, ChangeDetectorRef, Component, DoCheck, HostBinding, HostListener, Input, OnDestroy, OnInit, ViewChild } from '@angular/core';
import { ControlValueAccessor, FormControl, NG_VALUE_ACCESSOR } from '@angular/forms';
import { BehaviorSubject, merge } from 'rxjs';
import { distinctUntilChanged, skip, startWith } from 'rxjs/operators';
import { SubSink } from 'subsink';
import { TextboxSize } from './textbox-size.type';

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
  @Input() size: TextboxSize = 'large';
  @Input() showValidation = true;
  @Input() clearable = false;
  @Input() control!: FormControl;

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

  ngOnInit(): void {
    this.updateModifiers();
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
    this.hasContent = this.control.value !== '' && this.control.value !== null;

    // Needs to be touched or have content in order to be applied
    this.isValid = (this.control.touched || this.hasContent) && this.control.valid && this.showValidation;
    this.isInvalid = (this.control.touched || this.hasContent) && !this.control.valid && this.showValidation;

    this.cd.detectChanges();
  }

  /**
   * Create a class string based on current flags
   * @param additional Additional classes
   * @returns Class string to be bound
   */
  mkClasses(additional = '') {
    const pr = "textbox--";
    return `${pr}${this.size} ${additional}`;
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

  listErrors(): string[] {
    return Object.keys(this.control.errors || {});
  }

  errorDetails(error: string): any {
    return this.control.errors?.[error];
  }
}
