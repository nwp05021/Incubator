# src/CMakeLists.txt 전체 내용
FILE(GLOB_RECURSE app_sources *.cpp *.c)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "../include"
    REQUIRES bt protocomm wifi_provisioning nvs_flash esp_wifi esp_event esp_netif
)