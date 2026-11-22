#include <Arduino.h>

/* =========================================================
   Project 10 : ESP32 RS485 receiver (port of 8.NANO_RS485)
   Prints every received frame as HEX and ASCII.

   WIRING (MAX485 module, same pins as 7.RS485_ESP32)
     MAX485 RO      -> GPIO16 (Serial2 RX)
     MAX485 DI      -> GPIO17 (Serial2 TX, unused while receiving)
     MAX485 DE + RE -> GPIO4  (held LOW = receive)
     MAX485 A / B   -> RS485 bus A / B (to the STM8 sender)
     MAX485 VCC     -> 5V (module supply), GND -> common ground
     Note: if the module's RO outputs 5V, use a divider or a
           3.3V-compatible module, the ESP32 pins are 3.3V.
   ========================================================= */

// ---- Pins / settings ----
#define RS485_RX_PIN   16     // RO -> GPIO16
#define RS485_TX_PIN   17     // DI -> GPIO17
#define RS485_DE_RE    4      // DE + RE tied together; LOW = receive
#define RS485_BAUD     9600   // must match the sender
#define MONITOR_BAUD   115200
#define IDLE_GAP_MS    50     // gap that ends a frame

static uint8_t buf[64];
static uint8_t len = 0;
static unsigned long lastByteMs = 0;
static unsigned long frameCount = 0;

static void flushFrame() {
  if (len == 0) return;
  frameCount++;
  Serial.print("#");
  Serial.print(frameCount);
  Serial.print(" (");
  Serial.print(len);
  Serial.print(" bytes) HEX: ");
  for (uint8_t i = 0; i < len; i++) {
    Serial.printf("%02X ", buf[i]);
  }
  Serial.print("| ASCII: ");
  for (uint8_t i = 0; i < len; i++) {
    Serial.write((buf[i] >= 32 && buf[i] <= 126) ? buf[i] : '.');
  }
  Serial.println();
  len = 0;
}

void setup() {
  Serial.begin(MONITOR_BAUD);
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);   // receive mode
  Serial2.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  Serial.println("RS485 receiver ready (ESP32)");
}

void loop() {
  while (Serial2.available()) {
    buf[len++] = (uint8_t)Serial2.read();
    lastByteMs = millis();
    if (len >= sizeof(buf)) flushFrame();
  }
  if (len > 0 && millis() - lastByteMs >= IDLE_GAP_MS) flushFrame();
}
