# ESPHome DS2482 Pro

This is an optimized ESPHome external component for **DS2482-100** and **DS2482-800** I2C-to-1-Wire bridge masters.

## 🚀 Key Features

* **Hardware Acceleration:** ROM Search and Triplet commands offloaded to the DS2482 chip.
* **Parallel Conversion:** All sensors on a channel start conversion simultaneously (saves time).
* **Data Integrity:**
  * Hardware CRC8 validation.
  * Sanity range check (-55°C to +127°C).
  * Power-on 85°C filter.
* **Smart Reliability:** 5-retry logic before marking sensor as `NAN`.

## 🛠 Installation

Add this to your YAML:

```yaml
external_components:
  - source:
      type: git
      url: [https://github.com/sermaster2477/esphome-ds2482-pro](https://github.com/sermaster2477/esphome-ds2482-pro)
    components: [ ds2482 ]

sensor:
  - platform: ds2482
    ds2482_id: ds_hub          # Leave as is (links to the ds2482 hub id)
    name: "Living Room Temp"   # [CHANGE] Any name for your sensor
    
    # [CHANGE] Your sensor's 64-bit address (found via Scan Button)
    address: 0x2861640B4E27C94C 
    
    # [CHANGE] Channel number (0-7 for DS2482-800, always 0 for DS2482-100)
    channel: 1                 
    
    # [OPTIONAL] How often to poll the sensor
    update_interval: 60s
```

## 🔍 Diagnostics
Use this button to find sensor addresses in logs:


```yaml
button:
  - platform: template
    name: "Scan 1-Wire Bus"
    on_press:
      lambda: |-
        id(ds_hub).scan_and_log_devices();
```