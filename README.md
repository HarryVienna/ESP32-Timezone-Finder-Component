# ESP32-Timezone-Finder-Component

This is a compoment for the ESP32 to get the timezone based on GPS coordinates. 


The database can be generated with the [Timezone-Database-Generator](https://github.com/HarryVienna/Timezone-Database-Generator)

The output file is stored on a an a 128 MBit W25Q128 SpiFlash chip that is connected to the ESP32. The flashing is done with the
command 

**esptool.py --chip esp32 write_flash --flash_mode dio  --spi-connection 18,19,23,21,5 0x0000 timezones.bin**




