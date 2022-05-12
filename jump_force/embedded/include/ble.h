#ifndef BLE
#define BLE

#include "common.h"
#include "config.h"

#include <BLEServer.h>

void setup_ble();
void send_message(std::string message);

#endif
