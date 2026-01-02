# Enhanced DS2482 Component for ESPHome

This is an optimized ESPHome external component for the **DS2482-100** and **DS2482-800** I2C-to-1-Wire bridge masters. 

Most existing drivers use software-based bit-banging for 1-Wire operations, which is slow and CPU-intensive. This implementation leverages the **hardware capabilities** of the DS2482 chip for maximum reliability and speed.

Tested with 10+ DS18B20 sensors at 1s update interval. Supports hot-swapping and handles sensor disconnections correctly by publishing NAN.

## Key Features

* **Hardware-Accelerated ROM Search:** Uses the DS2482 `1-Wire Triplet` (0x78) command. This offloads the 1-Wire bit-processing to the chip's internal logic, making device discovery robust even on busy buses.
* **Parallel Temperature Conversion (Skip ROM):** Implements a global "Convert T" command (0x44) for all sensors on a channel simultaneously. This reduces the total polling time significantly (e.g., 10 sensors are read in ~800ms total, instead of 7.5 seconds).
* **Forced Active Pullup (APU):** Automatically enables the DS2482 internal active pullup. This is critical for long 1-Wire cables (up to 100m+) where standard passive resistors fail to maintain signal integrity.
* **Multi-Channel Support:** Fully compatible with the 8-channel DS2482-800, allowing hundreds of sensors on a single I2C bus.
* **Clean Debug Logging:** Advanced technical logs (like configuration confirmations) are moved to the `VERBOSE` level to keep your main log clean.

## Installation

Add this to your ESPHome configuration:

```yaml

# bus description i2c for connection ds2482
i2c:
  - id: bus_a
    sda: GPIO1         #  your connection pin number
    scl: GPIO2         #  your connection pin number
    frequency: 400kHz  #  IFor more stable operation, you can reduce the speed to 100 kHz
    scan: true         #  display in the logs which addresses ds2482 are found on the bus


external_components:
  - source:
      type: git
      url: https://github.com/sermaster2477/esphome-ds2482-pro
    components: [ ds2482 ]

# Example configuration
ds2482:
  id: ds_hub
  address: 0x18    # address if all address lines ds2482-800 are shorted to ground

sensor:
  - platform: ds2482
    ds2482_id: ds_hub
    name: "Living Room Temp"
    address: 0x2861640B4E27C94C  # your thermometer's address
    channel: 1                   # channel number ds2482-800 from 0 to 7
    update_interval: 60s

#  Hardware Search Button
#  You can add a button to trigger a deep scan of all channels to find your sensor addresses:
#  You can copy them from the log to add them to the YAML code.

button:
  - platform: template
    name: "Scan 1-Wire Bus"  #  scans all channels and displays the addresses of found thermometers in the log on a channel-by-channel basis
    on_press:
      lambda: |-
        id(ds_hub).scan_and_log_devices();
