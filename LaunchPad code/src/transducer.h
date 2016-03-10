#ifndef TRANSDUCER_H
#define TRANSDUCER_H

#include <stdint.h>

extern uint16_t transducer_val[4];

void transducer_init(void);
void transducer_periodic(void);

#endif /* TRANSDUCER_H */
