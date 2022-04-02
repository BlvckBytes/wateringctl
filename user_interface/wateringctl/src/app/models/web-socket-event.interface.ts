import { EWebSocketEventType } from './web-socket-event-type.enum';

export interface IWebSocketEvent {
  type: EWebSocketEventType;
  args: string[];
}