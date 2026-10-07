# Pega todos os arquivos de código-fonte
file(GLOB_RECURSE sources "*.c" "*.cpp" "*.cc")

# Adiciona a pasta "model" nos diretórios de include (INCLUDE_DIRS)
idf_component_register(SRCS ${sources}
                       INCLUDE_DIRS "." "model"
                       REQUIRES esp_adc esp_timer esp_driver_i2c)