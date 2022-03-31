import { Injectable } from '@angular/core';
import { BehaviorSubject, map, Observable } from 'rxjs';
import { IValveRequest } from '../models/request/valve-request.interface';
import { IValve } from '../models/valve.interface';
import { HttpService } from './http.service';

@Injectable({
  providedIn: 'root'
})
export class ValvesService {

  allValves = new BehaviorSubject<IValve[] | null>(null);

  constructor(
    private httpService: HttpService,
  ) {}

  getAllValves(): Observable<IValve[]> {
    return new Observable<IValve[]>(s => {
      this.httpService.get<any>('/valves').pipe(map(it => it.items)).subscribe(v => {
        this.allValves.next(v);
        s.next(v);
      });
    });
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
