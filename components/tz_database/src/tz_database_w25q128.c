// esptool.py --chip esp32 write_flash --flash_mode dio  --spi-connection 18,19,23,21,5 0x0000 timezones.bin

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "tz_database.h"


static const char *TAG = "w25q128";



typedef struct {
    tz_database_t parent;
    tz_database_conf_t config;
    const esp_partition_t* partition;
} w25q128_t;

static int32_t float_to_int(float input, float scale, uint8_t precision);
static float int_to_float(int32_t input, float scale, uint8_t precision);
static bool check_inside_country(const esp_partition_t* partition, u_int32_t data_position, int32_t latitude_int, int32_t longitude_int);
static bool check_inside_shape(const esp_partition_t* partition, u_int32_t data_position, int32_t latitude_int, int32_t longitude_int);
static int32_t next_value(const esp_partition_t* partition, u_int32_t *position);


static esp_err_t w25q128_init(tz_database_t *tzdb)
{
    esp_err_t ret;

    w25q128_t *w25q128 = __containerof(tzdb, w25q128_t, parent);

   
   const spi_bus_config_t bus_config = {
        .mosi_io_num = w25q128->config.mosi_io_num,
        .miso_io_num = w25q128->config.miso_io_num,
        .sclk_io_num = w25q128->config.sclk_io_num,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    const esp_flash_spi_device_config_t device_config = {
        .host_id =  w25q128->config.host_id,
        .cs_id = 0,
        .cs_io_num = w25q128->config.cs_io_num,
        .io_mode = w25q128->config.io_mode,
        .speed = w25q128->config.speed
    };

    ESP_LOGI(TAG, "Initializing external SPI Flash");
    ESP_LOGI(TAG, "Pin assignments:");
    ESP_LOGI(TAG, "MOSI: %2d   MISO: %2d   SCLK: %2d   CS: %2d",
        bus_config.mosi_io_num, bus_config.miso_io_num,
        bus_config.sclk_io_num, device_config.cs_io_num
    );

    // Initialize the SPI bus
    ret = spi_bus_initialize(VSPI_HOST, &bus_config, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }


    // Add device to the SPI bus
    esp_flash_t* ext_flash;
    ret = spi_bus_add_flash_device(&ext_flash, &device_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add Flash: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }

    // Probe the Flash chip and initialize it
    ret = esp_flash_init(ext_flash);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external Flash: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }

    // Print out the ID and size
    uint32_t id;
    ret = esp_flash_read_id(ext_flash, &id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read id: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }
    ESP_LOGI(TAG, "Initialized external Flash, size=%d KB, ID=0x%x", ext_flash->size / 1024, id);

    const char *partition_label = "storage";
    ESP_LOGI(TAG, "Adding external Flash as a partition, label=\"%s\", size=%d KB", partition_label, ext_flash->size / 1024);

    ret = esp_partition_register_external(ext_flash, 0, ext_flash->size, partition_label, ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, &w25q128->partition);
     if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register external partition: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }
  
    return ESP_OK;

}


static char *w25q128_find_timezone(tz_database_t *tzdb, float latitude, float longitude)
{
    w25q128_t *w25q128 = __containerof(tzdb, w25q128_t, parent);

    u_int32_t partition_pos = 0;

    tz_database_header_t header;

    esp_partition_read(w25q128->partition, partition_pos, &header, sizeof(header));
    partition_pos += sizeof(header);
    ESP_LOGI(TAG, "Timezone Database info: Version: %d   Signature: %.4s   Precision: %d   Creation Date %.10s", 
         header.version, header.signature, header.precision, header.creation_date);

    int32_t latitude_int = float_to_int(latitude, 90, header.precision);
    int32_t longitude_int = float_to_int(longitude, 180, header.precision);
    ESP_LOGI(TAG, "Search Latitude: %f %d", latitude, latitude_int);
    ESP_LOGI(TAG, "       Longitude: %f %d", longitude, longitude_int);

    u_int32_t entries;
    esp_partition_read(w25q128->partition, partition_pos, &entries, sizeof(u_int32_t));
    partition_pos += sizeof(u_int32_t);
    ESP_LOGI(TAG, "Entries in TOC: %d", entries);

    static tz_database_entry_t entry;
    for (int i = 0; i < entries; i++) {
        esp_partition_read(w25q128->partition, partition_pos, &entry, sizeof(tz_database_entry_t));
        partition_pos += sizeof(tz_database_entry_t);
        
        //ESP_LOGI(TAG, "Name: %.64s", entry.tz_name);      
        //ESP_LOGI(TAG, "Value: %.64s", entry.tz_value);

        u_int32_t data_position = entry.position;
        bool is_inside = check_inside_country(w25q128->partition, data_position, latitude_int, longitude_int);

        if (is_inside) {
            ESP_LOGI(TAG, "Inside timezone: %.64s", entry.tz_name);
            return (char *)&entry.tz_value;
        }

    }

    return "";

}


static bool check_inside_country(const esp_partition_t* partition, u_int32_t data_position, int32_t latitude_int, int32_t longitude_int) {

    tz_database_boundingbox_t boundingbox;
    esp_partition_read(partition, data_position, &boundingbox, sizeof(tz_database_boundingbox_t));
    data_position += sizeof(tz_database_boundingbox_t);
    //ESP_LOGI(TAG, "from_latitude: %d", boundingbox.from_latitude);
    //ESP_LOGI(TAG, "from_longitude: %d", boundingbox.from_longitude);
    //ESP_LOGI(TAG, "to_latitude: %d", boundingbox.to_latitude);
    //ESP_LOGI(TAG, "to_longitude: %d", boundingbox.to_longitude);

    if (latitude_int < boundingbox.from_latitude || latitude_int > boundingbox.to_latitude ||
        longitude_int < boundingbox.from_longitude || longitude_int > boundingbox.to_longitude ) {
        
        return false;
    }
    else {
        u_int32_t shapes;
        esp_partition_read(partition, data_position, &shapes, sizeof(u_int32_t));
        data_position += sizeof(u_int32_t);
        ESP_LOGI(TAG, "Inside Bounding Box");

        tz_database_shape_t shape;
        for (int j = 0; j < shapes; j++) { 

            esp_partition_read(partition, data_position, &shape, sizeof(tz_database_shape_t));
            data_position += sizeof(tz_database_shape_t);

            u_int32_t shape_position = shape.position;

            bool is_inside = check_inside_shape(partition, shape_position, latitude_int, longitude_int);

            if (is_inside) { 
                return true; 
            }
        }  
        return false;  
    }

}

static bool check_inside_shape(const esp_partition_t* partition, u_int32_t shape_position, int32_t latitude_int, int32_t longitude_int) {
            
    tz_database_point_t start_point;
    esp_partition_read(partition, shape_position, &start_point, sizeof(tz_database_point_t));
    shape_position += sizeof(tz_database_point_t);
    //ESP_LOGI(TAG, "Start: %d %d", start_point.latitude, start_point.longitude);          

    u_int32_t deltas;
    esp_partition_read(partition, shape_position, &deltas, sizeof(u_int32_t));
    shape_position += sizeof(u_int32_t);
    //ESP_LOGI(TAG, "deltas: %d", deltas);

    tz_database_point_t p1;
    tz_database_point_t p2;

    p1.latitude = start_point.latitude;
    p1.longitude = start_point.longitude;

    p2.latitude = start_point.latitude;
    p2.longitude = start_point.longitude;

    double x_inters;
    bool odd = false;

    for (int k = 0; k < deltas; k++) {
        p2.latitude += next_value(partition, &shape_position);
        p2.longitude += next_value(partition, &shape_position);
        //ESP_LOGI(TAG, "                                  k P2: %d %f %f", k, int_to_float(p2.latitude, 90, 24), int_to_float(p2.longitude, 180, 24));
        //ESP_LOGI(TAG, "P1: %d %d   P2: %d %d", p1.latitude, p1.longitude, p2.latitude, p2.longitude);
        
        // y muss zwischen min und max der Linie sein
        if (latitude_int > MIN(p1.latitude, p2.latitude) && latitude_int <= MAX(p1.latitude, p2.latitude)) {
            // x muss kleiner gleich dem großeren x Wert der Linie sein
            if (longitude_int <= MAX(p1.longitude, p2.longitude)) {
                // Horizontale Linie wird ignoriert da schon beim Endpunkt einer anderen Linie gezählt
                if (p1.latitude != p2.latitude) {
                    // Geradengleichung nach x aufgelöst  https://www.mathematik-oberstufe.de/analysis/lin/gerade2d-2punkte.html
                    x_inters = (latitude_int-p1.latitude) * (p2.longitude-p1.longitude) / (p2.latitude-p1.latitude) + p1.longitude;
                    if (longitude_int <= x_inters) {
                        odd = !odd;
                        ESP_LOGI(TAG, "Crossed line P1: %f %f   P2: %f %f", int_to_float(p1.latitude, 90, 24), int_to_float(p1.longitude, 180, 24), int_to_float(p2.latitude, 90, 24), int_to_float(p2.longitude, 180, 24));
                    }
                }
            }
            
        }
        p1 = p2;
    }

    return odd;
}

static int32_t next_value(const esp_partition_t* partition, u_int32_t *position) {
    uint8_t marker;
    int32_t value;
     
    esp_partition_read(partition, *position, &marker, sizeof(int8_t));  // Read and test if is the marker. Don't change position          

    if (marker != 0x80 && marker != 0x7F) {  // Not the marker. Read again as 8 Bit value

        esp_partition_read(partition, *position, &value, sizeof(int8_t)); // Read again in our variable
        *position += sizeof(int8_t);

        //ESP_LOGI(TAG, "value 8 Bit: %x", value);
        value = (int8_t) value;
        //ESP_LOGI(TAG, "value 8 Bit (int8_t): %d", value);

    } 
    else if (marker == 0x80) {  // marker for 16 bit value

        *position += sizeof(int8_t); // Move to next position

        esp_partition_read(partition, *position, &value, sizeof(int16_t));
        *position += sizeof(int16_t);

        //ESP_LOGI(TAG, "value 16 Bit: %x", delta_length);
        value = (int16_t) value;
        //ESP_LOGI(TAG, "value 16 Bit (int16_t): %d", value);
    }        
    else if (marker == 0x7F) { // marker for 32 Bit value

        *position += sizeof(int8_t); // Move to next position

        esp_partition_read(partition, *position, &value, sizeof(int32_t));
        *position += sizeof(int32_t);

        value = (int32_t) value;
        //ESP_LOGI(TAG, "value 32 Bit (int32_t): %d", value);                         
    }
    else {
        ESP_LOGI(TAG, "Error");
    }

    return value;
        
}

tz_database_t *tz_database_new_w25q128(const tz_database_conf_t *config)
{
    w25q128_t *w25q128 = calloc(1, sizeof(w25q128_t));

    w25q128->config.mosi_io_num = config->mosi_io_num;
    w25q128->config.miso_io_num = config->miso_io_num;
    w25q128->config.sclk_io_num = config->sclk_io_num;
    w25q128->config.cs_io_num = config->cs_io_num;
    w25q128->config.host_id = config->host_id;
    w25q128->config.io_mode = config->io_mode;
    w25q128->config.speed = config->speed;

    w25q128->parent.init = w25q128_init;
    w25q128->parent.find_timezone = w25q128_find_timezone;

    return &w25q128->parent;

}


static int32_t float_to_int(float input, float scale, uint8_t precision)
{
    const float inputScaled = input / scale;
    return (int32_t)(inputScaled * (float)(1 << (precision - 1)));
}

static float int_to_float(int32_t input, float scale, uint8_t precision)
{
    const float value = (float)input / (float)(1 << (precision - 1));
    return value * scale;
}