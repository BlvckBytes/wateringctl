import { Component, OnInit } from '@angular/core';
import { ActivatedRoute } from '@angular/router';
import { WebSocketFsService } from 'src/app/services/web-socket-fs.service';
import { Location } from '@angular/common'

@Component({
  selector: 'app-page-edit',
  templateUrl: './page-edit.component.html',
  styleUrls: ['./page-edit.component.scss']
})
export class PageEditComponent implements OnInit {

  contents: string = '';

  constructor(
    private route: ActivatedRoute,
    private loc: Location,
    private fsService: WebSocketFsService,
  ) {}

  ngOnInit(): void {
    this.route.params.subscribe(params => {
      this.fsService.readFile(params['name']).subscribe(bin => {
        this.contents = new TextDecoder().decode(bin);
      });
    });
  }

  back() {
    this.loc.back();
  }
}
