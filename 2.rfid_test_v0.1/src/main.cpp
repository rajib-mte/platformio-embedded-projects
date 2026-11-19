
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// RFID pin configuration
#define SS_PIN   5
#define RST_PIN  22

// Create RFID object
MFRC522 rfid(SS_PIN, RST_PIN);

// Check RFID module connection
void checkRFID()
{
    byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);

    Serial.print("RC522 Version: 0x");
    Serial.println(version, HEX);

    if (version == 0x91 || version == 0x92 || version == 0x88)
    {
        Serial.println("RFID MODULE CONNECTED");
    }
    else
    {
        Serial.println("RFID MODULE CONNECTION FAILED");
        while (true)
        {
            delay(1000);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    // Initialize SPI
    SPI.begin(18, 19, 23, SS_PIN);

    // Initialize RFID reader
    rfid.PCD_Init();

    delay(50);

    Serial.println("\nESP32 RFID RC522");

    checkRFID();

    Serial.println("Waiting for RFID cards...");
}

void loop()
{
    // Check for a new RFID card
    if (!rfid.PICC_IsNewCardPresent())
    {
        return;
    }

    // Read RFID card UID
    if (!rfid.PICC_ReadCardSerial())
    {
        return;
    }

    // Print card UID
    Serial.print("Card UID: ");

    for (byte i = 0; i < rfid.uid.size; i++)
    {
        if (rfid.uid.uidByte[i] < 0x10)
        {
            Serial.print("0");
        }

        Serial.print(rfid.uid.uidByte[i], HEX);

        if (i < rfid.uid.size - 1)
        {
            Serial.print(" ");
        }
    }

    Serial.println();

    // Stop communication with current card
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    delay(10);
    // Continue scanning
}
