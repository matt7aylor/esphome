#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace tes902 {

class TES902Component : public PollingComponent, public uart::UARTDevice {
  static const uint16_t MAX_READ_LENGTH = 10;

 public:
  TES902Component() = default;

  void dump_config() override;
  void loop() override;
  void update() override;

  float get_setup_priority() const override;

  void set_co2_sensor(sensor::Sensor *co2_sensor) { this->co2_sensor_ = co2_sensor; }

 protected:
  int readmsg_(int readch, char *buffer, int len);

  sensor::Sensor *co2_sensor_{nullptr};

  uint32_t last_transmission_{0};
};

}  // namespace tes902
}  // namespace esphome
