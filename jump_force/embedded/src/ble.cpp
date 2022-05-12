/**
 * @file ble.cpp
 * @author Joshua
 *
 * https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_server_multiconnect/BLE_server_multiconnect.ino
 * https://punchthrough.com/maximizing-ble-throughput-part-2-use-larger-att-mtu-2/
 * https://punchthrough.com/android-ble-guide/
 *
 */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>

#include <arduino-timer.h>

#include "common.h"
#include "ble.h"
#include "logger.h"

#define VOLTAGE_UPDATE_RATE 2 // seconds
#define VOLTAGE_PIN 37

bool deviceConnected = false;

TaskHandle_t ble_task;

BLEServer *server = NULL;
BLEService *service = NULL;
BLECharacteristic *message_send_characteristic = NULL;
BLECharacteristic *voltage_characteristic = NULL;

bool voltage_control_loop(void *params)
{
  if (!deviceConnected)
  {
    return true;
  }

  uint16_t voltage = analogRead(VOLTAGE_PIN) * 2.5;
  ss << voltage;
  voltage_characteristic->setValue(ss.str());
  clear_ss();
  voltage_characteristic->notify();

  return true;
}

class ServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    if (!deviceConnected)
    {
      return;
    }
    deviceConnected = false;
    delay(500);
    server->startAdvertising();
  }
};

class MessageReceiveCallbacks : public BLECharacteristicCallbacks
{
  void onNotify(BLECharacteristic *pCharacteristic)
  {
    log_message("received message");
  }
};

Timer<> ble_timer;

void setup_ble_main(void *params)
{
  BLEDevice::init(BLUETOOTH_NAME);
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  service = server->createService(SERVICE_UUID);

  message_send_characteristic = service->createCharacteristic(
      MESSAGE_SEND_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);

  BLECharacteristic *message_receive_characteristic = service->createCharacteristic(
      MESSAGE_RECEIVE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);
  message_receive_characteristic->setCallbacks(new MessageReceiveCallbacks());

  voltage_characteristic = service->createCharacteristic(
      VOLTAGE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06); // helps with iPhone connection issues
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  ss << "Bluetooth set up. Connect to \"" << BLUETOOTH_NAME << "\" on your phone.";
  log_message(ss.str());
  clear_ss();

  ble_timer = Timer<>();
  ble_timer.every(VOLTAGE_UPDATE_RATE * 1000, voltage_control_loop);

  for (;;)
  {
    ble_timer.tick();
  }
}

void setup_ble()
{
  log_message("Starting BLE");
  adcAttachPin(VOLTAGE_PIN);
  xTaskCreatePinnedToCore(
      setup_ble_main, // task function
      "ble_task",     // name of task
      10000,          // stack size of task
      NULL,           // parameter of the task
      1,              // priority of the task
      &ble_task,      // task handle to keep track of created task
      1);             // pin task to core
}

void send_message(std::string message)
{
  if (!deviceConnected)
  {
    return;
  }

  message_send_characteristic->setValue(message);
  message_send_characteristic->notify();
}
