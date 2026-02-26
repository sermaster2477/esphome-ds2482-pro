#include "ds2482_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ds2482 {

static const char *TAG = "ds2482.sensor";

void DS2482Sensor::update() {
  bool conversion_started = this->parent_->start_group_conversion(this->channel_);

  if (!conversion_started) {
    this->failed_consecutive_read_++;
    this->status_set_warning();
    
    // Добавлен вывод канала (CH) и адреса
    ESP_LOGE(TAG, "[CH:%d] 0x%016llX: ОШИБКА ШИНЫ (%d/5)", 
             this->channel_, this->address_, this->failed_consecutive_read_);
    
    if (this->failed_consecutive_read_ >= 5 && !std::isnan(this->state)) {
        ESP_LOGE(TAG, "[CH:%d] 0x%016llX: ПОТЕРЯН", this->channel_, this->address_);
        this->publish_state(NAN);
    }
    return;
  }

  this->set_timeout("read_temp", 800, [this]() {
    if (!this->parent_->select_channel(this->channel_)) return;

    bool success = false;

    if (this->parent_->reset_1w(this->channel_)) {
      this->parent_->write_byte_1w(this->channel_, 0x55); 
      for (int i = 0; i < 8; i++) {
          uint8_t byte = (uint8_t)(this->address_ >> (56 - i * 8));
          this->parent_->write_byte_1w(this->channel_, byte);
      }
      this->parent_->write_byte_1w(this->channel_, 0xBE);

      uint8_t data[9];
      bool all_zeros = true;
      for (int i = 0; i < 9; i++) {
        data[i] = this->parent_->read_byte_1w(this->channel_);
        if (data[i] != 0x00) all_zeros = false;
      }

      if ((this->parent_->crc8(data, 8) == data[8]) && !all_zeros) {
        float result = (int16_t)((data[1] << 8) | data[0]) / 16.0f;
        
        // Фильтр 85 градусов
        if (std::abs(result - 85.0f) < 0.01f && this->failed_consecutive_read_ == 0) {
            return; 
        }

        this->publish_state(result);
        this->status_clear_warning();
        this->failed_consecutive_read_ = 0; 
        success = true;
      }
    }

    if (!success) {
      this->failed_consecutive_read_++;
      this->status_set_warning();
      
      // Лог с указанием канала
      ESP_LOGE(TAG, "[CH:%d] 0x%016llX: ОШИБКА ДАННЫХ (%d/5)", 
               this->channel_, this->address_, this->failed_consecutive_read_);

      if (this->failed_consecutive_read_ >= 5) {
        if (!std::isnan(this->state)) {
          ESP_LOGE(TAG, "[CH:%d] 0x%016llX: ПОТЕРЯН", this->channel_, this->address_);
          this->publish_state(NAN);
        }
        // Чтобы счетчик не рос до миллионов, зафиксируем его на 100
        if (this->failed_consecutive_read_ > 100) this->failed_consecutive_read_ = 6;
      }
    }
  });
}
// dump_config оставляем без изменений