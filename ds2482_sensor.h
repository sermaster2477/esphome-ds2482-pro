#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "ds2482.h"

namespace esphome {
namespace ds2482 {

class DS2482Sensor : public sensor::Sensor, public PollingComponent {
 public:
  // Эти методы ДОЛЖНЫ быть в секции public:
  void set_parent(DS2482Component *parent) { parent_ = parent; }
  void set_address(uint64_t address) { address_ = address; }
  void set_channel(uint8_t channel) { channel_ = channel; }
  void set_overdrive(bool overdrive) { overdrive_ = overdrive; } // <--- Перенесли сюда

  void update() override;
  void dump_config() override;

 protected:
  DS2482Component *parent_;
  uint64_t address_;
  uint8_t channel_;
  bool overdrive_{false}; 
};

}  // namespace ds2482
}  // namespace esphome