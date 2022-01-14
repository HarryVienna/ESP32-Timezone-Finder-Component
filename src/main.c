#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_log.h"

#include "tz_database.h"


static const char* TAG = "main";
 

void app_main(){

       static tz_database_conf_t tz_database_conf = {
        .mosi_io_num = SPI3_IOMUX_PIN_NUM_MOSI,
        .miso_io_num = SPI3_IOMUX_PIN_NUM_MISO,
        .sclk_io_num = SPI3_IOMUX_PIN_NUM_CLK,
        .cs_io_num= SPI3_IOMUX_PIN_NUM_CS,
        .host_id = SPI3_HOST,
        .io_mode = SPI_FLASH_DIO,
        .speed = ESP_FLASH_40MHZ,
    };

    tz_database_t *tzdb = tz_database_new_w25q128(&tz_database_conf);
    tzdb->init(tzdb);

    char* timezone = tzdb->find_timezone(tzdb, -52.02451002761944, -74.9311167387433);
    ESP_LOGI(TAG, "Timezone: %s", timezone);

}
