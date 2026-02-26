#include "ds2482_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ds2482 {

static const char *TAG = "ds2482.sensor";

void DS2482Sensor::update() {
  // 1. Запуск конвертации на канале
  if (!this->parent_->start_group_conversion(this->channel_)) {
    this->status_set_warning();
    return;
  }

  // 2. Ждем 800мс и читаем результат
  this->set_timeout("read_temp", 800, [this]() {
    if (!this->parent_->select_channel(this->channel_)) return;

    bool success = false; // Флаг успешного чтения в текущем цикле

    if (this->parent_->reset_1w(this->channel_)) {
      this->parent_->write_byte_1w(this->channel_, 0x55); // MATCH ROM
      
      // Передача адреса (MSB first для совместимости с твоим YAML)
      for (int i = 0; i < 8; i++) {
          uint8_t byte = (uint8_t)(this->address_ >> (56 - i * 8));
          this->parent_->write_byte_1w(this->channel_, byte);
      }

      this->parent_->write_byte_1w(this->channel_, 0xBE); // READ SCRATCHPAD

      uint8_t data[9];
      bool all_zeros = true;
      for (int i = 0; i < 9; i++) {
        data[i] = this->parent_->read_byte_1w(this->channel_);
        if (data[i] != 0x00) all_zeros = false;
      }

      // ПРОВЕРКА CRC И ДАННЫХ
      if ((this->parent_->crc8(data, 8) == data[8]) && !all_zeros) {
        int16_t raw_temp = (int16_t)((data[1] << 8) | data[0]);
        float result = raw_temp / 16.0f;

        // Фильтр на 85 градусов при старте
        static bool initial_ignore = false;
        if (!initial_ignore && std::abs(result - 85.0f) < 0.01f) {
            ESP_LOGD(TAG, "'%s' - Пропуск стартовых 85°C", this->get_name().c_str());
            initial_ignore = true;
            return;
        }
        initial_ignore = true;

        // ЕСЛИ ВСЁ ОК: Публикуем и сбрасываем счетчик
        this->publish_state(result);
        this->status_clear_warning();
        this->failed_consecutive_read_ = 0; // ОБНУЛЯЕМ СЧЕТЧИК
        success = true; 
        ESP_LOGV(TAG, "'%s' - Успешное чтение: %.2f", this->get_name().c_str(), result);
      } else {
        ESP_LOGW(TAG, "'%s' - Ошибка CRC или пустые данные", this->get_name().c_str());
      }
    } else {
      ESP_LOGW(TAG, "'%s' - Датчик не ответил (No presence pulse)", this->get_name().c_str());
    }

    // ЛОГИКА СЧЕТЧИКА ПРИ ОШИБКЕ
    if (!success) {
      this->failed_consecutive_read_++; // Увеличиваем переменную из .h файла
      this->status_set_warning();
      
      ESP_LOGW(TAG, "'%s' - Ошибок подряд: %d из 5", this->get_name().c_str(), this->failed_consecutive_read_);

      if (this->failed_consecutive_read_ >= 5) {
        if (!std::isnan(this->state)) {
          ESP_LOGE(TAG, "'%s' - Датчик ПОТЕРЯН после 5 попыток! Шлем NAN.", this->get_name().c_str());
          this->publish_state(NAN);
        }
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

