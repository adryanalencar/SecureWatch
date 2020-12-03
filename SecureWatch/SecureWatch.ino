#include "./esppl_functions.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>

#define LED_USE_MODE 14
#define LED_SECURE_MODE 12
#define PUSH_BUTTON 15

String device_mac_address = "";
String wifi_password = "";
byte user_mac_address[6];
bool secure_mode = false;
bool configured = false;
long int start_count;


ESP8266WebServer server(80);

void setup()
{
    Serial.begin(115200);
    Serial.println();

    read_config();

    pinMode(LED_USE_MODE, OUTPUT);
    pinMode(LED_SECURE_MODE, OUTPUT);
    pinMode(PUSH_BUTTON, INPUT);

    if (!configured)
    {
        hotspot();
    }

    start_count = millis();   

    digitalWrite(LED_SECURE_MODE, secure_mode);
    digitalWrite(LED_USE_MODE, !secure_mode);

    config_sniffer();
}

void hotspot()
{
    Serial.println("Setting UP soft-ap...");
    WiFi.softAP("WATCH_" + device_mac_address);
    initialize_webserver();
}

void initialize_webserver()
{
    long int now = millis();
    bool led = false;    

    server.onNotFound(handleNotFound);
    server.on("/index", HTTP_GET, handleRoot);
    server.on("/setMacAddress", HTTP_POST, handleSetMacAddress);
    server.on("/setWifiPassword", HTTP_POST, handleSetWifiPassword);
    server.on("/save", HTTP_GET, handleSave);
    server.on("/reboot", HTTP_GET, handleReboot);

    server.begin();    

    while (true)
    {
        if(millis() - now > 500){
            digitalWrite(LED_SECURE_MODE, led);
            digitalWrite(LED_USE_MODE, !led);
            led = !led;
            now = millis();
        }
        
        server.handleClient();
    }
}

void read_config()
{

    EEPROM.begin(255);

    device_mac_address = WiFi.macAddress();
    device_mac_address.replace(":", "");

    configured = (EEPROM.read(254) != 1) ? false : true;

    if (configured)
    {
        secure_mode = EEPROM.read(253);

        for (int i = 0; i < 8; i++)
        {
            wifi_password += (char)EEPROM.read(i);
        }

        for (int i = 8; i < 14; i++)
        {
            user_mac_address[i - 8] = EEPROM.read(i);
        }
    }

    EEPROM.end();
}

void handleNotFound()
{
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void handleRoot()
{
    server.send(200, "text/plain", "Hello world!"); // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleSetMacAddress()
{
    user_mac_address[0] = (byte)server.arg("mac1").toInt();
    user_mac_address[1] = (byte)server.arg("mac2").toInt();
    user_mac_address[2] = (byte)server.arg("mac3").toInt();
    user_mac_address[3] = (byte)server.arg("mac4").toInt();
    user_mac_address[4] = (byte)server.arg("mac5").toInt();
    user_mac_address[5] = (byte)server.arg("mac6").toInt();

    server.send(200, "application/json", "{\"status\":\"success\"}");
}

void handleSetWifiPassword()
{
    String temp = server.arg("passwd");

    if (temp.length() != 8)
    {
        server.send(403, "application/json", "{\"status\":\"error\", \"error_code\":1}");
    }

    wifi_password = temp;

    server.send(200, "application/json", "{\"status\":\"success\"}");
}

void handleSave()
{
    EEPROM.begin(255);
    int i = 0;
    for (char c : wifi_password)
    {
        EEPROM.write(i, (byte)c);
        i++;
    }

    i = 8;

    for (byte c : user_mac_address)
    {
        EEPROM.write(i, c);
        i++;
    }

    EEPROM.write(254, 1);
    Serial.println(EEPROM.commit());
    EEPROM.end();

    server.send(200, "application/json", "{\"status\":\"success\"}");
}

void handleReboot()
{
    reboot();
}

void handleReset()
{
    erase_eeprom();
    server.send(200, "application/json", "{\"status\":\"success\"}");
}

void reboot()
{
    Serial.println("Rebooting...");
    while (true)
    {
    }
}

void config_sniffer()
{
    Serial.println("Sniffer initialyzed");
    esppl_init(cb);
    esppl_sniffing_start();
    WiFi.setOutputPower(4.0);
}

void erase_eeprom()
{
    EEPROM.begin(255);
    for (int i = 0; i < 255; i++)
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
    EEPROM.end();
}

bool mac_compare(uint8_t *mac1, uint8_t *mac2)
{
    for (int i = 0; i < ESPPL_MAC_LEN; i++)
    {
        if (mac1[i] != mac2[i])
        {
            return false;
        }
    }
    return true;
}

void print(esppl_frame_info *info)
{
    Serial.print("{\"source\":\"");
    for (int i = 0; i < 6; i++)
    {
        if (info->sourceaddr[i] < 16)
        {
            Serial.print("0");
        }
        Serial.print(info->sourceaddr[i], HEX);
        if (i != 5)
        {
            Serial.print(":");
        }
    }
    Serial.print("\",\"RSSI\":\"");
    Serial.print(info->rssi);
    Serial.print("\",\"RECEIVER\":\"");

    for (int i = 0; i < 6; i++)
    {
        if (info->receiveraddr[i] < 16)
        {
            Serial.print("0");
        }
        Serial.print(info->receiveraddr[i], HEX);
        if (i != 5)
        {
            Serial.print(":");
        }
    }
    Serial.print("\"}");
    Serial.println("");
}

void alert()
{
    Serial.println("ALERTA");
}

void cb(esppl_frame_info *info)
{

    if (mac_compare(info->sourceaddr, user_mac_address))
    {

        print(info);

        if (info->rssi < -40)
        {
            alert();
        }
    }
}

void set_function_state()
{
    EEPROM.begin(255);

    secure_mode = !secure_mode;
    EEPROM.write(253, secure_mode);

    EEPROM.commit();
    EEPROM.end();
}

void loop()
{

    if(digitalRead(PUSH_BUTTON)){
        start_count = millis();        

        while(digitalRead(PUSH_BUTTON)){
            if(millis() - start_count > 9000){
                digitalWrite(LED_SECURE_MODE, HIGH);
                digitalWrite(LED_USE_MODE, HIGH);
                erase_eeprom();
                reboot();
            }
        }
        
        set_function_state();
        digitalWrite(LED_SECURE_MODE, secure_mode);
        digitalWrite(LED_USE_MODE, !secure_mode);
        
    }

    if(secure_mode){

        for (int i = ESPPL_CHANNEL_MIN; i <= ESPPL_CHANNEL_MAX; i++)
        {
            esppl_set_channel(i);
            while (esppl_process_frames())
            {
                //
            }
        }
    }
}