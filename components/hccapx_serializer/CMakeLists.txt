idf_component_register(SRCS "hccapx_serializer.c"
                    INCLUDE_DIRS "interface"
                    REQUIRES esp_wifi
                    PRIV_REQUIRES frame_analyzer)