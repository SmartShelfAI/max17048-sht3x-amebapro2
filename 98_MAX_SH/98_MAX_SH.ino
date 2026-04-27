/*
 * Combined: MAX17048 Battery Monitor + SHT3x Temp/Humidity Sensor
 * Platform: AmebaPRO2-mini (AMB82-mini, RTL8735B)
 *
 * I2C bus: Pin 12 (SDA), Pin 13 (SCL)
 * Both devices share the same bus with different addresses.
 *
 * Readings every 10 seconds.
 */

#include <Wire.h>

// ── I2C addresses ───────────────────────────────────────────────────────────
#define SHT3X_ADDR       0x44
#define MAX17048_ADDR    0x36

// ── SHT3x commands ──────────────────────────────────────────────────────────
#define SHT3X_CMD_MEASURE_HPM  0x2400
#define SHT3X_CMD_SOFT_RESET   0x30A2
#define SHT3X_CMD_STATUS       0xF32D

// ── MAX17048 registers ─────────────────────────────────────────────────────
#define MAX17048_REG_VER       0x08
#define MAX17048_REG_VCELL     0x02
#define MAX17048_REG_SOC       0x04
#define MAX17048_REG_MODE      0x06
#define MAX17048_REG_STATUS    0x00

// ── Prototypes ──────────────────────────────────────────────────────────────
bool sht3x_init();
bool sht3x_read(float &temperature, float &humidity);
uint8_t crc8(const uint8_t *data, int len);

bool max17048_init();
bool max17048_read(float &voltage, float &soc);
uint16_t readReg16(uint8_t addr, uint8_t reg);
void writeReg16(uint8_t addr, uint8_t reg, uint16_t val);

// ── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println(F("\n========================================"));
  Serial.println(F("  MAX17048 + SHT3x Combined"));
  Serial.println(F("  SDA=Pin12  SCL=Pin13"));
  Serial.println(F("========================================\n"));

  Wire.begin();
  Wire.setClock(100000);
  delay(100);

  // Initialize SHT3x first
  Serial.print(F("SHT3x init... "));
  if (!sht3x_init()) {
    Serial.println(F("FAIL"));
    while (1) delay(1000);
  }
  Serial.println(F("OK"));

  // Initialize MAX17048
  Serial.print(F("MAX17048 init... "));
  if (!max17048_init()) {
    Serial.println(F("FAIL — check battery connection"));
    while (1) delay(1000);
  }
  Serial.println(F("OK"));

  Serial.println(F("\n--- Starting readings (10s interval) ---\n"));
}

// ── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  float temperature, humidity;
  float voltage, soc;

  bool sht_ok = sht3x_read(temperature, humidity);
  bool max_ok = max17048_read(voltage, soc);

  Serial.print(F("Temp: "));
  if (sht_ok) {
    Serial.print(temperature, 2);
    Serial.print(F("C  |  Hum: "));
    Serial.print(humidity, 2);
    Serial.print(F("%RH"));
  } else {
    Serial.print(F("ERR"));
  }

  Serial.print(F("  |  "));

  Serial.print(F("Batt: "));
  if (max_ok) {
    Serial.print(voltage, 3);
    Serial.print(F("V  |  SOC: "));
    Serial.print(soc, 1);
    Serial.print(F("%"));
  } else {
    Serial.print(F("ERR"));
  }

  Serial.println();
  delay(10000);
}

// ════════════════════════════════════════════════════════════════════════════
//  SHT3x
// ════════════════════════════════════════════════════════════════════════════

bool sht3x_init() {
  // Soft reset
  Wire.beginTransmission((int)SHT3X_ADDR);
  Wire.write((SHT3X_CMD_SOFT_RESET >> 8) & 0xFF);
  Wire.write(SHT3X_CMD_SOFT_RESET & 0xFF);
  if (Wire.endTransmission() != 0) return false;

  delay(50);

  // Read status to verify presence
  Wire.beginTransmission((int)SHT3X_ADDR);
  Wire.write((SHT3X_CMD_STATUS >> 8) & 0xFF);
  Wire.write(SHT3X_CMD_STATUS & 0xFF);
  if (Wire.endTransmission() != 0) return false;

  delay(10);
  Wire.requestFrom((int)SHT3X_ADDR, 3);
  delay(1);

  if (Wire.available() < 3) return false;
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  uint8_t crc = Wire.read();

  uint8_t data[2] = {msb, lsb};
  if (crc8(data, 2) != crc) return false;

  return true;
}

bool sht3x_read(float &temperature, float &humidity) {
  // Trigger high-precision measurement
  Wire.beginTransmission((int)SHT3X_ADDR);
  Wire.write((SHT3X_CMD_MEASURE_HPM >> 8) & 0xFF);
  Wire.write(SHT3X_CMD_MEASURE_HPM & 0xFF);
  if (Wire.endTransmission() != 0) return false;

  delay(16);  // Max conversion time for HPM

  // Read 6 bytes: T[2] + CRC + H[2] + CRC
  Wire.requestFrom((int)SHT3X_ADDR, 6);
  delay(1);

  if (Wire.available() < 6) return false;

  uint8_t d[6];
  for (int i = 0; i < 6; i++) d[i] = Wire.read();

  if (crc8(d, 2) != d[2]) return false;
  if (crc8(d + 3, 2) != d[5]) return false;

  uint16_t tRaw = ((uint16_t)d[0] << 8) | d[1];
  uint16_t hRaw = ((uint16_t)d[3] << 8) | d[4];

  temperature = -45.0f + 175.0f * (tRaw / 65535.0f);
  humidity    = 100.0f * (hRaw / 65535.0f);

  return true;
}

uint8_t crc8(const uint8_t *data, int len) {
  uint8_t crc = 0xFF;
  for (int j = len; j; --j) {
    crc ^= *data++;
    for (int i = 8; i; --i) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
  }
  return crc;
}

// ════════════════════════════════════════════════════════════════════════════
//  MAX17048
// ════════════════════════════════════════════════════════════════════════════

bool max17048_init() {
  uint16_t ver = readReg16(MAX17048_ADDR, MAX17048_REG_VER);
  return ((ver & 0xFFF0) == 0x0010);
}

bool max17048_read(float &voltage, float &soc) {
  uint16_t vcell = readReg16(MAX17048_ADDR, MAX17048_REG_VCELL);
  uint16_t socRaw = readReg16(MAX17048_ADDR, MAX17048_REG_SOC);

  if (vcell == 0xFFFF || socRaw == 0xFFFF) return false;

  voltage = vcell * 0.000078125f;   // 78.125 uV per LSB
  soc = socRaw / 256.0f;            // 1/256 % per LSB

  return true;
}

uint16_t readReg16(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission((int)addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFFFF;

  Wire.requestFrom((int)addr, 2);
  delay(1);  // Ameba workaround

  if (Wire.available() < 2) return 0xFFFF;

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  return ((uint16_t)msb << 8) | lsb;
}

void writeReg16(uint8_t addr, uint8_t reg, uint16_t val) {
  Wire.beginTransmission((int)addr);
  Wire.write(reg);
  Wire.write((uint8_t)(val >> 8));
  Wire.write((uint8_t)(val & 0xFF));
  Wire.endTransmission();
}
