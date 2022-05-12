#include "common.h"

void log_message(const std::string &message)
{
  Serial.println(message.c_str());
}
