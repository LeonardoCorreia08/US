#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Inicializa o barramento I2C e o MPU6050.
 *
 * Retorno:
 *   0  = sucesso
 *   <0 = erro
 */
int mpu6050_init(void);

/*
 * Retorna o barramento I2C utilizado pelo MPU6050.
 *
 * O OLED poderá utilizar o mesmo barramento.
 */
i2c_master_bus_handle_t mpu6050_get_i2c_bus(void);

/*
 * Lê os seis valores do sensor:
 *
 * AX, AY, AZ = acelerômetro
 * GX, GY, GZ = giroscópio
 *
 * Os valores são retornados nas mesmas unidades
 * utilizadas durante a coleta do dataset.
 */
int mpu6050_read(
    float* ax,
    float* ay,
    float* az,
    float* gx,
    float* gy,
    float* gz
);

#ifdef __cplusplus
}
#endif

#endif

