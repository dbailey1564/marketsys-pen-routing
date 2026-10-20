// Bench bring-up only. No indicator GPIO or radio commands are issued here.
#include <Arduino.h>

void setup()
{
    Serial.begin(115200, RAK_AT_MODE);
    delay(5000); // Leave a startup window for the bootloader entry command.
    api.system.firmwareVersion.set("MarketSys-SiP-check-0.1.0");
    Serial.println("MarketSys RAK3272-SiP build check started");
}

void loop()
{
    Serial.println("MarketSys RAK3272-SiP build check alive");
    delay(2000);
}
