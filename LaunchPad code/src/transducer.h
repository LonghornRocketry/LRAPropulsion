#ifndef TRANSDUCER_H
#define TRANSDUCER_H

#include <stdint.h>

extern volatile uint16_t transducer_val[16];

void transducer_init(void);

// transdcuer_periodic() must be called with at least ~5 uS
// between calls (200 kHz)
void transducer_periodic(void);

#endif /* TRANSDUCER_H */
