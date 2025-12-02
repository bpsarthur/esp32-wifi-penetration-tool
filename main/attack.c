/**
 * @file attack.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-02
 * @copyright Copyright (c) 2021
 * 
 * @brief Implements common attack wrapper.
 */

#include "attack.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "attack_handshake.h"
#include "attack_pmkid.h"
#include "attack_dos.h"
#include "attack_beacon_spam.h"
#include "attack_probe_sniff.h"
#include "webserver.h"
#include "wifi_controller.h"

static const char* TAG = "attack";
static attack_status_t attack_status = { .state = ATTACK_READY, .type = -1, .content_size = 0, .content = NULL };
static esp_timer_handle_t attack_timeout_handle;

const attack_status_t *attack_get_status() {
    return &attack_status;
}

void attack_update_status(attack_state_t state) {
    attack_status.state = state;
    if(state == ATTACK_FINISHED) {
        esp_timer_stop(attack_timeout_handle);
    }
}

char *attack_alloc_result_content(unsigned size) {
    if(attack_status.content) {
        free(attack_status.content);
    }
    attack_status.content = (char *) malloc(size);
    attack_status.content_size = size;
    return attack_status.content;
}

void attack_append_status_content(uint8_t *buffer, unsigned size) {
    if(attack_status.content_size == 0) {
        attack_status.content = (char *) malloc(size);
    } else {
        attack_status.content = (char *) realloc(attack_status.content, attack_status.content_size + size);
    }
    memcpy(&attack_status.content[attack_status.content_size], buffer, size);
    attack_status.content_size += size;
}

static void attack_timeout(void *arg) {
    ESP_LOGD(TAG, "Attack timed out");
    attack_status.state = ATTACK_TIMEOUT;
    switch(attack_status.type) {
        case ATTACK_TYPE_HANDSHAKE:
            attack_handshake_stop();
            break;
        case ATTACK_TYPE_PMKID:
            attack_pmkid_stop();
            break;
        case ATTACK_TYPE_DOS:
            attack_dos_stop();
            break;
        case ATTACK_TYPE_BEACON_SPAM:
            attack_beacon_spam_stop();
            break;
        case ATTACK_TYPE_PROBE_SNIFF:
            attack_probe_sniff_stop();
            break;
    }
}

static void attack_request_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Starting attack...");
    attack_request_t *attack_request = (attack_request_t *) event_data;
    
    if(attack_status.state == ATTACK_RUNNING) {
        ESP_LOGW(TAG, "Attack already in progress!");
        return;
    }

    attack_status.state = ATTACK_RUNNING;
    attack_status.type = attack_request->type;
    attack_status.content_size = 0;
    if(attack_status.content) {
        free(attack_status.content);
        attack_status.content = NULL;
    }

    // Prepare attack config
    attack_config_t attack_config = {
        .type = attack_request->type,
        .method = attack_request->method,
        .timeout = attack_request->timeout,
        .ap_record = wifictl_get_ap_record(attack_request->ap_record_id)
    };

    // Start timeout timer
    ESP_ERROR_CHECK(esp_timer_start_once(attack_timeout_handle, attack_request->timeout * 1000000));

    switch(attack_request->type) {
        case ATTACK_TYPE_HANDSHAKE:
            attack_handshake_start(&attack_config);
            break;
        case ATTACK_TYPE_PMKID:
            attack_pmkid_start(&attack_config);
            break;
        case ATTACK_TYPE_DOS:
            attack_dos_start(&attack_config);
            break;
        case ATTACK_TYPE_BEACON_SPAM:
            attack_beacon_spam_start(&attack_config);
            break;
        case ATTACK_TYPE_PROBE_SNIFF:
            attack_probe_sniff_start(&attack_config);
            break;
        default:
            ESP_LOGE(TAG, "Unknown attack type!");
            break;
    }
}

static void attack_reset_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Resetting attack status...");
    if(attack_status.state == ATTACK_RUNNING) {
        esp_timer_stop(attack_timeout_handle);
        // Stop current attack
        switch(attack_status.type) {
            case ATTACK_TYPE_HANDSHAKE:
                attack_handshake_stop();
                break;
            case ATTACK_TYPE_PMKID:
                attack_pmkid_stop();
                break;
            case ATTACK_TYPE_DOS:
                attack_dos_stop();
                break;
            case ATTACK_TYPE_BEACON_SPAM:
                attack_beacon_spam_stop();
                break;
            case ATTACK_TYPE_PROBE_SNIFF:
                attack_probe_sniff_stop();
                break;
        }
    }
    if(attack_status.content){
        free(attack_status.content);
        attack_status.content = NULL;
    }
    attack_status.content_size = 0;
    attack_status.type = -1;
    attack_status.state = ATTACK_READY;
}

void attack_init(){
    const esp_timer_create_args_t attack_timeout_args = {
        .callback = &attack_timeout
    };
    ESP_ERROR_CHECK(esp_timer_create(&attack_timeout_args, &attack_timeout_handle));

    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_REQUEST, &attack_request_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WEBSERVER_EVENTS, WEBSERVER_EVENT_ATTACK_RESET, &attack_reset_handler, NULL));
}