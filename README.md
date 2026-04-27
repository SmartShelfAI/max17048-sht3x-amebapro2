# MAX17048 + SHT3x Combined — Production Sketch

Combined battery monitor (MAX17048) and temperature/humidity sensor (SHT3x) on shared **I2C bus (Pin 12/13)** for **AmebaPRO2-mini (AMB82-mini, RTL8735B)**.

> ✅ **Verified working** — see [test log](#verified-output) below.

---

## Hardware Wiring

| Ameba Pin | Signal | Device | Note |
|-----------|--------|--------|------|
| **12** (PE4) | SDA | MAX17048 + SHT3x | 10kΩ pull-ups on module |
| **13** (PE3) | SCL | MAX17048 + SHT3x | 10kΩ pull-ups on module |
| 3.3V | VCC | Both | — |
| GND | GND | Both | — |

> ⚠️ **Do NOT use Pin 9/10 for I2C** — conflicts with DMIC / ADS1256 DRDY&CS.

| Device | I2C Address | Function |
|--------|-------------|----------|
| MAX17048 | `0x36` | Battery voltage (78.125 µV/LSB) & SOC (1/256 %/LSB) |
| SHT3x | `0x44` | Temperature & humidity, CRC8 validated |

---

## Why This Works (Ameba I2C Workarounds)

The RTL8735B I2C implementation has quirks that break standard Arduino `Wire` patterns:

| Workaround | Location | Why |
|------------|----------|-----|
| `Wire.requestFrom((int)addr, 2)` | `readReg16()` | `uint8_t` overload is broken in SDK 4.0.9 |
| `delay(1)` after `requestFrom()` | `readReg16()`, `readSHT3x()` | Buffer not ready immediately |
| `Wire.setClock(100000)` only | `setup()` | 400 kHz causes bus errors |
| Single `Wire.begin()` | `setup()` | Re-calling `begin()` corrupts bus state |
| VERSION mask `0xFFF0` | `max17048_init()` | Chip returns `0x0012`, `0x001F`, etc. |
| `endTransmission(false)` for reads | `readReg16()` | Required for repeated-start |

---

## Serial Output

### Startup

```
========================================
  MAX17048 + SHT3x Combined
  SDA=Pin12  SCL=Pin13
========================================

SHT3x init... OK
MAX17048 init... OK

--- Starting readings (10s interval) ---
```

### Verified Output

```
Temp: 24.67C  |  Hum: 42.90%RH  |  Batt: 3.899V  |  SOC: 64.1%
Temp: 24.72C  |  Hum: 43.81%RH  |  Batt: 3.899V  |  SOC: 64.1%
Temp: 24.82C  |  Hum: 43.85%RH  |  Batt: 3.899V  |  SOC: 64.0%
Temp: 24.93C  |  Hum: 43.29%RH  |  Batt: 3.899V  |  SOC: 64.0%
Temp: 25.00C  |  Hum: 42.78%RH  |  Batt: 3.899V  |  SOC: 64.0%
```

> Readings every **10 seconds** (`delay(10000)` in `loop()`).

---

## Build Instructions

1. **Arduino IDE 2.x**
2. **Board:** `AmebaPro2` → `AMB82-mini`
3. **SDK:** 4.0.9-build20250805 (SPI breaks on 4.1)
4. **Compiler warnings:** `None` (suppress harmless SDK warnings)
5. **Library:** `Wire` (built-in)

Open `98_MAX_SH/98_MAX_SH.ino` in Arduino IDE. Verify & Upload.

---

## JSON Payload (for server integration)

When integrated into `90_sleep_state`, the WebSocket payload includes:

```json
{
  "type": "data",
  "lc1": 123.45,
  "lc2": 123.45,
  "lc3": 123.45,
  "lc4": 123.45,
  "total": 493.80,
  "temp": 24.67,
  "hum": 42.90,
  "batt_v": 3.899,
  "batt_soc": 64.1
}
```

Fields `temp`, `hum`, `batt_v`, `batt_soc` are `NaN` if the corresponding sensor failed to read.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `MAX17048 init... FAIL` | No battery connected | Connect Li-Po to BAT+/BAT- |
| `SHT3x init... FAIL` | Wrong ADDR pin | ADDR → GND = `0x44`, ADDR → VCC = `0x45` |
| `0xFFFF` on all reads | I2C bus locked | Power-cycle, check pull-ups |
| `VERSION = 0x0012` | Different chip rev | OK — mask `0xFFF0` handles it |
| All I2C addresses ACK | Ameba quirk | Harmless, does not affect function |

---

## Related Repos

| Repo | Purpose |
|------|---------|
| [`max17048-amebapro2`](https://github.com/SmartShelfAI/max17048-amebapro2) | Standalone MAX17048 driver |
| [`sht3x-amebapro2`](https://github.com/SmartShelfAI/sht3x-amebapro2) | Standalone SHT3x driver |
| [`max17048-sht3x-debug`](https://github.com/SmartShelfAI/max17048-sht3x-debug) | Debug workspace (`97_MAX_SH`) |
| [`90_sleep_state`](https://github.com/SmartShelfAI/90_sleep_state) | Main scales firmware with deep sleep |

---

## License

MIT
