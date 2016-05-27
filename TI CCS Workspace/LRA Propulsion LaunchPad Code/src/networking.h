#ifndef NETWORKING_H
#define NETWORKING_H

void networking_init(void);

void uip_appcall(void);
void uip_udp_appcall(void);
void uip_log(char* str);

#endif /* NETWORKING_H */
