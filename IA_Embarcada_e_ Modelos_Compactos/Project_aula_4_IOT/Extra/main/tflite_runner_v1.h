#ifndef TFLITE_RUNNER_V1_H
#define TFLITE_RUNNER_V1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    IA_V1_NORMAL = 0,
    IA_V1_ATENCAO = 1,
    IA_V1_CRITICO = 2
} ia_v1_status_t;

typedef struct
{
    ia_v1_status_t classe;

    float probabilidade_normal;
    float probabilidade_atencao;
    float probabilidade_critico;
} ia_v1_resultado_t;

/**
 * Inicializa o modelo de IA V1.
 */
bool ia_v1_init(void);

/**
 * Executa uma inferência.
 *
 * Entradas:
 *  - temperatura ambiente
 *  - umidade ambiente
 *  - temperatura rack
 *  - umidade rack
 *  - gas ADC
 *  - energia (1 = presente / 0 = sem energia)
 */
bool ia_v1_predict(
    float temperatura_ambiente,
    float umidade_ambiente,
    float temperatura_rack,
    float umidade_rack,
    float gas,
    float energia,
    ia_v1_resultado_t *resultado
);

/**
 * Retorna o nome da classe.
 */
const char *ia_v1_status_string(
    ia_v1_status_t status
);

#ifdef __cplusplus
}
#endif

#endif