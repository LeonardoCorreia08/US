#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"

#include "esp_timer.h"
#include "esp_rom_sys.h"

// ============================================================
// PINOS
// ============================================================

#define BUZZER_PIN  14
#define DHT_PIN     15

// ============================================================
// I2C OLED
// ============================================================

#define I2C_MASTER_SCL_IO     9
#define I2C_MASTER_SDA_IO     8
#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_FREQ_HZ    400000

#define OLED_ADDR             0x3C

// ============================================================
// LIMITES DE TEMPERATURA
// ============================================================

#define TEMP_MUITO_BAIXA      10.0
#define TEMP_BAIXA            29.0
#define TEMP_NORMAL_MAX       35.0
#define TEMP_MUITO_ALTA       36.0

// ============================================================
// FONTE 5x8
// ============================================================

const uint8_t font5x8[96][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5f, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, {0x24, 0x2a, 0x7f, 0x2a, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00}, {0x00, 0x1c, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1c, 0x00}, {0x08, 0x2a, 0x1c, 0x2a, 0x08}, {0x08, 0x08, 0x3e, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02}, {0x3e, 0x51, 0x49, 0x45, 0x3e}, {0x00, 0x42, 0x7f, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4b, 0x31}, {0x18, 0x14, 0x12, 0x7f, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3c, 0x4a, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1e}, {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00}, {0x00, 0x08, 0x14, 0x22, 0x41}, {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x41, 0x22, 0x14, 0x08, 0x00}, {0x02, 0x01, 0x51, 0x09, 0x06}, {0x32, 0x49, 0x79, 0x41, 0x3e},
    {0x7e, 0x11, 0x11, 0x11, 0x7e}, {0x7f, 0x49, 0x49, 0x49, 0x36}, {0x3e, 0x41, 0x41, 0x41, 0x22},
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, {0x7f, 0x49, 0x49, 0x49, 0x41}, {0x7f, 0x09, 0x09, 0x01, 0x01},
    {0x3e, 0x41, 0x41, 0x51, 0x32}, {0x7f, 0x08, 0x08, 0x08, 0x7f}, {0x00, 0x41, 0x7f, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3f, 0x01}, {0x7f, 0x08, 0x14, 0x22, 0x41}, {0x7f, 0x40, 0x40, 0x40, 0x40},
    {0x7f, 0x02, 0x04, 0x02, 0x7f}, {0x7f, 0x04, 0x08, 0x10, 0x7f}, {0x3e, 0x41, 0x41, 0x41, 0x3e},
    {0x7f, 0x09, 0x09, 0x09, 0x06}, {0x3e, 0x41, 0x51, 0x21, 0x5e}, {0x7f, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7f, 0x01, 0x01}, {0x3f, 0x40, 0x40, 0x40, 0x3f},
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, {0x3f, 0x40, 0x38, 0x40, 0x3f}, {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x7f, 0x41, 0x41, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x7f, 0x00}, {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40}, {0x00, 0x01, 0x02, 0x04, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7f, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, {0x38, 0x44, 0x44, 0x48, 0x7f},
    {0x38, 0x54, 0x54, 0x54, 0x18}, {0x08, 0x7e, 0x09, 0x01, 0x02}, {0x08, 0x14, 0x54, 0x54, 0x3c},
    {0x7f, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7d, 0x40, 0x00}, {0x20, 0x40, 0x44, 0x3d, 0x00},
    {0x7f, 0x10, 0x28, 0x44, 0x00}, {0x00, 0x41, 0x7f, 0x40, 0x00}, {0x7c, 0x04, 0x18, 0x04, 0x78},
    {0x7c, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38}, {0x7c, 0x14, 0x14, 0x14, 0x08},
    {0x08, 0x14, 0x14, 0x18, 0x7c}, {0x7c, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3f, 0x44, 0x40, 0x20}, {0x3c, 0x40, 0x40, 0x20, 0x7c}, {0x1c, 0x20, 0x40, 0x20, 0x1c},
    {0x3c, 0x40, 0x30, 0x40, 0x3c}, {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0c, 0x50, 0x50, 0x50, 0x3c},
    {0x44, 0x64, 0x54, 0x4c, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00}, {0x00, 0x00, 0x7f, 0x00, 0x00},
    {0x00, 0x41, 0x36, 0x08, 0x00}, {0x08, 0x08, 0x2a, 0x1c, 0x08}, {0x08, 0x14, 0x22, 0x41, 0x00}
};

// ============================================================
// OLED
// ============================================================

void oled_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    uint8_t init_cmds[] = {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00,
        0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA,
        0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4,
        0xA6, 0xAF
    };

    i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, init_cmds, sizeof(init_cmds), pdMS_TO_TICKS(1000));
}

void oled_clear(void)
{
    uint8_t cmd[] = {0x00, 0x21, 0, 127, 0x22, 0, 7};
    i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, cmd, sizeof(cmd), pdMS_TO_TICKS(100));

    uint8_t data[129];
    data[0] = 0x40;
    memset(&data[1], 0x00, 128);

    for (int page = 0; page < 8; page++) {
        i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, data, sizeof(data), pdMS_TO_TICKS(100));
    }
}

void oled_print(int x, int y, const char *text) {
    uint8_t cmd[] = {0x00, 0xB0 + y, 0x00 + (x & 0x0F), 0x10 + ((x >> 4) & 0x0F)};
    i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, cmd, sizeof(cmd), 100);
    
    uint8_t data[129] = {0x40}; 
    int len = strlen(text);
    int idx = 1;
    
    for (int i = 0; i < len; i++) {
        int char_idx = text[i] - 32;
        if (char_idx >= 0 && char_idx < 96) {
            for(int j=0; j<5; j++) {
                data[idx++] = font5x8[char_idx][j];
            }
            data[idx++] = 0x00;
        }
    }
    i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, data, idx, 100);
}

// ============================================================
// DHT22
// ============================================================

int ler_dht22(float *temperatura, float *umidade)
{
    uint8_t data[5] = {0};

    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(DHT_PIN, 1);
    esp_rom_delay_us(40);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    int timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1) {
        if (++timeout > 100) return -1;
        esp_rom_delay_us(1);
    }

    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 0) {
        if (++timeout > 100) return -1;
        esp_rom_delay_us(1);
    }

    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1) {
        if (++timeout > 100) return -1;
        esp_rom_delay_us(1);
    }

    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level(DHT_PIN) == 0) {
            if (++timeout > 100) return -1;
            esp_rom_delay_us(1);
        }

        int64_t inicio = esp_timer_get_time();
        timeout = 0;

        while (gpio_get_level(DHT_PIN) == 1) {
            if (++timeout > 100) return -1;
            esp_rom_delay_us(1);
        }

        int64_t duracao = esp_timer_get_time() - inicio;
        if (duracao > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (checksum != data[4]) {
        return -1;
    }

    *umidade = ((data[0] << 8) | data[1]) / 10.0f;
    *temperatura = (((data[2] & 0x7F) << 8) | data[3]) / 10.0f;

    if (data[2] & 0x80) {
        *temperatura *= -1;
    }

    return 0;
}

// ============================================================
// BUZZER
// ============================================================

void buzzer_on(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4000);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void buzzer_off(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// ============================================================
// CONTROLE DO BUZZER E ALERTAS
// ============================================================

void controlar_indicadores(float temperatura)
{
    if (temperatura < TEMP_MUITO_BAIXA) {
        buzzer_on();
        printf(" -> ALERTA NO TERMINAL: TEMPERATURA MUITO BAIXA! (BUZZER LIGADO)\n");
    }
    else if (temperatura < TEMP_BAIXA) {
        buzzer_off();
    }
    else if (temperatura <= TEMP_NORMAL_MAX) {
        buzzer_off();
    }
    else if (temperatura <= TEMP_MUITO_ALTA) {
        buzzer_off();
    }
    else {
        buzzer_on();
        printf(" -> ALERTA NO TERMINAL: TEMPERATURA MUITO ALTA! (BUZZER LIGADO)\n");
    }
}

// ============================================================
// PROGRAMA PRINCIPAL
// ============================================================

void app_main(void)
{
    // ========================================================
    // CONFIGURAÇÃO DO BUZZER
    // ========================================================

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 1500, // Frequência ajustada para um som nítido
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BUZZER_PIN,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
    buzzer_off();

    // ========================================================
    // INICIALIZAÇÃO DO OLED
    // ========================================================

    oled_init();
    oled_clear();

    // ========================================================
    // VARIÁVEIS DO SENSOR
    // ========================================================
    
    float temperatura = 0.0f;
    float umidade = 0.0f;
    //char buffer_temp[32];
    //char buffer_hum[32];
    int temp_subindo = 1;     // 1 = subindo, 0 = descendo
    int umi_subindo = 1;      // 1 = subindo, 0 = descendo

    // ========================================================
    // LOOP PRINCIPAL
    // ========================================================

    while (1) {
        printf("\n================================\n");
        printf("Modo Auto-Piloto Ativado...\n");

        // 1. Faz a temperatura subir e descer automaticamente
        if (temp_subindo == 1) {
            temperatura += 3.0f; 
            if (temperatura >= 40.0f) temp_subindo = 0; 
        } else {
            temperatura -= 3.0f; 
            if (temperatura <= 5.0f) temp_subindo = 1;  
        }
        // 2. Faz a umidade subir e descer (entre 40 e 90)
        if (umi_subindo == 1) {
            umidade += 4.0f; 
            if (umidade >= 90.0f) umi_subindo = 0; 
        } else {
            umidade -= 4.0f; 
            if (umidade <= 40.0f) umi_subindo = 1;  
        }

        // 3. Imprime os valores no Terminal
        printf("Temperatura simulada: %.1f C\n", temperatura);
        printf("Humidade simulada: %.1f %%\n", umidade);

        // 4. Controla o Buzzer e imprime alertas no Terminal
        controlar_indicadores(temperatura);

        // 4. Atualiza o Ecrã OLED
        oled_clear();
        oled_print(0, 0, "Unisenai");
        oled_print(0, 2, "Leonardo Correia");

        char buffer_temp[32];
        char buffer_hum[32];
        sprintf(buffer_temp, "Temperatura: %.1f C", temperatura);
        sprintf(buffer_hum, "Humidade: %.1f %%", umidade);

        oled_print(0, 4, buffer_temp);
        oled_print(0, 5, buffer_hum);

        // 5. Imprime o Estado de Temperatura no OLED
        if (temperatura < 10.0) {
            oled_print(0, 7, "ALERTA: FRIO");
        } else if (temperatura < 29.0) {
            oled_print(0, 7, "Status: BAIXA");
        } else if (temperatura <= 35.0) {
            oled_print(0, 7, "Status: NORMAL");
        } else if (temperatura <= 36.0) {
            oled_print(0, 7, "Status: ALTA");
        } else {
            oled_print(0, 7, "ALERTA: CALOR");
        }

        // Aguarda 1 segundo
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}