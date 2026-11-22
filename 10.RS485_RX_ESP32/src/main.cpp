#include <Arduino.h>

/* =========================================================
   10.RS485_RX_ESP32 : ESP32 RS485 receiver (based on 8.NANO_RS485)
   Prints the incoming RS485 data on the serial monitor exactly
   as received. No counter, no labels, no extra output.

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

void setup() {
  Serial.begin(MONITOR_BAUD);
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);   // receive mode
  Serial2.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
}

void loop() {
  while (Serial2.available()) {
    Serial.write(Serial2.read());   // raw byte, exactly as received
  }
}
