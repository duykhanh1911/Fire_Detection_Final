#include "relay.h"

void relay_init()
{
  gpio_config_t io_conf = {};
  io_conf.pin_bit_mask = (1ULL << PIN_NUM_BUZZER) | (1ULL << PIN_NUM_PUMP);
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_up_en = true;
  gpio_config(&io_conf);
  relay_set_status_buzzer(RELAY_OFF);
  relay_set_status_pump(RELAY_OFF);
}

void relay_set_status_buzzer(bool level)
{
  gpio_set_level(PIN_NUM_BUZZER, level);
}

void relay_set_status_pump(bool level)
{
  gpio_set_level(PIN_NUM_PUMP, level);
}