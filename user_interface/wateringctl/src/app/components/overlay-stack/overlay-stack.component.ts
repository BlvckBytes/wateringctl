import { Component, ElementRef, EmbeddedViewRef, OnDestroy } from '@angular/core';
import { KeyEventsService } from 'src/app/services/key-events.service';
import { OverlaysService } from 'src/app/services/overlays.service';
import { SubSink } from 'subsink';

@Component({
  selector: 'app-overlay-stack',
  templateUrl: './overlay-stack.component.html',
  styleUrls: ['./overlay-stack.component.scss']
})
export class OverlayStackComponent implements OnDestroy {

  private _subs = new SubSink();

  constructor(
    overlaysS: OverlaysService,
    keyS: KeyEventsService,
    ref: ElementRef
  ) {
    this._subs.sink = overlaysS.overlays$.subscribe(i => {
      const h = ref.nativeElement;

      // Clear holder
      while (h.firstChild)
        h.removeChild(h.firstChild);

      // Loop items indexed
      i.forEach(([ov, item], index) => {
        const elem = (item.hostView as EmbeddedViewRef<any>).rootNodes[0] as HTMLElement;

        // Create wrapper with matching z-index
        const wrapper = document.createElement('div');
        wrapper.className = 'overlay hv-c';
        wrapper.style.zIndex = index.toString();
        elem.onclick = (e) => e.stopPropagation();
        wrapper.appendChild(elem);

        if (ov.userClosable)
          wrapper.onclick = () => overlaysS.destroy(i[index]);

        // Append wrapper to host
        h.appendChild(wrapper);
      });
    });

    // Listen for escape key hits
    this._subs.sink = keyS.key$.subscribe(e => {
      if (e.key !== 'Escape') return;
      overlaysS.destroyLatest();
    });
  }

  ngOnDestroy(): void {
    this._subs.unsubscribe();
  }
}
