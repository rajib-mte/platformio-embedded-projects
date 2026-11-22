#include <Arduino.h>

/* =========================================================
   RS485_RX_ESP32 : ESP32 RS485 receiver (based on 8.NANO_RS485)
   Prints each received RS485 message on the serial monitor.

   WIRING (MAX485 module)
     MAX485 RO      -> GPIO16 (Serial2 RX)
     MAX485 DI      -> GPIO17 (Serial2 TX, unused)
     MAX485 DE + RE -> GPIO4  (held LOW = receive)
     MAX485 A / B   -> RS485 bus A / B
     GND common with the sender
   ========================================================= */

#define RS485_RX_PIN   16
#define RS485_TX_PIN   17
#define RS485_DE_RE    4      // LOW = receive
#define RS485_BAUD     9600   // must match the sender
#define MONITOR_BAUD   115200
#define IDLE_GAP_MS    50     // gap that ends a message

static uint8_t buf[64];
static uint8_t len = 0;
static unsigned long lastByteMs = 0;

static void flushFrame() {
  if (len == 0) return;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t c = buf[i];
    if (c == '\r' || c == '\n') continue;           // line end added below
    Serial.write((c >= 32 && c <= 126) ? c : '.');
  }
  Serial.println();
  len = 0;
}

void setup() {
  Serial.begin(MONITOR_BAUD);
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);   // receive mode
  Serial2.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
}

void loop() {
  while (Serial2.available()) {
    buf[len++] = (uint8_t)Serial2.read();
    lastByteMs = millis();
    if (len >= sizeof(buf)) flushFrame();
  }
  if (len > 0 && millis() - lastByteMs >= IDLE_GAP_MS) flushFrame();
}
