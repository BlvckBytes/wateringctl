import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import { IValveRequest } from '../models/request/valve-request.interface';
import { IValve } from '../models/valve.interface';
import { HttpService } from './http.service';

@Injectable({
  providedIn: 'root'
})
export class ValvesService {

  constructor(
    private httpService: HttpService,
  ) {}

  getAllValves(): Observable<IValve[]> {
    return this.httpService.get('/valves');
  }

  putValve(
    id: number,
    valve: IValveRequest
  ): Observable<IValve> {
    return this.httpService.put(`/valves/${id}`, valve);
  }

  activateValve(
    id: number
  ): Observable<void> {
    return this.httpService.post(`/valves/${id}`, null);
  }

  deactivateValve(
    id: number
  ): Observable<void> {
    return this.httpService.delete(`/valves/${id}`);
  }
}
