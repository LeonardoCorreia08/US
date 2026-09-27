idf_component_register(
    SRCS
        "main.c"
        "datacenter.c"

        # IA anterior - manter
        "tflite_runner.cc"
        "preprocessamento.c"
        "mpu6050.c"
        "oled.c"
        "buzzer.c"
        "model/modelo_v0_int8.cc"

        # IA V1 - Datacenter
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