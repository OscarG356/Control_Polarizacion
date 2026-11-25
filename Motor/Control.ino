#include <Wire.h>

// ================== AS5600 ==================
#define AS5600_ADDR 0x36
#define ANGLE_REG   0x0E
#define AS5600_SDA  21
#define AS5600_SCL  22

// ================== TMC2160-OC ==================
const int stepPin = 25;
const int dirPin  = 26;
const int enPin   = 27;

// ================== PID ==================
float setpoint      = 180.0;   // ahora puede ser cualquier ángulo: 720, -540, etc.
float Kp = 2.0, Ki = 0.05, Kd = 0.5;

float error = 0, lastError = 0, integral = 0;
float output = 0;
float currentAngle = 0;        // ángulo absoluto multi-vuelta

// ================== LÍMITES ==================
const float umbral_minimo = 0.5;
const float maxSpeed      = 800.0;
const float deadband      = 0.3;
const unsigned long PID_PERIOD_MS = 5;

// ================== DELTA DE PARADA ==================
float deltaParada = 1.0;       // si |error| < deltaParada, no se mueve (grados)

// ================== MULTI-VUELTA ==================
float lastRawAngle = 0.0;      // última lectura 0-360
long  vueltas      = 0;        // vueltas acumuladas (puede ser negativo)
bool  primeraLectura = true;

// ================== GENERADOR DE PASOS ==================
const int   PASOS_POR_VUELTA = 3200;
const float GRADOS_POR_PASO  = 360.0 / PASOS_POR_VUELTA;
const int   PULSO_US         = 5;
unsigned long lastStepTime   = 0;
unsigned long lastPIDTime    = 0;
bool stepState = LOW;

// ================== SERIAL ==================
String cmdBuffer = "";
bool verbose = false;          // silencioso por defecto

void setup() {
  Serial.begin(115200);
  Wire.begin(AS5600_SDA, AS5600_SCL);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin,  OUTPUT);
  pinMode(enPin,   OUTPUT);
  digitalWrite(enPin, LOW);
  digitalWrite(stepPin, LOW);

  lastPIDTime  = millis();
  lastStepTime = micros();

  Serial.println("Listo. 'h' para ayuda. Modo silencioso activo.");
}

void loop() {
  unsigned long nowMs = millis();
  if (nowMs - lastPIDTime >= PID_PERIOD_MS) {
    currentAngle = readAS5600MultiVuelta();
    computePID((nowMs - lastPIDTime) / 1000.0);
    lastPIDTime = nowMs;
  }

  generarPasos();
  procesarSerial();

  // Telemetría SOLO si verbose=true
  if (verbose && (millis() % 200 < PID_PERIOD_MS)) {
    Serial.print("SP:");  Serial.print(setpoint, 1);
    Serial.print(" ANG:"); Serial.print(currentAngle, 2);
    Serial.print(" ERR:"); Serial.print(error, 2);
    Serial.print(" OUT:"); Serial.println(output, 2);
  }
}

// ======================================================
// Lee el AS5600 y acumula vueltas para dar ángulo absoluto
float readAS5600MultiVuelta() {
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(ANGLE_REG);
  if (Wire.endTransmission(false) != 0) return currentAngle;

  Wire.requestFrom(AS5600_ADDR, 2);
  if (Wire.available() < 2) return currentAngle;

  uint16_t raw = (Wire.read() << 8) | Wire.read();
  float rawAngle = (raw * 360.0) / 4096.0;

  if (primeraLectura) {
    lastRawAngle = rawAngle;
    primeraLectura = false;
    vueltas = 0;
    return rawAngle;
  }

  // Detectar cruce de 0/360
  float diff = rawAngle - lastRawAngle;
  if (diff >  180.0) vueltas--;       // pasó de ~360 a ~0 (bajando)
  if (diff < -180.0) vueltas++;       // pasó de ~0 a ~360 (subiendo)

  lastRawAngle = rawAngle;
  return rawAngle + vueltas * 360.0;
}

void computePID(float dt) {
  if (dt <= 0) return;

  error = setpoint - currentAngle;

  // Delta de parada: si está dentro de la tolerancia, no tocar nada
  if (abs(error) < deltaParada) {
    integral = 0;
    output   = 0;
    lastError = error;
    return;
  }

  integral += error * dt;
  integral = constrain(integral, -maxSpeed, maxSpeed);

  float derivative = (error - lastError) / dt;
  output = Kp * error + Ki * integral + Kd * derivative;
  output = constrain(output, -maxSpeed, maxSpeed);

  lastError = error;
}

void generarPasos() {
  if (abs(output) < umbral_minimo) return;

  float pasosPorSeg = abs(output) / GRADOS_POR_PASO;
  if (pasosPorSeg < 1) return;

  unsigned long periodoUs = (unsigned long)(1000000.0 / pasosPorSeg);
  unsigned long nowUs = micros();

  if (stepState == LOW) {
    if (nowUs - lastStepTime >= periodoUs) {
      digitalWrite(dirPin, output > 0 ? HIGH : LOW);
      digitalWrite(stepPin, HIGH);
      stepState   = HIGH;
      lastStepTime = nowUs;
    }
  } else {
    if (nowUs - lastStepTime >= PULSO_US) {
      digitalWrite(stepPin, LOW);
      stepState    = LOW;
      lastStepTime = nowUs;
    }
  }
}

// ======================================================
void procesarSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmdBuffer.length() > 0) {
        ejecutarComando(cmdBuffer);
        cmdBuffer = "";
      }
    } else {
      cmdBuffer += c;
      if (cmdBuffer.length() > 64) cmdBuffer = "";
    }
  }
}

void ejecutarComando(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  char op = cmd.charAt(0);

  switch (op) {
    case 's': {  // setpoint multi-vuelta
      float v = cmd.substring(1).toFloat();
      setpoint = v;
      integral = 0;
      lastError = 0;
      Serial.print("SP="); Serial.println(setpoint, 2);
      break;
    }

    case 'd': {  // delta de parada
      float v = cmd.substring(1).toFloat();
      if (v > 0) deltaParada = v;
      Serial.print("Delta="); Serial.println(deltaParada, 3);
      break;
    }

    case 'p': {  // ganancias
      float kp, ki, kd;
      if (sscanf(cmd.c_str(), "p %f %f %f", &kp, &ki, &kd) == 3) {
        Kp = kp; Ki = ki; Kd = kd;
        integral = 0; lastError = 0;
        Serial.print("Kp="); Serial.print(Kp, 3);
        Serial.print(" Ki="); Serial.print(Ki, 3);
        Serial.print(" Kd="); Serial.println(Kd, 3);
      } else {
        Serial.println("Uso: p <kp> <ki> <kd>");
      }
      break;
    }

    case 'v': {  // toggle verbose
      verbose = !verbose;
      Serial.print("Verbose="); Serial.println(verbose ? "ON" : "OFF");
      break;
    }

    case 'z': {  // resetear contador de vueltas
      vueltas = 0;
      primeraLectura = true;
      Serial.println("Vueltas reseteadas a 0");
      break;
    }

    case 'r': {
      Serial.print("SP=");  Serial.print(setpoint, 2);
      Serial.print(" ANG="); Serial.print(currentAngle, 2);
      Serial.print(" ERR="); Serial.print(error, 2);
      Serial.print(" OUT="); Serial.print(output, 2);
      Serial.print(" VUELTAS="); Serial.println(vueltas);
      break;
    }

    case 'h': {
      Serial.println("Comandos:");
      Serial.println("  s <grados>       setpoint (puede ser >360 o negativo)");
      Serial.println("  d <grados>       delta de parada (tolerancia)");
      Serial.println("  p <kp> <ki> <kd> ganancias PID");
      Serial.println("  v                toggle telemetria periodica");
      Serial.println("  z                resetear contador de vueltas");
      Serial.println("  r                estado actual");
      Serial.println("  h                ayuda");
      break;
    }

    default:
      Serial.print("Comando desconocido: ");
      Serial.println(cmd);
  }
}