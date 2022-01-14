#include <stdio.h>
#include <string.h>
#include <unity.h>


#include "esp_task_wdt.h"
#include "esp_log.h"

#include "tz_database.h"

static const char* TAG = "test";

tz_database_t *tzdb;

void setUp(void) {

    esp_task_wdt_init(60, false);

    static tz_database_conf_t tz_database_conf = {
        .mosi_io_num = SPI3_IOMUX_PIN_NUM_MOSI,
        .miso_io_num = SPI3_IOMUX_PIN_NUM_MISO,
        .sclk_io_num = SPI3_IOMUX_PIN_NUM_CLK,
        .cs_io_num= SPI3_IOMUX_PIN_NUM_CS,
        .host_id = SPI3_HOST,
        .io_mode = SPI_FLASH_DIO,
        .speed = ESP_FLASH_40MHZ,
    };

    tzdb = tz_database_new_w25q128(&tz_database_conf);
    tzdb->init(tzdb);

}

void tearDown(void) {
    // clean stuff up here
}

void tz_test(void) {
    
    // Vatikan is inside italy - Europe/Vatican
    char* vatikan = tzdb->find_timezone(tzdb, 41.902790, 12.454220);
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0,M10.5.0/3", vatikan);

    // beside vatikan - Europe/Rome
    char* italy = tzdb->find_timezone(tzdb, 41.906552, 12.457458);
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0,M10.5.0/3", italy);

    // Somewhere in italy - Europe/Rome
    char* italy2 = tzdb->find_timezone(tzdb, 44.39583, 10.64997);
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0,M10.5.0/3", italy2);

    // Sicily - Europe/Rome
    char* italy3 = tzdb->find_timezone(tzdb, 38.1262, 13.1685);
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0,M10.5.0/3", italy3);

    // Austria - Europe/Vienna
    char* austria = tzdb->find_timezone(tzdb, 47.4977,9.9474);
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0,M10.5.0/3", austria);

    // Germany - Europe/Berlin
    char* germany = tzdb->find_timezone(tzdb, 49.85752888777688, 10.18292849938819);
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0,M10.5.0/3", germany);

    // Busingen, second timezone in Germany - Europe/Busingen
    char* busingen = tzdb->find_timezone(tzdb, 47.70228,8.69860);
    TEST_ASSERT_EQUAL_STRING("CET-1CEST,M3.5.0,M10.5.0/3", busingen);
 
    // Near Moscow - Europe/Moscow
    char* russia = tzdb->find_timezone(tzdb, 55.9144, 38.0237);
    TEST_ASSERT_EQUAL_STRING("MSK-3", russia);

    // Pacific/Auckland
    char* auckland = tzdb->find_timezone(tzdb, -50.793, 166.069);
    TEST_ASSERT_EQUAL_STRING("NZST-12NZDT,M9.5.0,M4.1.0/3", auckland);
    
    // America/Rankin_Inlet
    char* rankin = tzdb->find_timezone(tzdb, 79.983,-99.365);
    TEST_ASSERT_EQUAL_STRING("CST6CDT,M3.2.0,M11.1.0", rankin);

    // Asia/Urumqi
    char* china = tzdb->find_timezone(tzdb, 44.372, 87.528);
    TEST_ASSERT_EQUAL_STRING("XJT-6", china);

    // America/Puerto_Rico
    char* puerto_rico = tzdb->find_timezone(tzdb, 18.251, -66.551);
    TEST_ASSERT_EQUAL_STRING("AST4", puerto_rico);

    // Asia/Shanghai
    char* shanghai = tzdb->find_timezone(tzdb, 31.2194, 121.4587);
    TEST_ASSERT_EQUAL_STRING("CST-8", shanghai);
}

void app_main(){

    UNITY_BEGIN();   

    RUN_TEST(tz_test);

    UNITY_END();
}
