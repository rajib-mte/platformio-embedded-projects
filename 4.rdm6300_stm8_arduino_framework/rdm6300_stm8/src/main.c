
#include <Arduino.h>

#define LED_PIN PA1

void setup()
{
    pinMode(LED_PIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_PIN, HIGH);
    delay(10);

    digitalWrite(LED_PIN, LOW);
    delay(10);
}
