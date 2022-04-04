import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { PageBrowserComponent } from './pages/page-browser/page-browser.component';
import { PageEditComponent } from './pages/page-edit/page-edit.component';

const routes: Routes = [
  { path: 'browser', component: PageBrowserComponent },
  { path: 'edit', component: PageEditComponent },
  { path: '**', redirectTo: 'browser' }
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }
