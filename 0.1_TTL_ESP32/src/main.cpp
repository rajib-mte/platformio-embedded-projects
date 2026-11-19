#define RXD2 16
#define TXD2 17

void setup()
{
    Serial.begin(9600);  // USB Serial Monitor

    Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

    Serial.println("ESP32 UART TTL Test Started");
}

void loop()
{
    // Send data through TTL UART
    Serial2.println("Hello from ESP32!");

    // Receive data from TTL UART
    while (Serial2.available())
    {
        char receivedData = Serial2.read();
        Serial.write(receivedData);
    }

    delay(1000);
}