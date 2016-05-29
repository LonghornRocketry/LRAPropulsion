#ifndef __STATUS_LED_H
#define __STATUS_LED_H

void status_led_init(void);
void status_led_periodic(void);
void status_led_timer_interrupt(void);

#endif /* __STATUS_LED_H */
