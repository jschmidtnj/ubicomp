#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>
#include <sstream>
#include <arduino-timer.h>
#include <math.h>
#include <deque>
#include <numeric>
#include <limits>
#include <vector>

#include "carriers.h"

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

MKRIoTCarrier carrier;

std::ostringstream ss;

auto timer = timer_create_default();

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
  carrier.display.setRotation(0);
  carrier.display.setTextWrap(true);
  carrier.display.drawBitmap(60, 30, hardware_logo, 120, 121, 0xFFFF);
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

uint64_t steps = 0;
uint32_t last_step = millis();
std::deque<double> hist_accel;

bool check_step(void *)
{
  std::vector<float> data(3, 0);
  carrier.IMUmodule.readAcceleration(data[0], data[1], data[2]);

  if (data_filter(hist_accel, ACCEL_QUEUE_SIZE, DELTA_STEP_MS,
                  data, last_step, true,
                  std::numeric_limits<double>::max(), STEP_THRESHOLD))
  {
    steps++;
  }

  return true;
}

bool left_turn = false;
uint32_t last_left_turn = millis();
std::deque<double> hist_left_turn;

bool right_turn = false;
uint32_t last_right_turn = millis();
std::deque<double> hist_right_turn;

bool check_turn(void *)
{
  std::vector<float> data(3, 0);
  carrier.IMUmodule.readAcceleration(data[0], data[1], data[2]);

  std::vector<float> left_turn_data{data[2]};
  if (data_filter(hist_left_turn, TURN_QUEUE_SIZE, DELTA_TURN,
                  left_turn_data, last_left_turn, false, LEFT_TURN_THRESHOLD))
  {
    if (!left_turn)
    {
      left_turn = true;
      timer.at(millis() + DELTA_TURN, [](void *) -> bool
               {
      left_turn = false;
      return false; });
    }
  }

  std::vector<float> right_turn_data{data[2]};
  if (data_filter(hist_right_turn, TURN_QUEUE_SIZE, DELTA_TURN,
                  right_turn_data, last_right_turn, false,
                  std::numeric_limits<double>::max(), RIGHT_TURN_THRESHOLD))
  {
    if (!right_turn)
    {
      right_turn = true;
      timer.at(millis() + DELTA_TURN, [](void *) -> bool
               {
      right_turn = false;
      return false; });
    }
  }

  return true;
}

bool show_turn(void *)
{
  // red and green are flipped for some reason
  const uint32_t left_color = carrier.leds.Color(255, 0, 0);
  const uint32_t right_color = carrier.leds.Color(0, 255, 0);
  const uint32_t off_color = carrier.leds.Color(0, 0, 0);

  carrier.leds.setPixelColor(LEFT_LED, left_turn ? left_color : off_color);
  carrier.leds.setPixelColor(RIGHT_LED, right_turn ? right_color : off_color);
  carrier.leds.show();

  return true;
}

const int steps_line = 160;

const int text_size = 3;
const int text_size_x = text_size * 6;
const int text_size_y = text_size * 8;

bool show_steps(void *)
{
  const int start_steps = 80;
  const int message_space = 15;

  carrier.display.setCursor(start_steps, steps_line);
  carrier.display.fillRect(start_steps, steps_line - 5, message_space * text_size_x, text_size_y + 5, 0x0000);
  ss << steps << " steps";
  carrier.display.setCursor(start_steps, steps_line);
  carrier.display.print(ss.str().c_str());
  clear_ss();
  return true;
}

void setup_steps_display()
{
  carrier.display.fillScreen(0x0000);
  carrier.display.setCursor(54, 40);
  carrier.display.setTextSize(text_size);
  carrier.display.print("Steps");
  carrier.display.drawBitmap(70, 60, steps_logo, 100, 100, 0xF621);
}

void setup()
{
  Serial.begin(BAUD_RATE);
  setup_carrier();
  delay(5000);
  setup_steps_display();
  timer.every(500, show_steps);
  timer.every(50, check_step);

  timer.every(500, show_turn);
  timer.every(DELTA_TURN / 2, check_turn);
}

void loop()
{
  timer.tick();
}
