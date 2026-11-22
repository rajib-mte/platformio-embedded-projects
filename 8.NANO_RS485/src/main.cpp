#include <Arduino.h>
#include <SoftwareSerial.h>

// ---- Pins / settings ----
#define RS485_RX_PIN   2      // RO -> D2
#define RS485_TX_PIN   3      // DI -> D3
#define RS485_DE_RE    4      // DE + RE tied together; LOW = receive
#define RS485_BAUD     9600
#define MONITOR_BAUD   9600
#define IDLE_GAP_MS    50     // gap that ends a frame

SoftwareSerial rs485(RS485_RX_PIN, RS485_TX_PIN);

static uint8_t buf[64];
static uint8_t len = 0;
static unsigned long lastByteMs = 0;
static unsigned long frameCount = 0;

static void flushFrame() {
  if (len == 0) return;
  frameCount++;
  Serial.print(F("#"));
  Serial.print(frameCount);
  Serial.print(F(" ("));
  Serial.print(len);
  Serial.print(F(" bytes) HEX: "));
  for (uint8_t i = 0; i < len; i++) {
    if (buf[i] < 0x10) Serial.print('0');
    Serial.print(buf[i], HEX);
    Serial.print(' ');
  }
  Serial.print(F("| ASCII: "));
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
  rs485.begin(RS485_BAUD);
  Serial.println(F("RS485 receiver ready"));
}

void loop() {
  while (rs485.available()) {
    buf[len++] = (uint8_t)rs485.read();
    lastByteMs = millis();
    if (len >= sizeof(buf)) flushFrame();
  }
  if (len > 0 && millis() - lastByteMs >= IDLE_GAP_MS) flushFrame();
}