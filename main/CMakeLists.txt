idf_component_register(SRCS "attack_dos.c" "attack_method.c" "main.c" "attack_handshake.c" "attack_pmkid.c" "attack.c" "attack_beacon_spam.c" "attack_probe_sniff.c"
                    INCLUDE_DIRS .
                    REQUIRES wifi_controller frame_analyzer wsl_bypasser pcap_serializer hccapx_serializer webserver esp_timer esp_event esp_wifi log nvs_flash)
