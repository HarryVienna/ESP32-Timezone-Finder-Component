# ESP32-Timezone-Finder-Component

This is a compoment for the ESP32 to find the timezone based on GPS coordinates entirely offline. Therefore a database file is used that is stored on a 128 MBit W25Q128 SpiFlash chip that is connected to the ESP32.  


The database file can be generated with the [Timezone-Database-Generator](https://github.com/HarryVienna/Timezone-Database-Generator)

The flashing is done with the command 

**esptool.py --chip esp32 write_flash --flash_mode dio  --spi-connection 18,19,23,21,5 0x0000 timezones.bin**


You can find a detailed description on my [website](https://www.haraldkreuzer.net/en/news/esp32-library-offline-time-zone-search-given-gps-coordinates).


