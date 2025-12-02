#include "attack_beacon_spam.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "wsl_bypasser.h"

static const char *TAG = "attack_beacon_spam";
static esp_timer_handle_t beacon_timer_handle;
static bool is_running = false;

// Basic Beacon Frame Template (Fixed parts)
static const uint8_t beacon_header[] = {
    0x80, 0x00,             // Frame Control (Beacon)
    0x00, 0x00,             // Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination (Broadcast)
    // Source (6) + BSSID (6) + Seq (2) to be filled dynamically
};

static const uint8_t beacon_fixed_params[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
    0x64, 0x00,             // Beacon Interval
    0x11, 0x04              // Capabilities
};

static const uint8_t beacon_rates_channel[] = {
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Supported Rates
    0x03, 0x01, 0x01        // Channel 1
};

static void timer_send_beacon(void *arg) {
    if (!is_running) return;

    uint8_t frame[128];
    int offset = 0;

    // Header
    memcpy(&frame[offset], beacon_header, sizeof(beacon_header));
    offset += sizeof(beacon_header);

    // Randomize Source/BSSID
    uint8_t mac[6];
    esp_fill_random(mac, 6);
    mac[0] &= 0xFE; // Unicast
    mac[0] |= 0x02; // Locally Administered

    memcpy(&frame[offset], mac, 6); // Source
    offset += 6;
    memcpy(&frame[offset], mac, 6); // BSSID
    offset += 6;

    // Sequence Control
    frame[offset++] = 0x00;
    frame[offset++] = 0x00;

    // Fixed Params
    memcpy(&frame[offset], beacon_fixed_params, sizeof(beacon_fixed_params));
    offset += sizeof(beacon_fixed_params);

    // Randomize SSID
    const char *ssids[] = {
        "Free WiFi", "Starbucks", "iPhone", "AndroidAP", 
        "Virus.exe", "FBI Surveillance", "Skynet", "Loading...", 
        "Connection Lost", "Error 404", "Trojan.win32"
    };
    const char *ssid = ssids[esp_random() % 11];
    uint8_t ssid_len = strlen(ssid);

    // SSID Tag
    frame[offset++] = 0x00; // Tag Number
    frame[offset++] = ssid_len; // Tag Length
    memcpy(&frame[offset], ssid, ssid_len);
    offset += ssid_len;

    // Rates and Channel
    memcpy(&frame[offset], beacon_rates_channel, sizeof(beacon_rates_channel));
    offset += sizeof(beacon_rates_channel);

    wsl_bypasser_send_raw_frame(frame, offset);
}

void attack_beacon_spam_start(attack_config_t *attack_config) {
    if (is_running) return;
    ESP_LOGI(TAG, "Starting Beacon Spam...");
    is_running = true;
    
    const esp_timer_create_args_t timer_args = {
        .callback = &timer_send_beacon,
        .name = "beacon_spam"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &beacon_timer_handle));
    // Send every 100ms
    ESP_ERROR_CHECK(esp_timer_start_periodic(beacon_timer_handle, 100000));
}

void attack_beacon_spam_stop(void) {
    if (!is_running) return;
    is_running = false;
    ESP_ERROR_CHECK(esp_timer_stop(beacon_timer_handle));
    esp_timer_delete(beacon_timer_handle);
    ESP_LOGI(TAG, "Beacon Spam stopped.");
}
