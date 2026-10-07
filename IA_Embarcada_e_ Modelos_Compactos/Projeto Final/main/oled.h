#ifndef OLED_H
#define OLED_H

#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa um OLED em um endereço específico e retorna o "handle" (controle) dele */
i2c_master_dev_handle_t oled_init(i2c_master_bus_handle_t bus, uint8_t endereco);

/* Limpa a tela de um OLED específico */
void oled_clear(i2c_master_dev_handle_t dev);

/* Escreve um texto em um OLED específico */
void oled_print(i2c_master_dev_handle_t dev, int x, int y, const char* texto);

#ifdef __cplusplus
}
#endif

#endif