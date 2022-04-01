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

  private subs = new SubSink();

  constructor(
    overlaysS: OverlaysService,
    keyS: KeyEventsService,
    ref: ElementRef
  ) {
    this.subs.sink = overlaysS.overlays$.subscribe(i => {
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
        wrapper.appendChild(elem);

        // Create close cross
        if (ov.userClosable) {
          const cross = document.createElement('img');
          cross.src = 'graphics/plus.svg';
          cross.className = 'overlay__close svg svg--md';
          cross.onclick = () => overlaysS.destroy(i[index]);
          elem.appendChild(cross);
        }

        // Append wrapper to host
        h.appendChild(wrapper);
      });
    });

    // Listen for escape key hits
    this.subs.sink = keyS.key$.subscribe(e => {
      if (e.key !== 'Escape') return;
      overlaysS.destroyLatest();
    });
  }

  ngOnDestroy(): void {
    this.subs.unsubscribe();
  }
}
