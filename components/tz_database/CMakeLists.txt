set(component_srcs "src/tz_database_w25q128.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ""
                       PRIV_REQUIRES "driver fatfs spi_flash"
                       REQUIRES "")
