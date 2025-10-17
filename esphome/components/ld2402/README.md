# HLK-LD2402 mmWave Sensor Component

This component supports the HLK-LD2402 24GHz mmWave presence detection sensor.

## Features

- Presence detection with configurable sensitivity
- Distance measurement (0-10m range)
- 16 detection gates with individual move and micro-motion thresholds
- Automatic threshold calibration
- Configurable timeout for presence detection
- Engineering mode for detailed energy data
- Firmware version and serial number reporting
- Non-volatile parameter storage

## Configuration

```yaml
uart:
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 115200

ld2402:
  id: my_ld2402

binary_sensor:
  - platform: ld2402
    has_target:
      name: "Presence"

sensor:
  - platform: ld2402
    distance:
      name: "Distance"

text_sensor:
  - platform: ld2402
    fw_version:
      name: "Firmware Version"
    serial_number:
      name: "Serial Number"

button:
  - platform: ld2402
    apply_config:
      name: "Apply Config"
    factory_reset:
      name: "Factory Reset"
    restart_module:
      name: "Restart Module"
    revert_config:
      name: "Revert Config"

number:
  - platform: ld2402
    presence_timeout:
      name: "Timeout"
    max_distance:
      name: "Max Distance"
    gate_0:
      move_threshold:
        name: "Gate 0 Move Threshold"
      micro_threshold:
        name: "Gate 0 Micro Threshold"
    # Configure additional gates as needed (gate_1 through gate_15)

select:
  - platform: ld2402
    operating_mode:
      name: "Operating Mode"
```

## Protocol Details

The component implements the HLK-LD2402 serial protocol with:

- Little-endian data format
- Frame format: Header (FD FC FB FA) + Length + Command + Data + Footer (04 03 02 01)
- Default baud rate: 115200, 1 stop bit, no parity
- Configuration mode for parameter modification
- Automatic parameter persistence

## Parameters

### Max Distance (Parameter ID: 0x0001)
Range: 7-100 (represents 0.7m to 10m with 10x scaling)

### Timeout (Parameter ID: 0x0004)
Range: 0-65535 seconds

### Move Thresholds (Parameter IDs: 0x0010-0x001F)
Individual thresholds for 16 gates (0-15)
Range: 0-65535 (squared modulus values)

### Micro-motion Thresholds (Parameter IDs: 0x0030-0x003F)
Individual micro-motion thresholds for 16 gates (0-15)
Range: 0-65535 (squared modulus values)

## Commands Supported

- Read firmware version (0x0000)
- Read serial number (0x0011, 0x0016)
- Enable/disable configuration mode (0x00FF, 0x00FE)
- Read/write parameters (0x0008, 0x0007)
- Set data output mode (0x0012)
- Auto threshold generation (0x0009)
- Query threshold progress (0x000A)
- Save parameters (0x00FD)
- Power-on auto gain (0x00EE)

## Operating Modes

- **Normal Mode**: Standard presence detection output
- **Engineering Mode**: Detailed energy data for all gates

## Notes

- Parameters must be saved after modification using the "Apply Config" button
- Factory reset restores default thresholds and settings
- Automatic threshold calibration available with configurable sensitivity factors
- Serial number reading requires firmware v3.3.5 or later for hex format
