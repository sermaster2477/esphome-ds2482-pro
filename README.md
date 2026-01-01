# Enhanced DS2482 Component for ESPHome

This is an optimized ESPHome external component for the **DS2482-100** and **DS2482-800** I2C-to-1-Wire bridge masters. 

Most existing drivers use software-based bit-banging for 1-Wire operations, which is slow and CPU-intensive. This implementation leverages the **hardware capabilities** of the DS2482 chip for maximum reliability and speed.

## Key Features

* **Hardware-Accelerated ROM Search:** Uses the DS2482 `1-Wire Triplet` (0x78) command. This offloads the 1-Wire bit-processing to the chip's internal logic, making device discovery robust even on busy buses.
* **Parallel Temperature Conversion (Skip ROM):** Implements a global "Convert T" command (0x44) for all sensors on a channel simultaneously. This reduces the total polling time significantly (e.g., 10 sensors are read in ~800ms total, instead of 7.5 seconds).
* **Forced Active Pullup (APU):** Automatically enables the DS2482 internal active pullup. This is critical for long 1-Wire cables (up to 100m+) where standard passive resistors fail to maintain signal integrity.
* **Multi-Channel Support:** Fully compatible with the 8-channel DS2482-800, allowing hundreds of sensors on a single I2C bus.
* **Clean Debug Logging:** Advanced technical logs (like configuration confirmations) are moved to the `VERBOSE` level to keep your main log clean.

## Installation

Add this to your ESPHome configuration:

```yaml
external_components:
  - source:
      type: git
      url: [https://github.com/YOUR_USERNAME/esphome-ds2482-custom](https://github.com/YOUR_USERNAME/esphome-ds2482-custom)
    components: [ ds2482 ]

# Example configuration
ds2482:
  id: ds_hub
  address: 0x18

sensor:
  - platform: ds2482
    name: "Living Room Temp"
    address: 0x2861640B4E27C94C
    channel: 1
    update_interval: 60s
