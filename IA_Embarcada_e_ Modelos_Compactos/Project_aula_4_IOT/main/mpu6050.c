#include "mpu6050.h"

#include <stdio.h>

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * Endereço I2C padrão do MPU6050.
 *
 * AD0 em GND -> 0x68
 * AD0 em VCC -> 0x69
 */
#define MPU6050_ADDR 0x68

/*
 * GPIOs utilizados no ESP32-S3.
 */
#define MPU6050_SDA_GPIO 8
#define MPU6050_SCL_GPIO 9

/*
 * Frequência do barramento I2C.
 */
#define MPU6050_I2C_FREQ_HZ 400000

/*
 * Registradores do MPU6050.
 */
#define MPU6050_REG_WHO_AM_I     0x75
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

/*
 * Barramento I2C compartilhado.
 *
 * O MPU6050 cria o barramento.
 * O OLED utilizará o mesmo barramento.
 */
static i2c_master_bus_handle_t i2c_bus = NULL;

/*
 * Dispositivo MPU6050 conectado ao barramento.
 */
static i2c_master_dev_handle_t mpu6050_dev = NULL;

/*
 * Escreve um registrador do MPU6050.
 */
static esp_err_t mpu6050_write_register(
    uint8_t reg,
    uint8_t value
)
{
    uint8_t dados[2] = {
        reg,
        value
    };

    return i2c_master_transmit(
        mpu6050_dev,
        dados,
        sizeof(dados),
        -1
    );
}

/*
 * Lê vários bytes a partir de um registrador.
 */
static esp_err_t mpu6050_read_registers(
    uint8_t reg,
    uint8_t* dados,
    size_t tamanho
)
{
    return i2c_master_transmit_receive(
        mpu6050_dev,
        &reg,
        1,
        dados,
        tamanho,
        -1
    );
}

int mpu6050_init(void)
{
    /*
     * Configura o barramento I2C.
     */
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = MPU6050_SDA_GPIO,
        .scl_io_num = MPU6050_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    esp_err_t erro = i2c_new_master_bus(
        &bus_config,
        &i2c_bus
    );

    if (erro != ESP_OK)
    {
        printf(
            "ERRO: falha ao criar barramento I2C: %s\n",
            esp_err_to_name(erro)
        );

        return -1;
    }

    /*
     * Adiciona o MPU6050 ao barramento.
     */
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = MPU6050_I2C_FREQ_HZ
    };

    erro = i2c_master_bus_add_device(
        i2c_bus,
        &dev_config,
        &mpu6050_dev
    );

    if (erro != ESP_OK)
    {
        printf(
            "ERRO: falha ao adicionar MPU6050: %s\n",
            esp_err_to_name(erro)
        );

        return -2;
    }

    /*
     * Verifica o endereço WHO_AM_I.
     */
    uint8_t who_am_i = 0;

    erro = mpu6050_read_registers(
        MPU6050_REG_WHO_AM_I,
        &who_am_i,
        1
    );

    if (erro != ESP_OK)
    {
        printf(
            "ERRO: MPU6050 nao respondeu no I2C.\n"
        );

        return -3;
    }

    printf(
        "MPU6050 WHO_AM_I: 0x%02X\n",
        who_am_i
    );

    /*
     * Retira o MPU6050 do modo sleep.
     */
    erro = mpu6050_write_register(
        MPU6050_REG_PWR_MGMT_1,
        0x00
    );

    if (erro != ESP_OK)
    {
        printf(
            "ERRO: nao foi possivel inicializar o MPU6050.\n"
        );

        return -4;
    }

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );

    printf(
        "MPU6050 inicializado com sucesso!\n"
    );

    return 0;
}

/*
 * Retorna o barramento I2C utilizado pelo MPU6050.
 *
 * O OLED utilizará o mesmo barramento I2C.
 */
i2c_master_bus_handle_t mpu6050_get_i2c_bus(void)
{
    return i2c_bus;
}

int mpu6050_read(
    float* ax,
    float* ay,
    float* az,
    float* gx,
    float* gy,
    float* gz
)
{
    if (
        ax == NULL ||
        ay == NULL ||
        az == NULL ||
        gx == NULL ||
        gy == NULL ||
        gz == NULL
    )
    {
        return -1;
    }

    /*
     * O MPU6050 fornece 14 bytes consecutivos:
     *
     * AXH AXL
     * AYH AYL
     * AZH AZL
     * TEMP_H TEMP_L
     * GXH GXL
     * GYH GYL
     * GZH GZL
     */
    uint8_t dados[14];

    esp_err_t erro = mpu6050_read_registers(
        MPU6050_REG_ACCEL_XOUT_H,
        dados,
        sizeof(dados)
    );

    if (erro != ESP_OK)
    {
        return -2;
    }

    int16_t raw_ax =
        (int16_t)((dados[0] << 8) | dados[1]);

    int16_t raw_ay =
        (int16_t)((dados[2] << 8) | dados[3]);

    int16_t raw_az =
        (int16_t)((dados[4] << 8) | dados[5]);

    int16_t raw_gx =
        (int16_t)((dados[8] << 8) | dados[9]);

    int16_t raw_gy =
        (int16_t)((dados[10] << 8) | dados[11]);

    int16_t raw_gz =
        (int16_t)((dados[12] << 8) | dados[13]);

    /*
     * Configuração padrão:
     *
     * Acelerômetro: ±2g
     * 16384 LSB/g
     *
     * Giroscópio: ±250 °/s
     * 131 LSB/(°/s)
     */
    *ax = (float)raw_ax / 16384.0f;
    *ay = (float)raw_ay / 16384.0f;
    *az = (float)raw_az / 16384.0f;

    *gx = (float)raw_gx / 131.0f;
    *gy = (float)raw_gy / 131.0f;
    *gz = (float)raw_gz / 131.0f;

    return 0;
}
