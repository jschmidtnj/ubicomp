#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include <sstream>

#include "pitches.h"
#include "carriers.h"

#define BAUD_RATE 115200

MKRIoTCarrier carrier;

void setup_carrier()
{
  delay(1500);
  CARRIER_CASE = false;
  if (!carrier.begin())
  {
    Serial.println("Carrier not connected, check connections");
    while (1)
      ;
  }
}

void setup()
{
  Serial.begin(BAUD_RATE);
  setup_carrier();
}

String state = "start";

int count = 3;
uint32_t colorRed = carrier.leds.Color(0, 200, 0);
uint32_t colorGreen = carrier.leds.Color(200, 0, 0);
uint32_t colorBlue = carrier.leds.Color(0, 0, 200);

// notes in the melody:
int melody[] = {
    NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
    4, 8, 8, 4, 4, 4, 4, 4};

// Declare pages
void sensorsPage();

// Declare their flags
bool touchCheckCompleted = false;
bool shakeCheckCompleted = false;
bool colorCheckCompleted = false;
bool gestureCheckCompleted = false;
bool relay1CheckCompleted = false;
bool relay2CheckCompleted = false;

void loop()
{
  delay(5000);
  sensorsPage();
  while (1)
    ;
}

std::ostringstream ss;

void clear_ss()
{
  ss.str("");
  ss.clear();
}

// Check the Env, IMU and light sensors
void sensorsPage()
{
  carrier.display.fillScreen(0x0000);

  // displaying pressure
  carrier.display.fillScreen(0x0000);
  carrier.display.setCursor(54, 40);
  carrier.display.setTextSize(3);
  carrier.display.print("Pressure");
  carrier.display.drawBitmap(70, 60, pressure_logo, 100, 100, 0xF621); // 0xF1C40F); //draw a pressure figure on the MKR IoT carrier's display
  float press = carrier.Pressure.readPressure();                       // storing pressure value in a variable
  carrier.display.setCursor(40, 160);
  carrier.display.println(press); // display the pressure on the screen
  carrier.display.setCursor(160, 160);
  carrier.display.print("KPa");
  delay(2500);

  // reading acceleration values to detect shaking
  float x, y, z, idleX;
  while (!carrier.IMUmodule.accelerationAvailable())
    ;
  if (carrier.IMUmodule.accelerationAvailable())
  {
    carrier.IMUmodule.readAcceleration(x, y, z);
  }

  idleX = x; // stroing the intial acceleration on the x axis

  // displaying the shaking page on the display
  carrier.display.fillScreen(0x0000);
  carrier.display.setTextSize(3);
  carrier.display.setCursor(75, 40);
  carrier.display.println("Shake");
  carrier.display.setCursor(30, 70);
  carrier.display.println("your device");
  carrier.display.drawBitmap(28, 90, shake_logo, 196, 100, 0x9C2D); // 0x9E846D); // displaying a shake figure
  carrier.leds.fill(carrier.leds.Color(0, 0, 90), 0, 5);
  carrier.leds.show();

  delay(2500);

  // Wait for the shake event
  while (!shakeCheckCompleted)
  {

    if (carrier.IMUmodule.accelerationAvailable())
    {
      carrier.IMUmodule.readAcceleration(x, y, z);
      Serial.println(x);
    }
    if (x > idleX + 2) // if the acceleration is more than the initial value, a movement is detected
    {
      shakeCheckCompleted = true;
    }
  }

  for (;;)
  {
    Serial.println("loop");
    float a_x, a_y, a_z, g_x, g_y, g_z;
    while (!carrier.IMUmodule.accelerationAvailable())
    {
      Serial.println("wait accel");
      delay(100);
    }
    if (carrier.IMUmodule.accelerationAvailable())
    {
      carrier.IMUmodule.readAcceleration(a_x, a_y, a_z);
    }
    while (!carrier.IMUmodule.gyroscopeAvailable())
    {
      Serial.println("wait gyro");
      delay(100);
    }
    if (carrier.IMUmodule.gyroscopeAvailable())
    {
      carrier.IMUmodule.readGyroscope(g_x, g_y, g_z);
    }
    carrier.display.fillScreen(0x0000);
    carrier.display.setCursor(25, 60);
    carrier.display.setTextSize(1);
    ss << "accel: " << a_x << ", " << a_y << ", " << a_z;
    carrier.display.print(ss.str().c_str());
    Serial.println(ss.str().c_str());
    clear_ss();
    carrier.display.setCursor(60, 180);
    ss << "gyro: " << g_x << ", " << g_y << ", " << g_z;
    carrier.display.print(ss.str().c_str());
    delay(1000);
  }
}
