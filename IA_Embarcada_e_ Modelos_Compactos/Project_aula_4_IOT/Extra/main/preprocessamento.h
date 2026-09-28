#ifndef PREPROCESSAMENTO_H
#define PREPROCESSAMENTO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Converte os 6 valores do MPU6050:
 *
 * AX, AY, AZ, GX, GY, GZ
 *
 * para o formato INT8 utilizado pelo modelo.
 */
void preprocessar_dados(
    const float dados[6],
    int8_t resultado[6]
);

#ifdef __cplusplus
}
#endif

#endif

