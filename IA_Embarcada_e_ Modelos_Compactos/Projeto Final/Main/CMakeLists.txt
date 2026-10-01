idf_component_register(SRCS "hello_world_main.c" "tflite_runner_v1.cc" "model/hello_world_int8.cc"
                       REQUIRES esp-tflite-micro
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "."
                       EMBED_FILES "model/hello_world_int8.tflite")