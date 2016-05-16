#ifndef __SOLENOID_H
#define __SOLENOID_H

#include <stdbool.h>

void solenoid_init(void);
void solenoid_periodic(void);
bool are_solenoids_powered(void);

#endif /* __SOLENOID_H */
