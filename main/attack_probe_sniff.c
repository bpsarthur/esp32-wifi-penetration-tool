#include "attack_probe_sniff.h"
#include "wifi_controller.h"
#include "sniffer.h"
#include "esp_log.h"
#include "esp_event.h"
#include "attack.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include <string.h>

static const char *TAG = "attack_probe_sniff";
static esp_timer_handle_t channel_hopper_timer;

static void channel_hopper(void *arg) {
    uint8_t ch;
    esp_wifi_get_channel(&ch, NULL);
    ch = (ch % 13) + 1;
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
}

static void mgmt_frame_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    wifi_promiscuous_pkt_t *frame = (wifi_promiscuous_pkt_t *) event_data;
    uint8_t *payload = frame->payload;
    
    // Frame Control (2 bytes)
    uint16_t frame_control = payload[0] | (payload[1] << 8);
    uint8_t type = (frame_control >> 2) & 0x03;
    uint8_t subtype = (frame_control >> 4) & 0x0F;

    // Probe Request: Type 0 (Mgmt), Subtype 4
    if (type == 0 && subtype == 4) {
        // Management Header: 24 bytes
        // Header: FC(2) + Dur(2) + DA(6) + SA(6) + BSSID(6) + Seq(2) = 24 bytes.
        
        uint8_t *body = payload + 24;
        int len = frame->rx_ctrl.sig_len - 24;
        
        int offset = 0;
        while (offset < len) {
            uint8_t tag_number = body[offset];
            uint8_t tag_length = body[offset+1];
            
            if (tag_number == 0) { // SSID
                if (tag_length > 0) {
                    char ssid[33];
                    if (tag_length > 32) tag_length = 32;
                    memcpy(ssid, &body[offset+2], tag_length);
                    ssid[tag_length] = 0;
                    
                    // Log it
                    char log_msg[128];
                    // Format: "Probe: <SSID> from <MAC>"
                    snprintf(log_msg, sizeof(log_msg), "Probe: %s [%02X:%02X:%02X:%02X:%02X:%02X]\n", 
                        ssid, 
                        payload[10], payload[11], payload[12], payload[13], payload[14], payload[15]);
                    
                    attack_append_status_content((uint8_t*)log_msg, strlen(log_msg));
                }
                break; // Found SSID, stop parsing
            }
            offset += 2 + tag_length;
        }
    }
}

void attack_probe_sniff_start(attack_config_t *attack_config) {
    ESP_LOGI(TAG, "Starting Probe Sniff...");
    wifictl_sniffer_filter_frame_types(false, true, false);
    wifictl_sniffer_start(1);
    
    ESP_ERROR_CHECK(esp_event_handler_register(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_MGMT, &mgmt_frame_handler, NULL));
    
    // Start Hopper
    const esp_timer_create_args_t timer_args = {
        .callback = &channel_hopper,
        .name = "hopper"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &channel_hopper_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(channel_hopper_timer, 500000)); // 500ms
}

void attack_probe_sniff_stop(void) {
    ESP_ERROR_CHECK(esp_timer_stop(channel_hopper_timer));
    esp_timer_delete(channel_hopper_timer);
    ESP_ERROR_CHECK(esp_event_handler_unregister(SNIFFER_EVENTS, SNIFFER_EVENT_CAPTURED_MGMT, &mgmt_frame_handler));
    wifictl_sniffer_stop();
    ESP_LOGI(TAG, "Probe Sniff stopped.");
}
