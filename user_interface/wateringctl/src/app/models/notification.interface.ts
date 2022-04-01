import { INotificationButton } from './notification-button.interface';

export interface INotification {
  headline: string;
  text: string;
  icon: string;
  color: 'success' | 'warning';
  timeout: number | undefined;
  buttons: INotificationButton[];
}