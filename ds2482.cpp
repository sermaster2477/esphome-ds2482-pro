#include "ds2482.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace ds2482 {

static const char *TAG = "ds2482";

void DS2482Component::setup() {
  uint8_t cmd = 0xF0; // Device Reset
  if (this->write(&cmd, 1) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "DS2482 not found on I2C bus!");
    this->mark_failed();
    return;
  }
  delay(2);

  uint8_t test_ch[2] = {0xC3, 0xE1}; // Try to select Channel 1
  this->is_ds2482_800_ = (this->write(test_ch, 2) == i2c::ERROR_OK);
  
  this->configure_masters(true, false, false);
}

bool DS2482Component::configure_masters(bool active_pullup, bool overdrive, bool strong_pullup) {
  uint8_t val = 0x02; 
  if (active_pullup) val |= 0x01; 
  if (strong_pullup) val |= 0x04;
  if (overdrive)     val |= 0x08;

  uint8_t config_byte = (val | ((~val & 0x0F) << 4));
  uint8_t cmd[2] = {0xD2, config_byte};
  
  if (this->write(cmd, 2) != i2c::ERROR_OK) return false;

  uint8_t check_val;
  if (this->read(&check_val, 1) == i2c::ERROR_OK) {
    // Changed to VERBOSE to keep logs clean
    ESP_LOGV(TAG, "Config confirmed by chip: 0x%02X", check_val);
  }
  return true;
}

bool DS2482Component::select_channel(uint8_t channel) {
  static const uint8_t ch_codes[] = {0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87};
  if (channel > 7) return false;

  uint8_t cmd[2] = {0xC3, ch_codes[channel]};
  if (this->write(cmd, 2) != i2c::ERROR_OK) return false;

  uint8_t read_back;
  this->read(&read_back, 1);
  
  this->active_channel_ = channel;
  this->configure_masters(true, false, false); 
  return true;
}

bool DS2482Component::reset_1w(uint8_t channel) {
  if (!wait_busy()) return false;
  uint8_t cmd = 0xB4; 
  this->write(&cmd, 1);
  if (!wait_busy()) return false;
  return (read_status() & 0x02); // Presence Pulse Detect
}

bool DS2482Component::start_group_conversion(uint8_t channel) {
  uint32_t now = millis();
  if (now - this->last_conversion_start_[channel] < 2000 && this->last_conversion_start_[channel] != 0) {
    return true; 
  }

  if (!this->select_channel(channel)) return false;

  if (this->reset_1w(channel)) {
    this->write_byte_1w(channel, 0xCC); // SKIP ROM
    this->write_byte_1w(channel, 0x44); // CONVERT T
    this->last_conversion_start_[channel] = now;
    ESP_LOGD(TAG, "Group conversion started on channel %d", channel);
    return true;
  }
  return false;
}

void DS2482Component::write_byte_1w(uint8_t channel, uint8_t data) {
  if (!wait_busy()) return;
  uint8_t cmd[2] = {0xA5, data};
  this->write(cmd, 2);
}

uint8_t DS2482Component::read_byte_1w(uint8_t channel) {
  if (!wait_busy()) return 0;
  uint8_t cmd = 0x96;
  this->write(&cmd, 1);
  if (!wait_busy()) return 0;
  
  uint8_t srp[2] = {0xE1, 0xE1}; 
  this->write(srp, 2);
  uint8_t data;
  this->read(&data, 1);
  return data;
}

bool DS2482Component::search(uint8_t *newAddr) {
  uint8_t id_bit_number = 1, last_zero = 0, rom_byte_number = 0, rom_byte_mask = 1;
  bool search_result = false;

  if (!this->last_device_flag_) {
    if (!this->reset_1w(this->active_channel_)) {
      this->last_discrepancy_ = 0;
      return false;
    }
    this->write_byte_1w(this->active_channel_, 0xF0); // SEARCH ROM

    do {
      uint8_t search_direction = 0;
      if (id_bit_number < this->last_discrepancy_) {
        search_direction = ((this->last_rom_address_[rom_byte_number] & rom_byte_mask) > 0) ? 1 : 0;
      } else {
        search_direction = (id_bit_number == this->last_discrepancy_) ? 1 : 0;
      }

      if (!wait_busy()) return false;
      uint8_t triplet_cmd[2] = {0x78, (uint8_t)(search_direction ? 0x80 : 0x00)};
      this->write(triplet_cmd, 2);
      if (!wait_busy()) return false;

      uint8_t status = read_status();
      bool id_bit = (status & 0x20), comp_bit = (status & 0x40);
      uint8_t direction = (status & 0x80) ? 1 : 0;

      if (id_bit && comp_bit) break;
      if (!id_bit && !comp_bit && direction == 0) last_zero = id_bit_number;

      if (direction) this->last_rom_address_[rom_byte_number] |= rom_byte_mask;
      else this->last_rom_address_[rom_byte_number] &= ~rom_byte_mask;

      id_bit_number++;
      rom_byte_mask <<= 1;
      if (rom_byte_mask == 0) { rom_byte_number++; rom_byte_mask = 1; }
    } while (rom_byte_number < 8);

    if (!(id_bit_number < 65)) {
      this->last_discrepancy_ = last_zero;
      if (this->last_discrepancy_ == 0) this->last_device_flag_ = true;
      search_result = true;
    }
  }
  if (!search_result || !this->last_rom_address_[0]) {
    this->last_discrepancy_ = 0;
    this->last_device_flag_ = false;
    search_result = false;
  } else {
    for (int i = 0; i < 8; i++) newAddr[i] = this->last_rom_address_[i];
  }
  return search_result;
}

void DS2482Component::scan_and_log_devices() {
  ESP_LOGI(TAG, "Starting 1-Wire search...");
  for (uint8_t ch = 0; ch < (is_ds2482_800_ ? 8 : 1); ch++) {
    if (!select_channel(ch)) continue;
    ESP_LOGI(TAG, "Scanning Channel %d:", ch);
    uint8_t address[8];
    bool found = false;
    this->last_discrepancy_ = 0;
    this->last_device_flag_ = false;
    while (this->search(address)) {
      found = true;
      ESP_LOGI(TAG, "  - Found: 0x%02X%02X%02X%02X%02X%02X%02X%02X", 
        address[0], address[1], address[2], address[3], address[4], address[5], address[6], address[7]);
    }
    if (!found) ESP_LOGD(TAG, "  - No devices found");
  }
}

bool DS2482Component::wait_busy() {
  uint32_t start = millis();
  while (millis() - start < 20) {
    if (!(read_status() & 0x01)) return true;
    delay(1);
  }
  return false;
}

uint8_t DS2482Component::read_status() {
  uint8_t srp[2] = {0xE1, 0xF0}; 
  this->write(srp, 2);
  uint8_t status;
  this->read(&status, 1);
  return status;
}

uint8_t DS2482Component::crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t inbyte = data[i];
    for (uint8_t j = 0; j < 8; j++) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}

void DS2482Component::dump_config() {
  ESP_LOGCONFIG(TAG, "DS2482 Master Hub (%s)", is_ds2482_800_ ? "8-Channel" : "1-Channel");
}

void DS2482Component::recover_bus() {
    uint8_t cmd = 0xF0; 
    this->write(&cmd, 1);
    delay(20);
    this->configure_masters(true, false, false);
}

} 
}