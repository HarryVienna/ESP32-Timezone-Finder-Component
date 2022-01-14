
#pragma once

#include "driver/gpio.h"

#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"


#include "esp_err.h"


/**
* @brief TZ database Configuration Type
*
*/
typedef struct tz_database_conf_s {
        int mosi_io_num;      
        int miso_io_num;              
        int sclk_io_num;          
        int cs_io_num; 
        spi_host_device_t host_id; 
        esp_flash_io_mode_t io_mode;
        esp_flash_speed_t speed;
} tz_database_conf_t;

/**
* @brief TZ Database Type
*
*/
typedef struct tz_database_s tz_database_t;


/**
* @brief Declare of TZ database Type
*
*/
struct tz_database_s {

    esp_err_t (*init)(tz_database_t *tzdb);

    char *(*find_timezone)(tz_database_t *tzdb, float latitude, float longitude);

};


/**
* @brief Install a new TZ database
*
* @param config: TZ database configuration
* @return
*      TZ database instance or NULL
*/
tz_database_t *tz_database_new_w25q128(const tz_database_conf_t *config);



/**
 * @brief TZ Database header
 *
 */
typedef struct {
    uint8_t version; 
    char signature[4];  
    uint8_t precision;
    char creation_date[10]; 
    char filler[16]; 
} tz_database_header_t;

/**
 * @brief TZ Database entry
 *
 */
typedef struct {
    char tz_name[64]; 
    char tz_value[64]; 
    uint32_t position;
} tz_database_entry_t;

/**
 * @brief TZ Database bounding box
 *
 */
typedef struct {
    int32_t from_latitude;
    int32_t from_longitude;
    int32_t to_latitude;
    int32_t to_longitude;
} tz_database_boundingbox_t;

/**
 * @brief TZ Shape entry
 *
 */
typedef struct { 
    uint32_t position;
} tz_database_shape_t;

/**
 * @brief TZ Database point
 *
 */
typedef struct {
    int32_t latitude;
    int32_t longitude;
} tz_database_point_t;