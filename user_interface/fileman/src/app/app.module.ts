import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { TranslateModule, TranslateLoader } from '@ngx-translate/core';
import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { PageBrowserComponent } from './pages/page-browser/page-browser.component';
import { PageEditComponent } from './pages/page-edit/page-edit.component';
import { PathBarComponent } from './components/path-bar/path-bar.component';
import { ButtonComponent } from './components/button/button.component';
import { NotificationCenterComponent } from './components/notification-center/notification-center.component';
import { OverlayStackComponent } from './components/overlay-stack/overlay-stack.component';
import { OverlayFolderCreateComponent } from './components/overlays/overlay-folder-create/overlay-folder-create.component';
import { OverlayFileUploadComponent } from './components/overlays/overlay-file-upload/overlay-file-upload.component';
import { TextboxComponent } from './components/textbox/textbox.component';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { HttpClient, HttpClientModule } from '@angular/common/http';
import { TranslateHttpLoader } from '@ngx-translate/http-loader';
import { NgxDropzoneModule } from 'ngx-dropzone';
import { AutofocusDirective } from './directives/autofocus.directive';
import { CodemirrorComponent } from './components/codemirror/codemirror.component';

import 'codemirror/mode/meta';
import { OverlayFileCreateComponent } from './components/overlays/overlay-file-create/overlay-file-create.component';
import { SortableTableComponent } from './components/sortable-table/sortable-table.component';
import { BrowserFileActionsComponent } from './pages/page-browser/browser-file-actions/browser-file-actions.component';
import { NgViewInitDirective } from './directives/ng-view-init.directive';
import { BrowserFileIconComponent } from './pages/page-browser/browser-file-icon/browser-file-icon.component';

@NgModule({
  declarations: [
    AppComponent,
    PageBrowserComponent,
    PageEditComponent,
    PathBarComponent,
    ButtonComponent,
    NotificationCenterComponent,
    OverlayStackComponent,
    OverlayFolderCreateComponent,
    OverlayFileUploadComponent,
    TextboxComponent,
    AutofocusDirective,
    CodemirrorComponent,
    OverlayFileCreateComponent,
    SortableTableComponent,
    BrowserFileActionsComponent,
    NgViewInitDirective,
    BrowserFileIconComponent,
  ],
  imports: [
    BrowserModule,
    AppRoutingModule,
    ReactiveFormsModule,
    FormsModule,
    HttpClientModule,
    NgxDropzoneModule,
    TranslateModule.forRoot({
      loader: {
        provide: TranslateLoader,
        useFactory: (http: HttpClient) => new TranslateHttpLoader(http, '/i18n/'),
        deps: [HttpClient],
      },
    }),
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
