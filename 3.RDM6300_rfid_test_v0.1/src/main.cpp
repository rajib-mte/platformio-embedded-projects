#include <Arduino.h>

#define RFID_RX_PIN 16
#define RFID_TX_PIN 17

HardwareSerial RFID(2);

String rfidData = "";


int cnt = 0;

void setup()
{
    Serial.begin(115200);

    RFID.begin(9600, SERIAL_8N1,
               RFID_RX_PIN, RFID_TX_PIN);

    Serial.println("RDM6300 Ready...");
}

void loop()
{
    while (RFID.available())
    {
        char data = RFID.read();
        // Serial.print(data);

        // Start of RFID frame
        if (data == 0x02)
        {
            rfidData = "";
            rfidData += data;
        }

        // Collect RFID data
        else if (rfidData.length() > 0)
        {
            rfidData += data;
        }

        // End of RFID frame
        if (data == 0x03)
        {
            if (rfidData.length() == 14)
            {
                String cardID = rfidData.substring(1, 9);

                cnt++;
                Serial.print("Counter = ");
                Serial.print(cnt);
                Serial.print(" > ");

                Serial.print("Card ID: ");
                Serial.println(cardID);
            }

            rfidData = "";
        }
    }
}
