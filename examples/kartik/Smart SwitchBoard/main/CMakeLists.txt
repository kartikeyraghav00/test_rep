idf_component_register(SRCS "main.c"
                            "json_parse.c"
                            "mqtt_plug.c"
                            "ssl_rest.c"
                            "gatt_svr.c"
                            "ota_bulb_ota.c"
                            "metering.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
