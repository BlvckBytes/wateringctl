import { ApplicationRef, ComponentFactoryResolver, ComponentRef, Injectable, Injector } from '@angular/core';
import { NavigationStart, Router } from '@angular/router';
import { BehaviorSubject } from 'rxjs';
import { IOverlay } from '../models/overlay.interface';

export type ComponentOverlay = [IOverlay, ComponentRef<any>];

@Injectable({
  providedIn: 'root'
})
export class OverlaysService {

  private items: ComponentOverlay[] = [];
  private _overlays$ = new BehaviorSubject(this.items);

  constructor(
    private compFacRes: ComponentFactoryResolver,
    private inj: Injector,
    private appRef: ApplicationRef,
    router: Router,
  ) {
    // Remove overlays on navigation
    router.events.subscribe(e => {
      if(e instanceof NavigationStart)
        this.destroyAll();
    });
  }

  get overlays$() {
    return this._overlays$.asObservable();
  }

  /**
   * Instantiates an overlay from it's basic data
   * @param item Basic data for the overlay construction
   * @returns Constructed component
   */
  private instantiate(item: IOverlay): ComponentOverlay {
    // Create instance of component
    const fac = this.compFacRes.resolveComponentFactory(item.component);
    const comp = fac.create(this.inj);
    const inst = comp.instance as any;

    // Apply inputs
    for (const key of Object.keys(item.inputs)) {
      // Only apply if the input object truly has this property (not in prototype chain)
      // and if the target component has a corresponding field to apply to
      if (inst.hasOwnProperty(key) && item.inputs.hasOwnProperty(key))
        inst[key] = item.inputs[key];
    }

    // Attach
    this.appRef.attachView(comp.hostView);
    return [item, comp];
  }

  /**
   * Publish a new overlay to the stack to be rendered
   * @param item Overlay to publish
   */
  publish(item: IOverlay): ComponentOverlay {
    const inst = this.instantiate(item);
    this.items.push(inst);
    this._overlays$.next(this.items);
    return inst;
  }

  /**
   * Destroy an existing overlay by it's list-entry
   * @param item Entry within overlays$' list
   * @returns True if destroyed, false if not found
   */
  destroy(item: ComponentOverlay): boolean {
    // Skip if not existing
    const index = this.items.indexOf(item);
    if (index < 0) return false;

    // Remove from list
    this.items.splice(index, 1);
    this._overlays$.next(this.items);

    // Detatch and destroy
    this.appRef.detachView(item[1].hostView);
    item[1].destroy();
    return true;
  }

  /**
   * Destroy the latest added overlay in the stack
   * @param byUser Whether or not this is a user action
   * @returns True if destroyed, false if not found
   */
  destroyLatest(byUser = true): boolean {
    if (this.items.length === 0) return false;
    const latest = this.items[this.items.length - 1];

    // Prevent if not user closable
    if (byUser && !latest[0].userClosable) return false;

    return this.destroy(latest);
  }

  /**
   * Destroy all available overlays on the stack
   */
   destroyAll(): void {
    for (const item of this.items)
      this.destroy(item);
  }
}
