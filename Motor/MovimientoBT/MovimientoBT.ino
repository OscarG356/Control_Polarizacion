#include <Arduino.h>
#include <AccelStepper.h>
#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth no está habilitado en este core de ESP32
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error El perfil SPP no está disponible. Solo funciona en el ESP32 original.
#endif

constexpr int PIN_STEP = 18;
constexpr int PIN_DIR  = 19;

// Una vuelta completa = 200 pasos * 8 micropasos = 1600
constexpr long STEPS_PER_REV = 1600;

BluetoothSerial SerialBT;
const char* BT_NAME = "ESP32-Motor";

AccelStepper stepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR);
String inputBuffer = "";
bool enMovimiento = false;

void setup() {
  Serial.begin(115200);
  delay(300);

  SerialBT.begin(BT_NAME);
  Serial.print("Bluetooth iniciado: ");
  Serial.println(BT_NAME);

  stepper.setMaxSpeed(1600);
  stepper.setAcceleration(1200);

  // Arrancar en 0 (posición de referencia)
  stepper.setCurrentPosition(0);

  Serial.print("Rango permitido: 0 a ");
  Serial.print(STEPS_PER_REV);
  Serial.println(" pasos (0-360 grados).");
  Serial.println("Comandos: <n> relativo, p<n> absoluto, stop, pos, v<n>, a<n>");
}

void loop() {
  // --- Leer comandos desde Bluetooth ---
  while (SerialBT.available() > 0) {
    char c = SerialBT.read();
    if (c == '\n' || c == '\r') {
      inputBuffer.trim();
      if (inputBuffer.length() > 0) {
        Serial.print("[BT] Comando recibido: ");
        Serial.println(inputBuffer);
        procesarComando(inputBuffer);
      }
      inputBuffer = "";
    } else {
      inputBuffer += c;
      if (inputBuffer.length() > 20) inputBuffer = "";
    }
  }

  // --- Ejecutar movimiento ---
  stepper.run();

  // --- Avisar cuando termina (por USB) ---
  if (enMovimiento && stepper.distanceToGo() == 0) {
    enMovimiento = false;
    Serial.print("[OK] Llegado. Posicion: ");
    Serial.print(stepper.currentPosition());
    Serial.print(" (");
    Serial.print((float)stepper.currentPosition() * 360.0 / STEPS_PER_REV, 2);
    Serial.println(" grados)");
    // Opcional: avisar tambien al movil
    // SerialBT.println("OK");
  }
}

void procesarComando(String cmd) {
  cmd.toLowerCase();

  if (cmd == "stop") {
    stepper.stop();
    Serial.println("[CMD] Deteniendo...");
    // SerialBT.println("STOP");
    return;
  }

  if (cmd == "pos") {
    Serial.print("[POS] Posicion: ");
    Serial.print(stepper.currentPosition());
    Serial.print(" (");
    Serial.print((float)stepper.currentPosition() * 360.0 / STEPS_PER_REV, 2);
    Serial.println(" grados)");
    // SerialBT.print("POS ");
    // SerialBT.println(stepper.currentPosition());
    return;
  }

  if (cmd.startsWith("v")) {
    long v = cmd.substring(1).toInt();
    if (v > 0) {
      stepper.setMaxSpeed(v);
      Serial.print("[CFG] Velocidad: ");
      Serial.println(v);
      // SerialBT.print("V ");
      // SerialBT.println(v);
    }
    return;
  }

  if (cmd.startsWith("a")) {
    long a = cmd.substring(1).toInt();
    if (a > 0) {
      stepper.setAcceleration(a);
      Serial.print("[CFG] Aceleracion: ");
      Serial.println(a);
      // SerialBT.print("A ");
      // SerialBT.println(a);
    }
    return;
  }

  // --- Posición absoluta con límite ---
  if (cmd.startsWith("p")) {
    long destino = cmd.substring(1).toInt();
    if (destino < 0 || destino > STEPS_PER_REV) {
      Serial.print("[ERR] Fuera de rango (0-");
      Serial.print(STEPS_PER_REV);
      Serial.println(")");
      // SerialBT.println("ERR_RANGO");
      return;
    }
    stepper.moveTo(destino);
    enMovimiento = true;
    Serial.print("[ABS] Yendo a ");
    Serial.print(destino);
    Serial.println(" pasos (absoluto)");
    // SerialBT.println("OK");
    return;
  }

  // --- Movimiento relativo con límite ---
  bool valido = true;
  int inicio = (cmd[0] == '-' || cmd[0] == '+') ? 1 : 0;
  if (cmd.length() == inicio) valido = false;
  for (int i = inicio; i < cmd.length(); i++) {
    if (!isDigit(cmd[i])) { valido = false; break; }
  }
  if (!valido) {
    Serial.print("[ERR] Comando no valido: ");
    Serial.println(cmd);
    // SerialBT.println("ERR_CMD");
    return;
  }

  long pasos = cmd.toInt();
  long destino = stepper.currentPosition() + pasos;

  if (destino < 0 || destino > STEPS_PER_REV) {
    Serial.print("[ERR] Destino fuera de rango (0-");
    Serial.print(STEPS_PER_REV);
    Serial.print("). Posicion actual: ");
    Serial.println(stepper.currentPosition());
    // SerialBT.println("ERR_RANGO");
    return;
  }

  stepper.moveTo(destino);
  enMovimiento = true;
  Serial.print("[REL] Moviendo ");
  Serial.print(pasos);
  Serial.print(" -> destino ");
  Serial.println(destino);
  // SerialBT.println("OK");
}