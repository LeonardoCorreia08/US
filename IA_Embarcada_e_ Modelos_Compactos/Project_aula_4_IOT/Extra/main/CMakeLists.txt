idf_component_register(
    SRCS
        "main.c"
        "datacenter.c"

        # IA V1 - Datacenter        
        "preprocessamento.c"
        "mpu6050.c"
        "oled.c"
        "buzzer.c"
        "model/modelo_v1_int8.cc"        
        "tflite_runner_v1.cc"

    INCLUDE_DIRS
        "."

    REQUIRES
        driver
        esp_driver_ledc
        esp-tflite-micro
        esp_driver_i2c
        esp_driver_gpio
        esp_adc
        dht
)