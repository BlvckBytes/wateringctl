import { Component } from '@angular/core';
import { INotificationButton } from 'src/app/models/notification-button.interface';
import { INotification } from 'src/app/models/notification.interface';
import { NotificationsService } from 'src/app/services/notifications.service';

@Component({
  selector: 'app-notification-center',
  templateUrl: './notification-center.component.html',
  styleUrls: ['./notification-center.component.scss']
})
export class NotificationCenterComponent {

  items: INotification[] = [];

  constructor(
    private notificationsS: NotificationsService,
  ) {
    notificationsS.items$.subscribe(i => this.items = i);
  }

  destroy(item: INotification) {
    this.notificationsS.destroy(item);
  }

  buttonClick(item: INotification, button: INotificationButton) {
    button.callback?.();

    if (item.destroyOnButtons)
      this.notificationsS.destroy(item);
  }
}
