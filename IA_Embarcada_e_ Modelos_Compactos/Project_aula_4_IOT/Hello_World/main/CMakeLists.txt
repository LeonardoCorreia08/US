idf_component_register(SRCS "hello_world_main.c", "model/hello_world_int8.cc"

                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS ""
                       EMBED_FILES "model/hello_world_int8.tflite")

