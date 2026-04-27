# MAX17048 + SHT3x Combined — Production Sketch

Combined battery monitor (MAX17048) and temperature/humidity sensor (SHT3x) on shared **I2C bus (Pin 12/13)** for **AmebaPRO2-mini (AMB82-mini, RTL8735B)**.

## Hardware

| Device     | I2C Address | Pin 12 | Pin 13 | Function |
|------------|-------------|--------|--------|----------|
| MAX17048   | 0x36        | SDA    | SCL    | Battery voltage & SOC |
| SHT3x      | 0x44        | SDA    | SCL    | Temperature & humidity |

> ⚠️ Do **NOT** use Pin 9/10 for I2C (conflict with DMIC / ADS1256).

## Features

- Single `Wire.begin()` — no bus re-initialization between devices
- SHT3x initialized first, then MAX17048 (both orders work, but this is stable)
- CRC8 validation on every SHT3x measurement
- 10-second reading interval
- English-only serial output

## Serial Output

```
========================================
  MAX17048 + SHT3x Combined
  SDA=Pin12  SCL=Pin13
========================================

SHT3x init... OK
MAX17048 init... OK

--- Starting readings (10s interval) ---

Temp: 23.45C  |  Hum: 56.78%RH  |  Batt: 4.112V  |  SOC: 87.5%
```

## Build

- Arduino IDE 2.x
- Board: **AmebaPro2 AMB82-mini**
- SDK: 4.0.9-build20250805 or newer
- Compiler warnings: **None**

## Related Repos

- [MAX17048 standalone](https://github.com/SmartShelfAI/max17048-amebapro2)
- [SHT3x standalone](https://github.com/SmartShelfAI/sht3x-amebapro2)
- [Debug workspace](https://github.com/SmartShelfAI/max17048-sht3x-debug)

## License

MIT
