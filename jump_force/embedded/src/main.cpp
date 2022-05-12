#include <Arduino.h>

#include <arduino-timer.h>

#include "ble.h"

#define BAUD_RATE 115200

Timer<> main_timer = timer_create_default();

void setup()
{
  Serial.begin(BAUD_RATE);

  setup_ble();
  delay(500);
}

void loop()
{
  main_timer.tick();
}
