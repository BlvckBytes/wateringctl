import { Component, HostBinding, Input } from '@angular/core';
import { OverlaysService } from 'src/app/services/overlays.service';
import { PathBarService } from 'src/app/services/path-bar.service';
import { OverlayFileUploadComponent } from '../overlays/overlay-file-upload/overlay-file-upload.component';
import { OverlayFolderCreateComponent } from '../overlays/overlay-folder-create/overlay-folder-create.component';
import { IPathPart } from './path-part.interface';

@Component({
  selector: 'app-path-bar',
  templateUrl: './path-bar.component.html',
  styleUrls: ['./path-bar.component.scss']
})
export class PathBarComponent {

  pathParts: IPathPart[] = [];

  get canGoUp() {
    return this.pathParts.length > 1;
  }

  @Input()
  @HostBinding('class.--disabled')
  disabled: boolean = false;

  constructor(
    private overlaysService: OverlaysService,
    private pathBarService: PathBarService,
  ) {
    pathBarService.path$.subscribe(path => {
      // Split individual folders
      this.pathParts = path.split('/')
        .filter(it => it !== '')
        .map(it => ({
          name: it,
          icon: 'folder.svg'
        }))

      // Always start with root
      this.pathParts.unshift({
        name: 'root',
        icon: 'disk.svg'
      });
    })
  }

  navigateTo(partsIndex: number) {
    if (this.disabled) return;

    this.pathBarService.navigateTo(
      partsIndex == 0 ? '/' : this.pathParts
      .slice(1, partsIndex + 1)
      .reduce((acc, curr) => acc + '/' + curr.name, '')
    );
  }

  openUploadOverlay(): void {
    if (this.disabled) return;

    this.overlaysService.publish({
      component: OverlayFileUploadComponent,
      inputs: [],
      userClosable: true,
    });
  }

  openFolderOverlay(): void {
    if (this.disabled) return;

    this.overlaysService.publish({
      component: OverlayFolderCreateComponent,
      inputs: [],
      userClosable: true,
    });
  }

  up(): void {
    if (!this.canGoUp)
      return;

    // Navigate back to root
    if (this.pathParts.length == 2)
      this.pathBarService.navigateTo("/");

    // Navigate to previous folder
    else {
      this.pathBarService.navigateTo(
        '/' +
        this.pathParts
          .slice(1, this.pathParts.length - 1)
          .map(it => it.name)
          .join('/')
      );
    }
  }
}
