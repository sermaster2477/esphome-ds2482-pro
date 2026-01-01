#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace ds2482 {

class DS2482Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  bool reset_1w(uint8_t channel);
  void write_byte_1w(uint8_t channel, uint8_t data);
  uint8_t read_byte_1w(uint8_t channel);
  bool select_channel(uint8_t channel);
  bool configure_masters(bool active_pullup, bool overdrive, bool strong_pullup);
  
  bool start_group_conversion(uint8_t channel);
  void scan_and_log_devices();
  bool search(uint8_t *newAddr);
  
  void recover_bus();
  uint8_t crc8(const uint8_t *data, uint8_t len);

 protected:
  uint8_t read_status();
  bool wait_busy();

  bool is_ds2482_800_{false};
  uint8_t active_channel_{0};
  uint32_t last_conversion_start_[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  // Search state
  uint8_t last_rom_address_[8];
  uint8_t last_discrepancy_{0};
  bool last_device_flag_{false};
};

}  // namespace ds2482
}  // namespace esphome