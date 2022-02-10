#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include <sstream>

#define BAUD_RATE 115200

MKRIoTCarrier carrier;

std::ostringstream ss;

void clear_ss()
{
  ss.str("");
  ss.clear();
}

void setup_carrier()
{
  delay(1500);
  CARRIER_CASE = false;
  if (!carrier.begin())
  {
    Serial.println("Carrier not connected, check connections");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("done with setup");
  delay(5000);
}

void setup()
{
  Serial.begin(BAUD_RATE);
  setup_carrier();
}

void loop()
{
  Serial.println("loop");
  float a_x, a_y, a_z, g_x, g_y, g_z;
  while (!carrier.IMUmodule.accelerationAvailable()) {
    Serial.println("wait accel");
    delay(100);
  }
  if (carrier.IMUmodule.accelerationAvailable())
  {
    carrier.IMUmodule.readAcceleration(a_x, a_y, a_z);
  }
  while (!carrier.IMUmodule.gyroscopeAvailable()) {
    Serial.println("wait gyro");
    delay(100);
  }
  if (carrier.IMUmodule.gyroscopeAvailable())
  {
    carrier.IMUmodule.readGyroscope(g_x, g_y, g_z);
  }
  carrier.display.fillScreen(0x0000);
  carrier.display.setCursor(25, 60);
  carrier.display.setTextSize(3);
  ss << "accel: " << a_x << ", " << a_y << ", " << a_z;
  carrier.display.print(ss.str().c_str());
  Serial.println(ss.str().c_str());
  clear_ss();
  carrier.display.setCursor(60, 180);
  ss << "gyro: " << g_x << ", " << g_y << ", " << g_z;
  carrier.display.print(ss.str().c_str());
  delay(1000);
}
