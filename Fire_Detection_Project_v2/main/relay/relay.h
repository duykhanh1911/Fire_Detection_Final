#ifndef RELAY_H
#define RELAY_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "driver/gpio.h"

#include "io_def/io_def.h"

#define RELAY_ON 0
#define RELAY_OFF 1

void relay_init();

void relay_set_status_buzzer(bool level);

void relay_set_status_pump(bool level);

#endif // RELAY_H