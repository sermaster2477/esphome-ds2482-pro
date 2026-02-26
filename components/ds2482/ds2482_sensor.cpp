#include "ds2482_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ds2482 {

static const char *TAG = "ds2482.sensor";

void DS2482Sensor::update() {
  // Запускаем конвертацию на канале
  if (!this->parent_->start_group_conversion(this->channel_)) {
    this->status_set_warning();
    return;
  }

  // Ждем завершения преобразования (800мс достаточно для 12 бит)
  this->set_timeout("read_temp", 800, [this]() {
    if (!this->parent_->select_channel(this->channel_)) return;

    // 1. Пробуем сбросить шину (Presence Pulse)
    if (this->parent_->reset_1w(this->channel_)) {
      
      // MATCH ROM
      this->parent_->write_byte_1w(this->channel_, 0x55); 
      
      // Передача адреса (твой порядок байт - MSB first для YAML-совместимости)
      for (int i = 0; i < 8; i++) {
          uint8_t byte = (uint8_t)(this->address_ >> (56 - i * 8));
          this->parent_->write_byte_1w(this->channel_, byte);
      }

      // READ SCRATCHPAD
      this->parent_->write_byte_1w(this->channel_, 0xBE);

      uint8_t data[9];
      bool all_zeros = true;
      for (int i = 0; i < 9; i++) {
        data[i] = this->parent_->read_byte_1w(this->channel_);
        if (data[i] != 0x00) all_zeros = false;
      }

      // --- ПРОВЕРКА ДАННЫХ ---
      bool crc_valid = (this->parent_->crc8(data, 8) == data[8]);
      
      if (crc_valid && !all_zeros) {
        // ДАННЫЕ ВЕРНЫ
        int16_t raw_temp = (int16_t)((data[1] << 8) | data[0]);
        float result = raw_temp / 16.0f;

        // Фильтр на значение 85.0 (только при первом включении)
        static bool first_read_done = false;
        if (!first_read_done && std::abs(result - 85.0f) < 0.01f) {
            ESP_LOGD(TAG, "Skipping initial 85.0°C for 0x%llX", this->address_);
            first_read_done = true;
            return; 
        }
        first_read_done = true;

        // Публикуем результат и сбрасываем все ошибки
        this->publish_state(result);
        this->status_clear_warning();
        this->failed_consecutive_read_ = 0; // Сброс счетчика ошибок

      } else {
        // ОШИБКА CRC ИЛИ ПУСТЫЕ ДАННЫЕ
        this->failed_consecutive_read_++;
        ESP_LOGW(TAG, "Bad data from 0x%llX (Try %d/5). CRC: %s", 
                 this->address_, this->failed_consecutive_read_, crc_valid ? "OK" : "FAIL");
        this->status_set_warning();
      }
    } else {
      // ДАТЧИК НЕ ОТВЕТИЛ НА RESET
      this->failed_consecutive_read_++;
      ESP_LOGW(TAG, "No presence pulse for 0x%llX (Try %d/5)", 
               this->address_, this->failed_consecutive_read_);
      this->status_set_warning();
    }

    // --- ЛОГИКА NAN (ОТВАЛ ДАТЧИКА) ---
    // Если датчик не отвечает 5 раз подряд (50 секунд при интервале 10с)
    if (this->failed_consecutive_read_ >= 5) {
      if (!std::isnan(this->state)) {
        ESP_LOGE(TAG, "Sensor 0x%llX is lost after 5 attempts!", this->address_);
        this->publish_state(NAN);
      }
    }
  });
}

void DS2482Sensor::dump_config() {
  LOG_SENSOR("", "DS2482 Sensor", this);
  ESP_LOGCONFIG(TAG, "  Channel: %d", this->channel_);
  ESP_LOGCONFIG(TAG, "  Address: 0x%llX", this->address_);
}

}  // namespace ds2482
}  // namespace esphome

