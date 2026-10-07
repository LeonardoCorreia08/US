#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h" 
#include "driver/i2c_master.h" 
#include "model/modelo_ia.h" 
#include "oled.h" 

#define I2C_MASTER_SDA_IO 8 
#define I2C_MASTER_SCL_IO 9 

#define CANAL_ADC_PH       ADC_CHANNEL_0 
#define CANAL_ADC_TURBIDEZ ADC_CHANNEL_1 
#define CANAL_ADC_GAS      ADC_CHANNEL_2 

adc_oneshot_unit_handle_t adc1_handle;
i2c_master_bus_handle_t i2c_bus_handle; 

i2c_master_dev_handle_t tela_coleta;
i2c_master_dev_handle_t tela_resultado;

void inicializar_i2c() {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle);
}

void inicializar_hardware() {
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = ADC_UNIT_1 };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12, 
    };

    adc_oneshot_config_channel(adc1_handle, CANAL_ADC_PH, &config);
    adc_oneshot_config_channel(adc1_handle, CANAL_ADC_TURBIDEZ, &config);
    adc_oneshot_config_channel(adc1_handle, CANAL_ADC_GAS, &config);
}

void app_main(void) {
    inicializar_hardware();
    inicializar_i2c();
    
    // Inicializa as DUAS telas
    tela_coleta = oled_init(i2c_bus_handle, 0x3C);
    tela_resultado = oled_init(i2c_bus_handle, 0x3D);

    oled_clear(tela_coleta);
    oled_clear(tela_resultado);

    int64_t tempo_inicial = esp_timer_get_time();

    char txt_linha1[32]; char txt_linha2[32];
    char txt_linha3[32]; char txt_linha4[32];

    while (1) {
        int64_t tempo_atual = esp_timer_get_time();
        float leitura_dia = (float)(tempo_atual - tempo_inicial) / 10000000.0f; 
        if (leitura_dia > 15.0f) leitura_dia = 15.0f; 

        int raw_ph, raw_turb, raw_gas;
        adc_oneshot_read(adc1_handle, CANAL_ADC_PH, &raw_ph);
        adc_oneshot_read(adc1_handle, CANAL_ADC_TURBIDEZ, &raw_turb);
        adc_oneshot_read(adc1_handle, CANAL_ADC_GAS, &raw_gas);

        float leitura_ph = 5.0f + ((float)raw_ph / 4095.0f) * 3.5f; 
        float leitura_turbidez = 200.0f + ((float)raw_turb / 4095.0f) * 2800.0f;
        float leitura_co2 = 300.0f + ((float)raw_gas / 4095.0f) * 700.0f;
        
        float predicoes[4] = {0};
        prever_bioprocesso(leitura_dia, leitura_ph, 26.5f, leitura_turbidez, leitura_co2, predicoes);

        // ==========================================
        // 1. PRINT NO CONSOLE SERIAL (RESTAURADO)
        // ==========================================
        printf("\n[Dia %.1f] Sensores: pH=%.1f | Turbidez=%.0f NTU | CO2=%.0f ppm\n", 
               leitura_dia, leitura_ph, leitura_turbidez, leitura_co2);
        printf("-> Sistema de Biorremediação: DQO=%.1f | P=%.1f | N=%.1f | S=%.1f\n", 
               predicoes[0], predicoes[1], predicoes[2], predicoes[3]);

        // ==========================================
        // 2. ESCREVE NA TELA 1 (0x3C) - COLETA
        // ==========================================
        snprintf(txt_linha1, sizeof(txt_linha1), "Tempo: Dia %.1f", leitura_dia);
        snprintf(txt_linha2, sizeof(txt_linha2), "pH: %.1f", leitura_ph);
        snprintf(txt_linha3, sizeof(txt_linha3), "Turb: %.0f", leitura_turbidez);
        snprintf(txt_linha4, sizeof(txt_linha4), "CO2: %.0f", leitura_co2);

        oled_clear(tela_coleta);
        oled_print(tela_coleta, 0, 0, txt_linha1);
        oled_print(tela_coleta, 0, 2, txt_linha2);
        oled_print(tela_coleta, 0, 4, txt_linha3);
        oled_print(tela_coleta, 0, 6, txt_linha4);

        // ==========================================
        // 3. ESCREVE NA TELA 2 (0x3D) - PREDICAO DA IA
        // ==========================================
        snprintf(txt_linha1, sizeof(txt_linha1), "--- IA EDGE ---");
        snprintf(txt_linha2, sizeof(txt_linha2), "DQO: %.1f mg/L", predicoes[0]);
        snprintf(txt_linha3, sizeof(txt_linha3), "N: %.1f mg/L", predicoes[2]);
        snprintf(txt_linha4, sizeof(txt_linha4), "P: %.1f mg/L", predicoes[1]);

        oled_clear(tela_resultado);
        oled_print(tela_resultado, 0, 0, txt_linha1);
        oled_print(tela_resultado, 0, 2, txt_linha2);
        oled_print(tela_resultado, 0, 4, txt_linha3);
        oled_print(tela_resultado, 0, 6, txt_linha4);

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}