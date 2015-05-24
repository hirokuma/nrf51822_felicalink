#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

void gpiote_irq_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low);

#endif /* MAIN_H */
