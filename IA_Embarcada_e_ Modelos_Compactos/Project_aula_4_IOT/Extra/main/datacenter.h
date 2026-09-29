#ifndef DATACENTER_H
#define DATACENTER_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "tflite_runner_v1.h"


/*  * GPIOs  ESP32-S3 DevKitC-1  */

/* DHT22 */
#define DC_DHT_AMBIENTE_GPIO       4
#define DC_DHT_EQUIPAMENTOS_GPIO   16


/* I2C / SDA = GPIO8 / SCL = GPIO9 */

#define DC_I2C_SDA_GPIO            8
#define DC_I2C_SCL_GPIO            9


/*  SENSOR DE GAS / ESP32-S3: / ADC1_CH6 = GPIO7 */

#define DC_GAS_GPIO                7


/* SAIDAS  */

#define DC_BUZZER_GPIO             14

#define DC_LED_VERMELHO_GPIO       10
#define DC_LED_AMARELO_GPIO        11
#define DC_LED_VERDE_GPIO          12


/*  BOTAO DE ENERGIA  */

#define DC_ENERGIA_GPIO            17


/*  ENDERECOS I2C */

#define DC_OLED_1_ADDR             0x3C
#define DC_OLED_2_ADDR             0x3D
#define DC_LCD_ADDR                0x27


/* LIMITES AMBIENTAIS */

#define DC_TEMP_BAIXA              17.0f
#define DC_TEMP_ATENCAO            22.0f
#define DC_TEMP_CRITICA            24.0f


/* 30 minutos */
#define DC_TEMPO_CRITICO_SEG       1800


/*  LIMITES DO SENSOR DE GAS */

#define DC_GAS_ATENCAO             1800
#define DC_GAS_CRITICO             2800


/* ============================================================
 * MODO DE CONTROLE
 *
 * 0 = sensores reais / regras tradicionais
 * 1 = simulacao automatica / IA 
 * ============================================================ */

#define DC_MODO_MANUAL             0
#define DC_MODO_AUTOMATICO         1


/* ============================================================
 * CENARIOS DO MODO AUTOMATICO
 *
 * O modo automatico percorre diferentes situacoes para
 * demonstrar a analise da IA V1.
 * ============================================================ */

#define DC_CENARIO_NORMAL          0
#define DC_CENARIO_ATENCAO         1
#define DC_CENARIO_CRITICO         2

#define DC_TOTAL_CENARIOS          3


/*  ESTADOS */

typedef enum
{
    DC_STATUS_NORMAL = 0,

    DC_STATUS_ATENCAO,

    DC_STATUS_CRITICO,

    DC_STATUS_SEM_ENERGIA,

    DC_STATUS_GAS

} dc_status_t;


/*  DADOS DOS SENSORES  */

typedef struct
{
    float temperatura_ambiente;

    float umidade_ambiente;

    float temperatura_equipamentos;

    float umidade_equipamentos;

    int gas;

} dc_sensores_t;


/*  HISTORICO AMBIENTAL  */

typedef struct
{
    float temperatura_minima;

    float temperatura_maxima;

    float temperatura_pico;

    float diferenca_termica_maxima;

    uint32_t quantidade_picos_22;

    uint32_t quantidade_picos_24;

    uint32_t tempo_acima_22;

    uint32_t tempo_acima_24;

    uint32_t tempo_continuo_acima_22;

    uint32_t tempo_continuo_acima_24;

} dc_historico_t;


/*   ENERGIA */

typedef struct
{
    bool presente;

    uint32_t quantidade_quedas;

    uint32_t duracao_queda_atual;

    uint32_t ultima_duracao_queda;

} dc_energia_t;


/*  ESTADO GERAL */

typedef struct
{
    dc_status_t status;

    bool temperatura_alta;

    bool temperatura_critica;

    bool temperatura_baixa;

    bool gas_atencao;

    bool gas_critico;

    bool energia_presente;

    bool climatizacao_suspeita;

    bool alarme_ativo;

} dc_estado_t;


/*  INTELIGENCIA ARTIFICIAL   */

typedef struct
{
    ia_v1_status_t classe;

    float probabilidade_normal;

    float probabilidade_atencao;

    float probabilidade_critico;

    bool disponivel;

} dc_ia_t;


/* ESTRUTURA PRINCIPAL */

typedef struct
{
    dc_sensores_t sensores;

    dc_historico_t historico;

    dc_energia_t energia;

    dc_estado_t estado;

    dc_ia_t ia;

    /* 0 = manual
     * 1 = automatico
     */
    uint8_t modo_controle;

    /* Cenario atual da simulacao automatica */
    uint32_t cenario_automatico;

    uint32_t tempo_funcionamento;

    uint32_t pagina_lcd;

} datacenter_t;


/*  INICIALIZACAO  */

esp_err_t datacenter_init(
    datacenter_t *dc
);


/*  ATUALIZACAO */

esp_err_t datacenter_atualizar(
    datacenter_t *dc
);


/*  DISPLAYS */

void datacenter_oled_atual(
    datacenter_t *dc
);


void datacenter_oled_analise(
    datacenter_t *dc
);


void datacenter_lcd_atualizar(
    datacenter_t *dc
);


/*  LEDS E BUZZER */

void datacenter_atualizar_leds(
    datacenter_t *dc
);


void datacenter_atualizar_alarme(
    datacenter_t *dc
);


/* INFORMACOES */

const char *datacenter_status_string(
    dc_status_t status
);


#endif