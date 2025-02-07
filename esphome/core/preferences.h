#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include "esphome/core/defines.h"

namespace esphome {

class ESPPreferenceObject {
 public:
  ESPPreferenceObject() = default;
  ESPPreferenceObject(size_t offset, size_t length, uint32_t type);

  template<typename T> bool save(T *src);

  template<typename T> bool load(T *dest);

  bool is_initialized() const;

 protected:
  friend class ESPPreferences;

  bool save_();
  bool load_();
  bool save_internal_();
  bool load_internal_();

  uint32_t calculate_crc_() const;

  size_t offset_ = 0;
  size_t length_words_ = 0;
  uint32_t type_ = 0;
  std::vector<uint32_t> data_;
#ifdef ARDUINO_ARCH_ESP8266
  bool in_flash_ = false;
#endif
};

#ifdef ARDUINO_ARCH_ESP8266
#ifdef USE_ESP8266_PREFERENCES_FLASH
static const bool DEFAULT_IN_FLASH = true;
#else
static const bool DEFAULT_IN_FLASH = false;
#endif
#endif

#ifdef ARDUINO_ARCH_ESP32
static const bool DEFAULT_IN_FLASH = true;
#endif

class ESPPreferences {
 public:
  ESPPreferences();
  void begin();
  ESPPreferenceObject make_preference(size_t length, uint32_t type, bool in_flash = DEFAULT_IN_FLASH);
  template<typename T> ESPPreferenceObject make_preference(uint32_t type, bool in_flash = DEFAULT_IN_FLASH);

#ifdef ARDUINO_ARCH_ESP8266
  /** On the ESP8266, we can't override the first 128 bytes during OTA uploads
   * as the eboot parameters are stored there. Writing there during an OTA upload
   * would invalidate applying the new firmware. During normal operation, we use
   * this part of the RTC user memory, but stop writing to it during OTA uploads.
   *
   * @param prevent Whether to prevent writing to the first 32 words of RTC user memory.
   */
  void prevent_write(bool prevent);
  bool is_prevent_write();
#endif

 protected:
  friend ESPPreferenceObject;

  uint32_t current_offset_;
#ifdef ARDUINO_ARCH_ESP32
  uint32_t nvs_handle_;
#endif
#ifdef ARDUINO_ARCH_ESP8266
  void save_esp8266_flash_();
  bool prevent_write_{false};
  uint32_t *flash_storage_;
  uint32_t current_flash_offset_;
#endif
};

extern ESPPreferences global_preferences;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

template<typename T> ESPPreferenceObject ESPPreferences::make_preference(uint32_t type, bool in_flash) {
  return this->make_preference((sizeof(T) + 3) / 4, type, in_flash);
}

template<typename T> bool ESPPreferenceObject::save(T *src) {
  if (!this->is_initialized())
    return false;
  // ensure all bytes are 0 (in case sizeof(T) is not multiple of 4)
  std::fill_n(data_.begin(), length_words_, 0);
  memcpy(data_.data(), src, sizeof(T));
  return this->save_();
}

template<typename T> bool ESPPreferenceObject::load(T *dest) {
  std::fill_n(data_.begin(), length_words_, 0);
  if (!this->load_())
    return false;

  memcpy(dest, data_.data(), sizeof(T));
  return true;
}

}  // namespace esphome
