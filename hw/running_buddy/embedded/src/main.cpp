#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include <FastLED.h>
#include <sstream>
#include <arduino-timer.h>
#include <math.h>
#include <deque>
#include <numeric>
#include <iterator>
#include <limits>
#include <vector>

#include "carriers.h"

#define NUM_LEDS 16

#define BAUD_RATE 115200

#define ACCEL_QUEUE_SIZE 20
#define STEP_THRESHOLD 0.3
#define DELTA_STEP_MS 350

#define TURN_QUEUE_SIZE 20
#define LEFT_TURN_THRESHOLD -0.05
#define RIGHT_TURN_THRESHOLD 0.05
#define DELTA_TURN 100

#define LEFT_LED 3
#define RIGHT_LED 1

#define LED_PIN 21
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

CRGBArray<NUM_LEDS> leds;

uint8_t brightness = 128;

MKRIoTCarrier carrier;

std::ostringstream ss;

auto timer = timer_create_default();

void clear_ss()
{
  ss.str("");
  ss.clear();
}

void update_brightness()
{
  FastLED.setBrightness(brightness);
}

void setup_LEDs()
{
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  update_brightness();
}

void setup_carrier()
{
  delay(1500);
  CARRIER_CASE = false;
  if (!carrier.begin())
  {
    Serial.println("Carrier not connected, check connections");
    for (;;)
    {
      delay(1000);
    }
  }
  while (!carrier.IMUmodule.accelerationAvailable())
  {
    Serial.println("wait accel");
    delay(100);
  }
  Serial.println("done with setup");
  delay(1000);

  // show loading screen
  carrier.display.fillScreen(0x0000);
  carrier.display.setRotation(2); // rotate 180 degrees
  carrier.display.setTextWrap(true);
  carrier.display.drawBitmap(60, 30, loading_logo, 120, 121, 0xFFFF);
  carrier.display.setTextColor(0xFFFF);
  carrier.display.setTextSize(3);
  carrier.display.setCursor(35, 160);
  carrier.display.print("Loading...");
  delay(4000);
}

double mean(const std::deque<double> &vec)
{
  double avg = std::accumulate(vec.begin(), vec.end(), 0.0) / vec.size();
  return avg;
}

bool data_filter(std::deque<double> &hist_data, const size_t &queue_size, const int &delta,
                 const std::vector<float> &data, uint32_t &last_time, bool normalize = true,
                 const double &min_threshold = std::numeric_limits<double>::max(),
                 const double &max_threshold = std::numeric_limits<double>::min())
{
  double sum = std::accumulate(data.begin(), data.end(), 0.0, [normalize](double total, double curr)
                               { return total + (normalize ? pow(curr, 2) : curr); });
  double val = normalize ? sqrt(sum) : sum;
  hist_data.push_back(val);
  if (hist_data.size() <= queue_size)
  {
    return false;
  }
  hist_data.pop_front();
  uint32_t curr_time = millis();
  if (last_time + delta > curr_time)
  {
    return false;
  }
  double avg = abs(mean(hist_data));
  double normalized_val = val + (val < 0 ? avg : -avg);

  if (normalized_val > min_threshold || normalized_val < max_threshold)
  {
    return false;
  }
  last_time = curr_time;
  return true;
}

File file;
const String file_name = "data.csv";

void log_data_sd(String data)
{
  file = SD.open(file_name.c_str(), FILE_WRITE);

  if (file)
  {
    Serial.println(data);
    file.println(data);
    file.close();
  }
  else
  {
    Serial.println(data);
  }
}

void log_data(std::vector<float> &data)
{
  std::ostringstream oss;
  oss << "gyro,";
  std::copy(data.begin(), data.end(), std::ostream_iterator<int>(oss, ", "));
  log_data_sd(oss.str().c_str());
}

uint64_t steps = 0;
uint32_t last_step = millis();
std::deque<double> hist_accel;

bool handle_step(void *)
{
  std::vector<float> data(3, 0);
  carrier.IMUmodule.readAcceleration(data[0], data[1], data[2]);
  log_data(data);

  if (data_filter(hist_accel, ACCEL_QUEUE_SIZE, DELTA_STEP_MS,
                  data, last_step, true,
                  std::numeric_limits<double>::max(), STEP_THRESHOLD))
  {
    steps++;
    ss << "step," << steps;
    log_data_sd(ss.str().c_str());
    clear_ss();
  }

  return true;
}

const int text_size = 3;
const int text_size_x = text_size * 6;
const int text_size_y = text_size * 8;

void show_steps()
{
  const int start = 80;
  const int line = 160;
  const int message_space = 15;

  carrier.display.setCursor(start, line);
  carrier.display.fillRect(start, line - 5, message_space * text_size_x, text_size_y + 5, 0x0000);
  ss << steps << " steps";
  carrier.display.setCursor(start, line);
  carrier.display.print(ss.str().c_str());
  clear_ss();
}

void show_temperature()
{
  const int start = 80;
  const int line = 160;
  const int message_space = 15;

  carrier.display.fillRect(start, line - 5, message_space * text_size_x, text_size_y + 5, 0x0000);
  float temperature = carrier.Env.readTemperature();
  carrier.display.setCursor(start, line);
  carrier.display.print(temperature);
  carrier.display.print(" C");
}

enum class mode_type
{
  STEPS,
  TEMPERATURE
};

std::vector<mode_type> modes = {mode_type::STEPS, mode_type::TEMPERATURE};

size_t curr_mode_idx = modes.size() - 1;

bool handle_display(void *)
{
  mode_type curr_mode = modes[curr_mode_idx];
  switch (curr_mode)
  {
  case mode_type::STEPS:
    show_steps();
    break;
  case mode_type::TEMPERATURE:
    show_temperature();
    break;
  default:
    break;
  }
  return true;
}

CRGB off_color = CRGB::Black;

CRGB curr_color = off_color;

void handle_color(CRGB on_color, uint16_t pixel)
{
  const uint32_t led_color = curr_color == off_color ? carrier.leds.Color(255, 0, 0) : carrier.leds.Color(0, 0, 0);
  carrier.leds.setPixelColor(pixel, led_color);
  carrier.leds.show();
  curr_color = curr_color == off_color ? on_color : off_color;
  fill_solid(leds, NUM_LEDS, curr_color);
  FastLED.show();
  delay(50);
}

void setup_steps_display()
{
  carrier.display.fillScreen(0x0000);
  carrier.display.setCursor(54, 40);
  carrier.display.setTextSize(text_size);
  carrier.display.print("Steps");
  carrier.display.drawBitmap(70, 60, steps_logo, 100, 100, 0xF621);
}

void setup_temperature_display()
{
  carrier.display.fillScreen(0x0000);
  carrier.display.setCursor(54, 40);
  carrier.display.setTextSize(text_size);
  carrier.display.print("Temp");
  carrier.display.drawBitmap(70, 60, temperature_logo, 100, 100, 0xF621);
}

void toggle_display(bool right_direction = true)
{
  curr_mode_idx = (curr_mode_idx + (right_direction ? 1 : -1)) % modes.size();
  mode_type curr_mode = modes[curr_mode_idx];
  uint32_t led_color = 0;
  switch (curr_mode)
  {
  case mode_type::STEPS:
    led_color = carrier.leds.Color(0, 64, 0);
    setup_steps_display();
    break;
  case mode_type::TEMPERATURE:
    led_color = carrier.leds.Color(64, 0, 0);
    setup_temperature_display();
  default:
    break;
  }
  carrier.leds.setPixelColor(right_direction ? 1 : 3, led_color);
  carrier.leds.show();
  delay(50);
}

// TODO - use relay to activate buzzer
void setup_buzzer()
{
  carrier.Relay1.close();
}

void setup_sd()
{
  // SD.remove((char *)file_name.c_str());
  // file = SD.open(file_name.c_str(), FILE_WRITE);
  // file.close();
}

void setup()
{
  Serial.begin(BAUD_RATE);
  setup_carrier();
  setup_LEDs();
  setup_sd();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  setup_buzzer();
  delay(5000);
  toggle_display();
  timer.every(500, handle_display);
  timer.every(100, handle_step);
}

void loop()
{
  // update loop
  carrier.Buttons.update();
  timer.tick();

  if (carrier.Buttons.onTouchDown(TOUCH0))
  {
    // night
    log_data_sd("night_mode,");
    handle_color(CRGB::Aqua, 0);
  }
  if (carrier.Buttons.onTouchDown(TOUCH4))
  {
    // day
    log_data_sd("day_mode,");
    handle_color(CRGB::White, 4);
  }
  if (carrier.Buttons.onTouchDown(TOUCH1))
  {
    log_data_sd("toggle_display_left,");
    toggle_display(true);
  }
  if (carrier.Buttons.onTouchDown(TOUCH3))
  {
    log_data_sd("toggle_display_right,");
    toggle_display(false);
  }
}
