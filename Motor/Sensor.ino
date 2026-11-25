#include <Wire.h>

#define AS5600_ADDR 0x36
#define ANGLE_REG   0x0E   // registro de ángulo (12 bits)
#define SDA_PIN     21
#define SCL_PIN     22

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(200);
  Serial.println("AS5600 listo. Leyendo ángulo...");
}

void loop() {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(ANGLE_REG);
  if (Wire.endTransmission(false) != 0) {
    Serial.println("Error I2C: no responde el AS5600");
    delay(500);
    return;
  }

  Wire.requestFrom(AS5600_ADDR, 2);
  if (Wire.available() >= 2) {
    uint16_t raw = (Wire.read() << 8) | Wire.read();   // 0..4095
    float grados = (raw * 360.0) / 4096.0;

    Serial.print("RAW: ");
    Serial.print(raw);
    Serial.print("  |  Grados: ");
    Serial.print(grados, 2);
    Serial.print("  |  Vueltas: ");
    Serial.println(grados / 360.0, 4);
  }
  delay(100);  // 10 Hz, suficiente para verificar
}