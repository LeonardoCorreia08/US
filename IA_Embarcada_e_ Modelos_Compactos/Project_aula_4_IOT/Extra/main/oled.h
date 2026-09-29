
#ifndef OLED_H
#define OLED_H

#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Inicializa o OLED SSD1306.
 *
 * O OLED utiliza o mesmo barramento I2C
 * criado pelo driver do MPU6050.
 *
 * Retorno:
 *   0  = sucesso
 *   <0 = erro  */
int oled_init(
    i2c_master_bus_handle_t bus
);

/*Limpa toda a tela do OLED. */
void oled_clear(void);

/* Escreve um texto na tela.
 * x:
 *   posição horizontal em pixels.
 *
 * y:
 *   página vertical de 0 a 7. */
void oled_print(
    int x,
    int y,
    const char* texto
);

#ifdef __cplusplus
}
#endif

#endif

