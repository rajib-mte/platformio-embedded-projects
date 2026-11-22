#include <Arduino.h>

// ---- Pins / settings ----
#define RS485_RX_PIN   16     // RO -> GPIO16 (Serial2 RX)
#define RS485_TX_PIN   17     // DI -> GPIO17 (Serial2 TX)
#define RS485_DE_RE    4      // DE + RE tied together; HIGH = transmit
#define RS485_BAUD     9600   // must match Nano receiver
#define MONITOR_BAUD   115200
#define SEND_PERIOD_MS 1000

static unsigned long counter = 0;
static unsigned long lastSend = 0;

void setup() {
  Serial.begin(MONITOR_BAUD);
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);   // idle = receive
  Serial2.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  Serial.println("RS485 transmitter ready");
}

void loop() {
  if (millis() - lastSend < SEND_PERIOD_MS) return;
  lastSend = millis();

  String msg = "Hello RS485 #" + String(counter++) + "\r\n";

  digitalWrite(RS485_DE_RE, HIGH);  // enable driver
  Serial2.print(msg);
  Serial2.flush();                  // wait until all bits are sent
  digitalWrite(RS485_DE_RE, LOW);   // back to receive

  Serial.print("Sent: ");
  for (size_t i = 0; i < msg.length(); i++) {
    char c = msg[i];
    if (c == '\r') Serial.print("\\r");
    else if (c == '\n') Serial.print("\\n");
    else Serial.print(c);
  }
  Serial.print("  HEX:");
  for (size_t i = 0; i < msg.length(); i++) {
    Serial.printf(" %02X", (uint8_t)msg[i]);
  }
  Serial.println();
}