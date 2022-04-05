import {
  AfterViewInit,
  ChangeDetectionStrategy,
  Component,
  DoCheck,
  ElementRef,
  EventEmitter,
  forwardRef,
  Input,
  KeyValueDiffer,
  KeyValueDiffers,
  NgZone,
  OnDestroy,
  Output,
  ViewChild,
} from '@angular/core';
import { ControlValueAccessor, NG_VALUE_ACCESSOR } from '@angular/forms';
import * as CodeMirror from 'codemirror';
import { Editor, EditorChange, EditorFromTextArea, ScrollInfo } from 'codemirror';
import { forkJoin, Observable, Subscriber } from 'rxjs';

/*
  This file has been "forked" from ngx-codemirror: https://github.com/scttcper/ngx-codemirror, where
  afterwards the code has been cleaned up and the load-mode-on-demand feature has been implemented
*/

@Component({
  selector: 'ngx-codemirror',
  templateUrl: './codemirror.component.html',
  styleUrls: ['./codemirror.component.scss'],
  providers: [
    {
      provide: NG_VALUE_ACCESSOR,
      useExisting: forwardRef(() => CodemirrorComponent),
      multi: true,
    },
  ],
  preserveWhitespaces: false,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class CodemirrorComponent
  implements AfterViewInit, OnDestroy, ControlValueAccessor, DoCheck
{
  /*
  ============================================================================
                               Private Properties                             
  ============================================================================
  */

  // Static map of watchers that have interest in a codemirror mode's loading completion
  // This is used for efficient mode dependency-ensurement
  private static _modeLoadingWatchers: { [key: string]: Subscriber<void>[] } = {};

  private _options: any;                                 // Codemirror options
  private _optionsDiffer?: KeyValueDiffer<string, any>;  // Differ for codemirror options object
  private _onChange = (_: any) => {};                    // ControlValueAccessor
  private _onTouched = () => {};                         // ControlValueAccessor;
  private _codeMirrorLib: any;                           // either global variable or required library

  /*
  ============================================================================
                                Public Properties                             
  ============================================================================
  */

  @ViewChild('textarea') textarea!: ElementRef<HTMLTextAreaElement>;  // Actual textarea
  codeMirror?: EditorFromTextArea;                                    // Codemirror from text area
  isFocused = false;                                                  // Current focus state for textarea class binding
  value = '';                                                         // Current text content

  /**
   * Either initially load or get the loaded codemirror lib ref
   */
  get codeMirrorLib(): any {
    // Library already loaded
    if (this._codeMirrorLib)
      return this._codeMirrorLib;

    // in order to allow for universal rendering, we import Codemirror runtime with `import` to prevent node errors
    this._codeMirrorLib = typeof CodeMirror !== 'undefined' ? CodeMirror : import('codemirror');

    // Make codemirror accessible globally for loaded modules
    window.CodeMirror = this._codeMirrorLib;

    return this._codeMirrorLib;
  }

  /*
  ============================================================================
                                     Inputs                                   
  ============================================================================
  */
  
  @Input() className = '';                  // class applied to the created textarea/
  @Input() name = 'codemirror';             // name applied to the created textarea
  @Input() autoFocus = false;               // autofocus setting applied to the created textarea
  @Input() preserveScrollPosition = false;  // preserve previous scroll position after updating value
  @Input() modesPath = '/cmm/%N.js';        // Assets path for the .js mode files, %N is a name placeholder

  /**
   * set options for codemirror
   * @link http://codemirror.net/doc/manual.html#config
   */
  @Input()
  set options(value: { [key: string]: any }) {
    this._options = value;

    // Create kv differ initially
    if (!this._optionsDiffer && value)
      this._optionsDiffer = this._differs.find(value).create();
  }

  /*
  ============================================================================
                                     Outputs                                  
  ============================================================================
  */

  @Output() cursorActivity = new EventEmitter<Editor>();    // called when the text cursor is moved
  @Output() focusChange = new EventEmitter<boolean>();      // called when the editor is focused or loses focus
  @Output() scroll = new EventEmitter<ScrollInfo>();        // called when the editor is scrolled
  @Output() drop = new EventEmitter<[Editor, DragEvent]>();  // called when file(s) are dropped

  /*
  ============================================================================
                             Dependency Injection                             
  ============================================================================
  */

  constructor(
    private _differs: KeyValueDiffers,
    private _ngZone: NgZone
  ) {}

  /*
  ============================================================================
                                 Mode Autoloading                             
  ============================================================================
  */

  endLoading(mode: string) {
    // Notify all watchers
    CodemirrorComponent._modeLoadingWatchers[mode].forEach(it => {
      it.next();
      it.complete();
    });

    // Remove map entry
    delete CodemirrorComponent._modeLoadingWatchers[mode];
  }

  ensureDeps (mode: string): void {
    // Get the mode's dependencies
    const deps = this._codeMirrorLib.modes[mode].dependencies as string[];

    // This mode has no dependencies
    if (!deps) {
      this.endLoading(mode);
      return;
    }

    // Filter out already loaded dependencies
    const missing = deps.filter(it => !this._codeMirrorLib.modes.hasOwnProperty(it));

    // No more missing dependencies
    if (missing.length == 0) {
      this.endLoading(mode);
      return;
    }

    // Wait for all dependencies to load
    forkJoin(missing.map(it => this.requireMode(it)))
      .subscribe(() => this.endLoading(mode));
  }

  requireMode (mode: string): Observable<void> {
    return new Observable(obs => {
      // Already required, load it's dependencies
      if (this._codeMirrorLib.modes.hasOwnProperty(mode)) {
        this.ensureDeps(mode);
        return;
      }

      // Already loading this mode, add observer to watcher-list
      if (CodemirrorComponent._modeLoadingWatchers.hasOwnProperty(mode)) {
        CodemirrorComponent._modeLoadingWatchers[mode].push(obs);
        return;
      }

      // Create new script tag targetting the required mode's JS file
      const script = document.createElement('script');
      script.src = this.modesPath.replace("%N", mode);

      // Script loaded, load it's dependencies
      script.onload = () => this.ensureDeps(mode);

      // Insert before the topmost script
      const topmostScript = document.getElementsByTagName('script')[0];
      topmostScript?.parentElement?.insertBefore(script, topmostScript);

      // Register in loading-table
      CodemirrorComponent._modeLoadingWatchers[mode] = [obs];
    });
  };

  /*
  ============================================================================
                                    Lifecycles                                
  ============================================================================
  */

  ngAfterViewInit() {
    this._ngZone.runOutsideAngular(async () => {
      const codeMirrorObj = await this.codeMirrorLib;
      const codeMirror = codeMirrorObj?.default ? codeMirrorObj.default : codeMirrorObj;

      // Create codemirror from the textarea component
      this.codeMirror = codeMirror.fromTextArea(
        this.textarea.nativeElement,
        this._options,
      ) as EditorFromTextArea;
      
      // Bind codemirror events to local callbacks/emitters
      this.codeMirror.on('cursorActivity', cm => this._ngZone.run(() => this.cursorActive(cm)));
      this.codeMirror.on('scroll', this.scrollChanged.bind(this));
      this.codeMirror.on('blur', () => this._ngZone.run(() => this.focusChanged(false)));
      this.codeMirror.on('focus', () => this._ngZone.run(() => this.focusChanged(true)));
      this.codeMirror.on('change', (cm, change) => this._ngZone.run(() => this.codemirrorValueChanged(cm, change)),);
      this.codeMirror.on('drop', (cm, e) => this._ngZone.run(() => this.dropFiles(cm, e)));
      
      // Set initial value
      this.codeMirror.setValue(this.value);
    });
  }

  ngDoCheck() {
    // Differ not initialized
    if (!this._optionsDiffer)
      return;

    // check if options have changed, apply
    const changes = this._optionsDiffer.diff(this._options);
    if (changes) {
      changes.forEachChangedItem(option => this.setOptionIfChanged(option.key, option.currentValue));
      changes.forEachAddedItem(option => this.setOptionIfChanged(option.key, option.currentValue));
      changes.forEachRemovedItem(option => this.setOptionIfChanged(option.key, option.currentValue));
    }
  }

  ngOnDestroy() {
    // is there a lighter-weight way to remove the cm instance?
    if (this.codeMirror) {
      this.codeMirror.toTextArea();
    }
  }

  /*
  ============================================================================
                               Codemirror Events                              
  ============================================================================
  */

  codemirrorValueChanged(cm: Editor, change: EditorChange) {
    const cmVal = cm.getValue();
    if (this.value !== cmVal) {
      this.value = cmVal;
      this._onChange(this.value);
    }
  }

  focusChanged(focused: boolean) {
    this._onTouched();
    this.isFocused = focused;
    this.focusChange.emit(focused);
  }

  scrollChanged(cm: Editor) {
    this.scroll.emit(cm.getScrollInfo());
  }

  cursorActive(cm: Editor) {
    this.cursorActivity.emit(cm);
  }

  dropFiles(cm: Editor, e: DragEvent) {
    this.drop.emit([cm, e]);
  }

  /*
  ============================================================================
                                   Utilities                                  
  ============================================================================
  */

  normalizeLineEndings(str: string): string {
    if (!str)
      return str;

    // Either carriage return newline or just carriage return should be newlines
    return str.replace(/\r\n|\r/g, '\n');
  }

  setOptionIfChanged(optionName: string, newValue: any) {
    // Autoload mode, if required (even if codemirror is not yet initialized)
    if (optionName === 'mode' && newValue) {
      // Mode has already been loaded
      if (this.codeMirrorLib.modes.hasOwnProperty(newValue))
        return;

        // Load mode, then apply if codemirror has been initialized
      this.requireMode(newValue).subscribe(() => {
        this.codeMirror?.setOption('mode', this.codeMirror.getOption('mode'));
      });
    }

    // cast to any to handle strictly typed option names
    // could possibly import settings strings available in the future
    this.codeMirror?.setOption(optionName as any, newValue);
  }

  /*
  ============================================================================
                              ControlValueAccessor                            
  ============================================================================
  */

  writeValue(value: string) {
    // No value received
    if (value === null || value === undefined)
      return;

    // Codemirror not yet loaded, cache value to be set later on
    if (!this.codeMirror) {
      this.value = value;
      return;
    }

    const currentValue = this.codeMirror.getValue();
    if (
      value !== currentValue && // Value differs from current value
      this.normalizeLineEndings(currentValue) !== this.normalizeLineEndings(value) // Not just by line endings
    ) {
      // Update value
      this.value = value;

      // Check if scroll-pos should be preserved and cache it before setting, if applicable
      if (this.preserveScrollPosition) {
        const prevScrollPosition = this.codeMirror.getScrollInfo();
        this.codeMirror.setValue(this.value);
        this.codeMirror.scrollTo(prevScrollPosition.left, prevScrollPosition.top);
      }

      // Just set the value
      else
        this.codeMirror.setValue(this.value);
    }
  }

  registerOnChange(fn: (value: string) => void) {
    this._onChange = fn;
  }

  registerOnTouched(fn: () => void) {
    this._onTouched = fn;
  }

  setDisabledState(isDisabled: boolean) {
    this.setOptionIfChanged('readOnly', isDisabled);
  }
}
