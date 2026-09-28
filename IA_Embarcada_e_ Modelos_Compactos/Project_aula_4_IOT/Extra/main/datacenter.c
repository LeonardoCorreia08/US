#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "esp_system.h"
#include "esp_random.h"

#include "dht.h"

#include "datacenter.h"

static const char *datacenter_cenario_string(uint32_t cenario);

/* =========================================================
 * TAG
 * ========================================================= */

static const char *TAG = "DATACENTER";


/* =========================================================
 * ADC - SENSOR DE GAS
 *
 * ESP32-S3
 * GPIO7 = ADC1_CH6
 *
 * IMPORTANTE:
 * O valor lido aqui e ADC bruto.
 * Nao representa ppm real.
 * ========================================================= */

#define DC_GAS_ADC_UNIT       ADC_UNIT_1
#define DC_GAS_ADC_CHANNEL    ADC_CHANNEL_6
#define DC_GAS_ADC_ATTEN      ADC_ATTEN_DB_12


/* =========================================================
 * LCD2004 - PCF8574
 *
 * P0 = RS
 * P1 = RW
 * P2 = EN
 * P3 = BACKLIGHT
 * P4 = D4
 * P5 = D5
 * P6 = D6
 * P7 = D7
 * ========================================================= */

#define LCD_RS             0x01
#define LCD_RW             0x02
#define LCD_EN             0x04
#define LCD_BACKLIGHT      0x08

#define LCD_I2C_SPEED_HZ   100000


/* =========================================================
 * HANDLES I2C
 * ========================================================= */

static i2c_master_bus_handle_t i2c_bus = NULL;

static i2c_master_dev_handle_t oled1_handle = NULL;
static i2c_master_dev_handle_t oled2_handle = NULL;
static i2c_master_dev_handle_t lcd_handle = NULL;


/* =========================================================
 * ADC
 * ========================================================= */

static adc_oneshot_unit_handle_t gas_adc_handle = NULL;


/* =========================================================
 * ESTADO INTERNO
 * ========================================================= */

static bool energia_anterior = true;

static bool acima_22_anterior = false;
static bool acima_24_anterior = false;

static uint32_t ultimo_update = 0;

static uint32_t pagina_lcd = 0;


/* =========================================================
 * FONTE 5x7
 * ========================================================= */

static const uint8_t font5x7[][5] =
{
    {0x00,0x00,0x00,0x00,0x00},

    {0x3E,0x51,0x49,0x45,0x3E},
    {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},
    {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30},
    {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},
    {0x06,0x49,0x49,0x29,0x1E},

    {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},
    {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F},
    {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},
    {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01},
    {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},
    {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43}
};


/* =========================================================
 * I2C
 * ========================================================= */

static esp_err_t datacenter_i2c_init(void)
{
    esp_err_t erro;

    i2c_master_bus_config_t bus_config =
    {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = DC_I2C_SDA_GPIO,
        .scl_io_num = DC_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags =
        {
            .enable_internal_pullup = true
        }
    };

    erro = i2c_new_master_bus(
        &bus_config,
        &i2c_bus
    );

    if (erro != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Erro ao criar barramento I2C: %s",
            esp_err_to_name(erro)
        );

        return erro;
    }


    i2c_device_config_t oled1_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DC_OLED_1_ADDR,
        .scl_speed_hz = 400000
    };

    erro = i2c_master_bus_add_device(
        i2c_bus,
        &oled1_config,
        &oled1_handle
    );

    if (erro != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Erro OLED 1: %s",
            esp_err_to_name(erro)
        );

        return erro;
    }


    i2c_device_config_t oled2_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DC_OLED_2_ADDR,
        .scl_speed_hz = 400000
    };

    erro = i2c_master_bus_add_device(
        i2c_bus,
        &oled2_config,
        &oled2_handle
    );

    if (erro != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Erro OLED 2: %s",
            esp_err_to_name(erro)
        );

        return erro;
    }


    i2c_device_config_t lcd_config =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DC_LCD_ADDR,
        .scl_speed_hz = LCD_I2C_SPEED_HZ
    };

    erro = i2c_master_bus_add_device(
        i2c_bus,
        &lcd_config,
        &lcd_handle
    );

    if (erro != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Erro LCD: %s",
            esp_err_to_name(erro)
        );

        return erro;
    }


    ESP_LOGI(
        TAG,
        "I2C inicializado"
    );

    return ESP_OK;
}


/* =========================================================
 * LCD
 * ========================================================= */

static esp_err_t lcd_pcf8574_write(
    uint8_t valor
)
{
    return i2c_master_transmit(
        lcd_handle,
        &valor,
        1,
        100
    );
}


static esp_err_t lcd_pulse_enable(
    uint8_t valor
)
{
    esp_err_t erro;

    erro = lcd_pcf8574_write(
        valor | LCD_EN
    );

    if (erro != ESP_OK)
    {
        return erro;
    }

    esp_rom_delay_us(2);

    erro = lcd_pcf8574_write(
        valor & ~LCD_EN
    );

    if (erro != ESP_OK)
    {
        return erro;
    }

    esp_rom_delay_us(50);

    return ESP_OK;
}


static esp_err_t lcd_write_nibble(
    uint8_t nibble,
    bool dados
)
{
    uint8_t valor;

    valor = nibble & 0xF0;

    if (dados)
    {
        valor |= LCD_RS;
    }

    valor &= ~LCD_RW;
    valor |= LCD_BACKLIGHT;

    return lcd_pulse_enable(valor);
}


static esp_err_t lcd_write_byte(
    uint8_t valor,
    bool dados
)
{
    esp_err_t erro;

    erro = lcd_write_nibble(
        valor & 0xF0,
        dados
    );

    if (erro != ESP_OK)
    {
        return erro;
    }

    erro = lcd_write_nibble(
        (valor << 4) & 0xF0,
        dados
    );

    if (erro != ESP_OK)
    {
        return erro;
    }

    esp_rom_delay_us(50);

    return ESP_OK;
}


static esp_err_t lcd_comando(
    uint8_t comando
)
{
    esp_err_t erro;

    erro = lcd_write_byte(
        comando,
        false
    );

    if (erro != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Erro comando LCD 0x%02X: %s",
            comando,
            esp_err_to_name(erro)
        );

        return erro;
    }

    if (
        comando == 0x01 ||
        comando == 0x02
    )
    {
        vTaskDelay(
            pdMS_TO_TICKS(3)
        );
    }

    return ESP_OK;
}


static esp_err_t lcd_dado(
    uint8_t dado
)
{
    return lcd_write_byte(
        dado,
        true
    );
}


static void lcd_texto(
    const char *texto
)
{
    while (*texto)
    {
        if (
            lcd_dado(
                (uint8_t)*texto
            ) != ESP_OK
        )
        {
            return;
        }

        texto++;
    }
}


static void lcd_posicao(
    uint8_t coluna,
    uint8_t linha
)
{
    static const uint8_t offsets[4] =
    {
        0x00,
        0x40,
        0x14,
        0x54
    };

    if (linha > 3)
    {
        linha = 0;
    }

    if (coluna > 19)
    {
        coluna = 19;
    }

    lcd_comando(
        0x80 |
        (offsets[linha] + coluna)
    );
}


static esp_err_t lcd_init(void)
{
    esp_err_t erro;

    vTaskDelay(
        pdMS_TO_TICKS(100)
    );

    erro = lcd_pcf8574_write(
        LCD_BACKLIGHT
    );

    if (erro != ESP_OK)
    {
        return erro;
    }

    vTaskDelay(
        pdMS_TO_TICKS(20)
    );


    erro = lcd_write_nibble(
        0x30,
        false
    );

    if (erro != ESP_OK)
    {
        return erro;
    }

    vTaskDelay(
        pdMS_TO_TICKS(5)
    );


    erro = lcd_write_nibble(
        0x30,
        false
    );

    if (erro != ESP_OK)
    {
        return erro;
    }

    vTaskDelay(
        pdMS_TO_TICKS(5)
    );


    erro = lcd_write_nibble(
        0x30,
        false
    );

    if (erro != ESP_OK)
    {
        return erro;
    }

    vTaskDelay(
        pdMS_TO_TICKS(1)
    );


    erro = lcd_write_nibble(
        0x20,
        false
    );

    if (erro != ESP_OK)
    {
        return erro;
    }

    vTaskDelay(
        pdMS_TO_TICKS(5)
    );


    erro = lcd_comando(0x28);

    if (erro != ESP_OK)
    {
        return erro;
    }


    erro = lcd_comando(0x08);

    if (erro != ESP_OK)
    {
        return erro;
    }


    erro = lcd_comando(0x01);

    if (erro != ESP_OK)
    {
        return erro;
    }

    vTaskDelay(
        pdMS_TO_TICKS(3)
    );


    erro = lcd_comando(0x06);

    if (erro != ESP_OK)
    {
        return erro;
    }


    erro = lcd_comando(0x0C);

    if (erro != ESP_OK)
    {
        return erro;
    }


    ESP_LOGI(
        TAG,
        "LCD 20x4 inicializado"
    );

    return ESP_OK;
}


static void lcd_linha(
    uint8_t linha,
    const char *texto
)
{
    char buffer[21];

    memset(
        buffer,
        ' ',
        20
    );

    buffer[20] = '\0';

    strncpy(
        buffer,
        texto,
        20
    );

    lcd_posicao(
        0,
        linha
    );

    lcd_texto(
        buffer
    );
}


/* =========================================================
 * OLED - COMUNICACAO
 * ========================================================= */

static void oled_write_command(
    i2c_master_dev_handle_t handle,
    uint8_t command
)
{
    uint8_t buffer[2];

    buffer[0] = 0x00;
    buffer[1] = command;

    i2c_master_transmit(
        handle,
        buffer,
        2,
        100
    );
}


static void oled_write_data(
    i2c_master_dev_handle_t handle,
    const uint8_t *data,
    size_t tamanho
)
{
    if (tamanho > 128)
    {
        tamanho = 128;
    }

    uint8_t buffer[129];

    buffer[0] = 0x40;

    memcpy(
        &buffer[1],
        data,
        tamanho
    );

    i2c_master_transmit(
        handle,
        buffer,
        tamanho + 1,
        100
    );
}


/* =========================================================
 * OLED - INICIALIZACAO
 * ========================================================= */

static void oled_init(
    i2c_master_dev_handle_t handle
)
{
    const uint8_t comandos[] =
    {
        0xAE,
        0xD5, 0x80,
        0xA8, 0x3F,
        0xD3, 0x00,
        0x40,
        0x8D, 0x14,
        0x20, 0x00,
        0xA1,
        0xC8,
        0xDA, 0x12,
        0x81, 0xCF,
        0xD9, 0xF1,
        0xDB, 0x40,
        0xA4,
        0xA6,
        0xAF
    };

    for (
        size_t i = 0;
        i < sizeof(comandos);
        i++
    )
    {
        oled_write_command(
            handle,
            comandos[i]
        );
    }
}


/* =========================================================
 * OLED - LIMPAR
 * ========================================================= */

static void oled_limpar(
    i2c_master_dev_handle_t handle
)
{
    uint8_t linha[128];

    memset(
        linha,
        0x00,
        sizeof(linha)
    );

    for (int pagina = 0; pagina < 8; pagina++)
    {
        oled_write_command(
            handle,
            0xB0 | pagina
        );

        oled_write_command(
            handle,
            0x00
        );

        oled_write_command(
            handle,
            0x10
        );

        oled_write_data(
            handle,
            linha,
            sizeof(linha)
        );
    }
}


/* =========================================================
 * OLED - POSICAO
 * ========================================================= */

static void oled_posicao(
    i2c_master_dev_handle_t handle,
    uint8_t x,
    uint8_t pagina
)
{
    oled_write_command(
        handle,
        0xB0 | pagina
    );

    oled_write_command(
        handle,
        0x00 | (x & 0x0F)
    );

    oled_write_command(
        handle,
        0x10 | ((x >> 4) & 0x0F)
    );
}


/* =========================================================
 * OLED - CARACTERE
 * ========================================================= */

static void oled_caractere(
    i2c_master_dev_handle_t handle,
    char c
)
{
    uint8_t glyph[6];

    memset(
        glyph,
        0x00,
        sizeof(glyph)
    );


    if (c >= '0' && c <= '9')
    {
        memcpy(
            glyph,
            font5x7[c - '0' + 1],
            5
        );
    }

    else if (c >= 'A' && c <= 'Z')
    {
        memcpy(
            glyph,
            font5x7[11 + (c - 'A')],
            5
        );
    }

    else if (c == ' ')
    {
        memset(
            glyph,
            0x00,
            5
        );
    }

    else
    {
        if (c == ':')
        {
            glyph[0] = 0x00;
            glyph[1] = 0x36;
            glyph[2] = 0x36;
            glyph[3] = 0x00;
            glyph[4] = 0x00;
        }

        else if (c == '.')
        {
            glyph[0] = 0x00;
            glyph[1] = 0x40;
            glyph[2] = 0x60;
            glyph[3] = 0x00;
            glyph[4] = 0x00;
        }

        else if (c == '%')
        {
            glyph[0] = 0x62;
            glyph[1] = 0x64;
            glyph[2] = 0x08;
            glyph[3] = 0x13;
            glyph[4] = 0x23;
        }

        else if (c == '>')
        {
            glyph[0] = 0x08;
            glyph[1] = 0x14;
            glyph[2] = 0x22;
            glyph[3] = 0x41;
            glyph[4] = 0x00;
        }

        else if (c == '-')
        {
            glyph[0] = 0x08;
            glyph[1] = 0x08;
            glyph[2] = 0x08;
            glyph[3] = 0x08;
            glyph[4] = 0x08;
        }

        else
        {
            glyph[0] = 0x7F;
            glyph[1] = 0x41;
            glyph[2] = 0x41;
            glyph[3] = 0x41;
            glyph[4] = 0x7F;
        }
    }


    oled_write_data(
        handle,
        glyph,
        6
    );
}


/* =========================================================
 * OLED - TEXTO
 * ========================================================= */

static void oled_texto(
    i2c_master_dev_handle_t handle,
    uint8_t x,
    uint8_t pagina,
    const char *texto
)
{
    oled_posicao(
        handle,
        x,
        pagina
    );

    while (*texto)
    {
        oled_caractere(
            handle,
            *texto
        );

        texto++;
    }
}


/* =========================================================
 * DHT22
 * ========================================================= */

static esp_err_t datacenter_ler_dht(
    gpio_num_t gpio,
    float *temperatura,
    float *umidade
)
{
    if (
        temperatura == NULL ||
        umidade == NULL
    )
    {
        return ESP_ERR_INVALID_ARG;
    }


    float t = 0.0f;
    float u = 0.0f;


    esp_err_t erro = dht_read_float_data(
        DHT_TYPE_AM2301,
        gpio,
        &u,
        &t
    );


    if (erro == ESP_OK)
    {
        *temperatura = t;
        *umidade = u;

        return ESP_OK;
    }


    ESP_LOGW(
        TAG,
        "Falha DHT GPIO %d: %s",
        gpio,
        esp_err_to_name(erro)
    );


    return erro;
}


/* =========================================================
 * SENSOR DE GAS
 *
 * Retorna o valor bruto do ADC.
 *
 * NAO e ppm.
 * ========================================================= */

static int datacenter_ler_gas(void)
{
    int valor = 0;


    if (gas_adc_handle == NULL)
    {
        return 0;
    }


    esp_err_t erro = adc_oneshot_read(
        gas_adc_handle,
        DC_GAS_ADC_CHANNEL,
        &valor
    );


    if (erro != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Erro leitura gas: %s",
            esp_err_to_name(erro)
        );

        return 0;
    }


    return valor;
}


/* =========================================================
 * SIMULACAO AUTOMATICA
 *
 * O modo automatico e utilizado como demonstracao.
 *
 * A cada ciclo um novo cenario e escolhido:
 *
 * 0 = NORMAL
 * 1 = ATENCAO
 * 2 = CRITICO
 *
 * Os valores sao aleatorios dentro de faixas
 * diferentes para facilitar a demonstracao da IA V1.
 *
 * IMPORTANTE:
 * Estes valores NAO representam sensores reais.
 * ========================================================= */

static float datacenter_aleatorio_float(
    float minimo,
    float maximo
)
{
    uint32_t valor =
        esp_random();

    float percentual =
        (float)valor /
        (float)UINT32_MAX;

    return minimo +
        (
            (maximo - minimo) *
            percentual
        );
}


static int datacenter_aleatorio_int(
    int minimo,
    int maximo
)
{
    if (maximo <= minimo)
    {
        return minimo;
    }

    uint32_t valor =
        esp_random();

    return minimo +
        (int)(
            valor %
            (uint32_t)(maximo - minimo + 1)
        );
}


static void datacenter_simular_automatico(
    datacenter_t *dc
)
{
    if (dc == NULL)
    {
        return;
    }


    uint32_t cenario =
        esp_random() %
        DC_TOTAL_CENARIOS;


    dc->cenario_automatico =
        cenario;


    switch (cenario)
    {
        /* =================================================
         * NORMAL
         * ================================================= */

        case DC_CENARIO_NORMAL:

            dc->sensores.temperatura_ambiente =
                datacenter_aleatorio_float(
                    18.0f,
                    21.0f
                );

            dc->sensores.umidade_ambiente =
                datacenter_aleatorio_float(
                    45.0f,
                    60.0f
                );

            dc->sensores.temperatura_equipamentos =
                datacenter_aleatorio_float(
                    18.5f,
                    21.5f
                );

            dc->sensores.umidade_equipamentos =
                datacenter_aleatorio_float(
                    45.0f,
                    60.0f
                );

            dc->sensores.gas =
                datacenter_aleatorio_int(
                    500,
                    1299
                );

            break;


        /* =================================================
         * ATENCAO
         * ================================================= */

        case DC_CENARIO_ATENCAO:

            dc->sensores.temperatura_ambiente =
                datacenter_aleatorio_float(
                    21.5f,
                    23.0f
                );

            dc->sensores.umidade_ambiente =
                datacenter_aleatorio_float(
                    55.0f,
                    70.0f
                );

            dc->sensores.temperatura_equipamentos =
                datacenter_aleatorio_float(
                    22.2f,
                    23.8f
                );

            dc->sensores.umidade_equipamentos =
                datacenter_aleatorio_float(
                    55.0f,
                    70.0f
                );

            dc->sensores.gas =
                datacenter_aleatorio_int(
                    1900,
                    2599
                );

            break;


        /* =================================================
         * CRITICO
         * ================================================= */

        case DC_CENARIO_CRITICO:

            dc->sensores.temperatura_ambiente =
                datacenter_aleatorio_float(
                    24.5f,
                    29.0f
                );

            dc->sensores.umidade_ambiente =
                datacenter_aleatorio_float(
                    65.0f,
                    85.0f
                );

            dc->sensores.temperatura_equipamentos =
                datacenter_aleatorio_float(
                    25.0f,
                    30.0f
                );

            dc->sensores.umidade_equipamentos =
                datacenter_aleatorio_float(
                    65.0f,
                    85.0f
                );

            dc->sensores.gas =
                datacenter_aleatorio_int(
                    3000,
                    4095
                );

            break;


        default:

            dc->cenario_automatico =
                DC_CENARIO_NORMAL;

            dc->sensores.temperatura_ambiente =
                20.0f;

            dc->sensores.umidade_ambiente =
                50.0f;

            dc->sensores.temperatura_equipamentos =
                20.0f;

            dc->sensores.umidade_equipamentos =
                50.0f;

            dc->sensores.gas =
                800;

            break;
    }


    /*
     * No modo automatico mantemos a energia presente.
     *
     * Dessa forma a IA pode demonstrar NORMAL,
     * ATENCAO e CRITICO sem que a condicao
     * SEM ENERGIA esconda as tres classes.
     */

    dc->energia.presente = true;

    dc->estado.energia_presente = true;


    ESP_LOGI(
        TAG,
        "SIMULACAO | Cenario: %s | "
        "Amb: %.1f C / %.0f %% | "
        "Rack: %.1f C / %.0f %% | "
        "Gas: %d",
        datacenter_cenario_string(
            dc->cenario_automatico
        ),
        dc->sensores.temperatura_ambiente,
        dc->sensores.umidade_ambiente,
        dc->sensores.temperatura_equipamentos,
        dc->sensores.umidade_equipamentos,
        dc->sensores.gas
    );
}


/* =========================================================
 * STRING DO CENARIO AUTOMATICO
 * ========================================================= */

static const char *datacenter_cenario_string(
    uint32_t cenario
)
{
    switch (cenario)
    {
        case DC_CENARIO_NORMAL:
            return "NORMAL";

        case DC_CENARIO_ATENCAO:
            return "ATENCAO";

        case DC_CENARIO_CRITICO:
            return "CRITICO";

        default:
            return "DESCONHECIDO";
    }
}


/* =========================================================
 * HISTORICO
 * ========================================================= */

static void datacenter_atualizar_historico(
    datacenter_t *dc,
    uint32_t delta
)
{
    float temperatura =
        dc->sensores.temperatura_equipamentos;


    if (
        temperatura <
        dc->historico.temperatura_minima
    )
    {
        dc->historico.temperatura_minima =
            temperatura;
    }


    if (
        temperatura >
        dc->historico.temperatura_maxima
    )
    {
        dc->historico.temperatura_maxima =
            temperatura;
    }


    if (
        temperatura >
        dc->historico.temperatura_pico
    )
    {
        dc->historico.temperatura_pico =
            temperatura;
    }


    float diferenca =
        fabsf(
            dc->sensores.temperatura_equipamentos -
            dc->sensores.temperatura_ambiente
        );


    if (
        diferenca >
        dc->historico.diferenca_termica_maxima
    )
    {
        dc->historico.diferenca_termica_maxima =
            diferenca;
    }


    bool acima22 =
        temperatura > DC_TEMP_ATENCAO;

    bool acima24 =
        temperatura > DC_TEMP_CRITICA;


    if (
        acima22 &&
        !acima_22_anterior
    )
    {
        dc->historico.quantidade_picos_22++;
    }


    if (
        acima24 &&
        !acima_24_anterior
    )
    {
        dc->historico.quantidade_picos_24++;
    }


    if (acima22)
    {
        dc->historico.tempo_acima_22 +=
            delta;

        dc->historico.tempo_continuo_acima_22 +=
            delta;
    }
    else
    {
        dc->historico.tempo_continuo_acima_22 = 0;
    }


    if (acima24)
    {
        dc->historico.tempo_acima_24 +=
            delta;

        dc->historico.tempo_continuo_acima_24 +=
            delta;
    }
    else
    {
        dc->historico.tempo_continuo_acima_24 = 0;
    }


    acima_22_anterior = acima22;
    acima_24_anterior = acima24;
}


/* =========================================================
 * ENERGIA
 * ========================================================= */

static void datacenter_atualizar_energia(
    datacenter_t *dc,
    uint32_t delta
)
{
    bool presente =
        gpio_get_level(
            DC_ENERGIA_GPIO
        );


    dc->energia.presente =
        presente;


    if (
        !presente &&
        energia_anterior
    )
    {
        dc->energia.quantidade_quedas++;

        dc->energia.duracao_queda_atual = 0;

        ESP_LOGW(
            TAG,
            "QUEDA DE ENERGIA"
        );
    }


    if (!presente)
    {
        dc->energia.duracao_queda_atual +=
            delta;
    }


    if (
        presente &&
        !energia_anterior
    )
    {
        dc->energia.ultima_duracao_queda =
            dc->energia.duracao_queda_atual;

        dc->energia.duracao_queda_atual = 0;

        ESP_LOGI(
            TAG,
            "ENERGIA RESTABELECIDA"
        );
    }


    energia_anterior =
        presente;
}


/* =========================================================
 * ESTADO DETERMINISTICO
 *
 * MODO MANUAL
 *
 * Esta funcao representa a logica tradicional do sistema.
 *
 * Ela nao depende da IA.
 * ========================================================= */

static void datacenter_atualizar_estado(
    datacenter_t *dc
)
{
    float temperatura =
        dc->sensores.temperatura_equipamentos;


    dc->estado.temperatura_baixa =
        temperatura < DC_TEMP_BAIXA;


    dc->estado.temperatura_alta =
        temperatura > DC_TEMP_ATENCAO;


    dc->estado.temperatura_critica =
        temperatura > DC_TEMP_CRITICA;


    dc->estado.gas_atencao =
        dc->sensores.gas >=
        DC_GAS_ATENCAO;


    dc->estado.gas_critico =
        dc->sensores.gas >=
        DC_GAS_CRITICO;


    dc->estado.energia_presente =
        dc->energia.presente;


    dc->estado.climatizacao_suspeita =
        dc->historico.tempo_continuo_acima_22 >=
        DC_TEMPO_CRITICO_SEG;


    if (!dc->energia.presente)
    {
        dc->estado.status =
            DC_STATUS_SEM_ENERGIA;
    }

    else if (dc->estado.gas_critico)
    {
        dc->estado.status =
            DC_STATUS_GAS;
    }

    else if (
        dc->estado.temperatura_critica ||
        dc->estado.climatizacao_suspeita
    )
    {
        dc->estado.status =
            DC_STATUS_CRITICO;
    }

    else if (
        dc->estado.temperatura_alta ||
        dc->estado.gas_atencao ||
        dc->estado.temperatura_baixa
    )
    {
        dc->estado.status =
            DC_STATUS_ATENCAO;
    }

    else
    {
        dc->estado.status =
            DC_STATUS_NORMAL;
    }


    dc->estado.alarme_ativo =
        (
            dc->estado.status ==
            DC_STATUS_CRITICO
        ) ||
        (
            dc->estado.status ==
            DC_STATUS_SEM_ENERGIA
        ) ||
        (
            dc->estado.status ==
            DC_STATUS_GAS
        );
}


/* =========================================================
 * IA V1
 * ========================================================= */

static void datacenter_atualizar_ia(
    datacenter_t *dc
)
{
    ia_v1_resultado_t resultado;


    if (!dc->ia.disponivel)
    {
        return;
    }


    bool sucesso =
        ia_v1_predict(
            dc->sensores.temperatura_ambiente,
            dc->sensores.umidade_ambiente,
            dc->sensores.temperatura_equipamentos,
            dc->sensores.umidade_equipamentos,
            (float)dc->sensores.gas,
            dc->energia.presente ? 1.0f : 0.0f,
            &resultado
        );


    if (!sucesso)
    {
        ESP_LOGW(
            TAG,
            "Falha na inferencia da IA V1"
        );

        dc->ia.disponivel = false;

        return;
    }


    dc->ia.classe =
        resultado.classe;

    dc->ia.probabilidade_normal =
        resultado.probabilidade_normal;

    dc->ia.probabilidade_atencao =
        resultado.probabilidade_atencao;

    dc->ia.probabilidade_critico =
        resultado.probabilidade_critico;


    ESP_LOGI(
        TAG,
        "IA V1: %s | "
        "N=%.4f A=%.4f C=%.4f",
        ia_v1_status_string(
            dc->ia.classe
        ),
        dc->ia.probabilidade_normal,
        dc->ia.probabilidade_atencao,
        dc->ia.probabilidade_critico
    );
}


/* =========================================================
 * CONTROLADOR
 *
 * MODO 0 = MANUAL
 * MODO 1 = AUTOMATICO
 *
 * MANUAL:
 *   Usa as regras deterministicas.
 *
 * AUTOMATICO:
 *   Usa a classificacao da IA V1.
 *
 * A ausencia de energia possui prioridade quando
 * o modo MANUAL esta ativo.
 *
 * No modo AUTOMATICO a energia e mantida presente
 * pela simulacao para permitir demonstrar as tres
 * classes da IA.
 * ========================================================= */

static void datacenter_aplicar_modo_controle(
    datacenter_t *dc
)
{
    /*
     * =====================================================
     * MODO MANUAL
     * =====================================================
     */

    if (
        dc->modo_controle ==
        DC_MODO_MANUAL
    )
    {
        /*
         * No manual os sensores e a energia fisica
         * determinam completamente o estado.
         */

        datacenter_atualizar_estado(
            dc
        );
    }

    /*
     * =====================================================
     * MODO AUTOMATICO
     * =====================================================
     */

    else if (
        dc->modo_controle ==
        DC_MODO_AUTOMATICO
    )
    {
        /*
         * Se a IA estiver disponivel,
         * ela define a classificacao final.
         */

        if (dc->ia.disponivel)
        {
            switch (dc->ia.classe)
            {
                case IA_V1_NORMAL:

                    dc->estado.status =
                        DC_STATUS_NORMAL;

                    break;


                case IA_V1_ATENCAO:

                    dc->estado.status =
                        DC_STATUS_ATENCAO;

                    break;


                case IA_V1_CRITICO:

                    dc->estado.status =
                        DC_STATUS_CRITICO;

                    break;


                default:

                    /*
                     * Caso inesperado:
                     * utiliza a logica deterministica.
                     */

                    datacenter_atualizar_estado(
                        dc
                    );

                    break;
            }
        }

        /*
         * IA indisponivel:
         * usa a logica tradicional como fallback.
         */

        else
        {
            datacenter_atualizar_estado(
                dc
            );
        }
    }

    /*
     * =====================================================
     * MODO INVALIDO
     * =====================================================
     */

    else
    {
        ESP_LOGW(
            TAG,
            "Modo de controle invalido: %d",
            dc->modo_controle
        );

        dc->modo_controle =
            DC_MODO_MANUAL;

        datacenter_atualizar_estado(
            dc
        );
    }


    /*
     * =====================================================
     * ALARME
     * =====================================================
     */

    dc->estado.alarme_ativo =
        (
            dc->estado.status ==
            DC_STATUS_CRITICO
        ) ||
        (
            dc->estado.status ==
            DC_STATUS_SEM_ENERGIA
        ) ||
        (
            dc->estado.status ==
            DC_STATUS_GAS
        );
}


/* =========================================================
 * STRING DO MODO
 * ========================================================= */

static const char *datacenter_modo_string(
    uint8_t modo
)
{
    switch (modo)
    {
        case DC_MODO_MANUAL:
            return "MANUAL";

        case DC_MODO_AUTOMATICO:
            return "AUTOMATICO";

        default:
            return "INVALIDO";
    }
}


/* =========================================================
 * STATUS
 * ========================================================= */

const char *datacenter_status_string(
    dc_status_t status
)
{
    switch (status)
    {
        case DC_STATUS_NORMAL:
            return "NORMAL";

        case DC_STATUS_ATENCAO:
            return "ATENCAO";

        case DC_STATUS_CRITICO:
            return "CRITICO";

        case DC_STATUS_SEM_ENERGIA:
            return "SEM ENERGIA";

        case DC_STATUS_GAS:
            return "GAS";

        default:
            return "DESCONHECIDO";
    }
}


/* =========================================================
 * LEDS
 * ========================================================= */

void datacenter_atualizar_leds(
    datacenter_t *dc
)
{
    gpio_set_level(
        DC_LED_VERDE_GPIO,
        0
    );

    gpio_set_level(
        DC_LED_AMARELO_GPIO,
        0
    );

    gpio_set_level(
        DC_LED_VERMELHO_GPIO,
        0
    );


    switch (dc->estado.status)
    {
        case DC_STATUS_NORMAL:

            gpio_set_level(
                DC_LED_VERDE_GPIO,
                1
            );

            break;


        case DC_STATUS_ATENCAO:

            gpio_set_level(
                DC_LED_AMARELO_GPIO,
                1
            );

            break;


        case DC_STATUS_CRITICO:
        case DC_STATUS_GAS:
        case DC_STATUS_SEM_ENERGIA:

            gpio_set_level(
                DC_LED_VERMELHO_GPIO,
                1
            );

            break;


        default:
            break;
    }
}


/* =========================================================
 * BUZZER
 * ========================================================= */

void datacenter_atualizar_alarme(
    datacenter_t *dc
)
{
    if (dc->estado.alarme_ativo)
    {
        ledc_set_duty(
            LEDC_LOW_SPEED_MODE,
            LEDC_CHANNEL_0,
            4096
        );

        ledc_update_duty(
            LEDC_LOW_SPEED_MODE,
            LEDC_CHANNEL_0
        );
    }
    else
    {
        ledc_set_duty(
            LEDC_LOW_SPEED_MODE,
            LEDC_CHANNEL_0,
            0
        );

        ledc_update_duty(
            LEDC_LOW_SPEED_MODE,
            LEDC_CHANNEL_0
        );
    }
}


/* =========================================================
 * OLED 1
 * ========================================================= */

void datacenter_oled_atual(
    datacenter_t *dc
)
{
    char linha[32];


    oled_limpar(
        oled1_handle
    );


    oled_texto(
        oled1_handle,
        0,
        0,
        "DATACENTER"
    );


    snprintf(
        linha,
        sizeof(linha),
        "TEMP: %.1f C",
        dc->sensores.temperatura_equipamentos
    );

    oled_texto(
        oled1_handle,
        0,
        2,
        linha
    );


    snprintf(
        linha,
        sizeof(linha),
        "UMID: %.0f %%",
        dc->sensores.umidade_equipamentos
    );

    oled_texto(
        oled1_handle,
        0,
        4,
        linha
    );


    snprintf(
        linha,
        sizeof(linha),
        "STATUS: %s",
        datacenter_status_string(
            dc->estado.status
        )
    );

    oled_texto(
        oled1_handle,
        0,
        6,
        linha
    );
}


/* =========================================================
 * OLED 2
 *
 * Analise
 * ========================================================= */

void datacenter_oled_analise(
    datacenter_t *dc
)
{
    char linha[32];


    oled_limpar(
        oled2_handle
    );


    oled_texto(
        oled2_handle,
        0,
        0,
        "MONITORAMENTO"
    );


    snprintf(
        linha,
        sizeof(linha),
        "PICOS >22: %lu",
        (unsigned long)
            dc->historico.quantidade_picos_22
    );

    oled_texto(
        oled2_handle,
        0,
        2,
        linha
    );


    uint32_t minutos =
        dc->historico.tempo_acima_22 / 60;

    uint32_t segundos =
        dc->historico.tempo_acima_22 % 60;


    oled_texto(
        oled2_handle,
        0,
        4,
        "TEMPO ALTO:"
    );


    snprintf(
        linha,
        sizeof(linha),
        "%02lu:%02lu",
        (unsigned long)minutos,
        (unsigned long)segundos
    );

    oled_texto(
        oled2_handle,
        0,
        6,
        linha
    );
}


/* =========================================================
 * IA - TEXTO PARA LCD
 * ========================================================= */

static const char *datacenter_ia_string(
    datacenter_t *dc
)
{
    if (!dc->ia.disponivel)
    {
        return "IA: INDISPONIVEL";
    }


    switch (dc->ia.classe)
    {
        case IA_V1_NORMAL:
            return "IA: NORMAL";

        case IA_V1_ATENCAO:
            return "IA: ATENCAO";

        case IA_V1_CRITICO:
            return "IA: CRITICO";

        default:
            return "IA: DESCONHECIDA";
    }
}


/* =========================================================
 * LCD - PAGINA PRINCIPAL
 * ========================================================= */

static void lcd_pagina_principal(
    datacenter_t *dc
)
{
    char linha[32];


    lcd_linha(
        0,
        "   DATACENTER"
    );


    snprintf(
        linha,
        sizeof(linha),
        "TEMP:%.1fC UMID:%.0f%%",
        dc->sensores.temperatura_equipamentos,
        dc->sensores.umidade_equipamentos
    );

    lcd_linha(
        1,
        linha
    );


    snprintf(
        linha,
        sizeof(linha),
        "STATUS: %s",
        datacenter_status_string(
            dc->estado.status
        )
    );

    lcd_linha(
        2,
        linha
    );


    snprintf(
        linha,
        sizeof(linha),
        "MODO: %s",
        datacenter_modo_string(
            dc->modo_controle
        )
    );

    lcd_linha(
        3,
        linha
    );
}


/* =========================================================
 * LCD - PAGINA ENERGIA
 * ========================================================= */

static void lcd_pagina_energia(
    datacenter_t *dc
)
{
    char linha[32];


    lcd_linha(
        0,
        "ENERGIA"
    );


    snprintf(
        linha,
        sizeof(linha),
        "QUEDAS: %lu",
        (unsigned long)
            dc->energia.quantidade_quedas
    );

    lcd_linha(
        1,
        linha
    );


    uint32_t segundos =
        dc->energia.duracao_queda_atual;

    uint32_t minutos =
        segundos / 60;


    snprintf(
        linha,
        sizeof(linha),
        "SEM ENERGIA: %lum",
        (unsigned long)minutos
    );

    lcd_linha(
        2,
        linha
    );


    if (dc->energia.presente)
    {
        lcd_linha(
            3,
            "ENERGIA: NORMAL"
        );
    }
    else
    {
        lcd_linha(
            3,
            "ENERGIA: AUSENTE"
        );
    }
}


/* =========================================================
 * LCD - PAGINA CLIMATIZACAO
 * ========================================================= */

static void lcd_pagina_clima(
    datacenter_t *dc
)
{
    char linha[32];


    lcd_linha(
        0,
        "CLIMATIZACAO"
    );


    uint32_t minutos22 =
        dc->historico.tempo_acima_22 / 60;


    snprintf(
        linha,
        sizeof(linha),
        ">22C: %02luh%02lum",
        (unsigned long)
            (minutos22 / 60),
        (unsigned long)
            (minutos22 % 60)
    );

    lcd_linha(
        1,
        linha
    );


    uint32_t minutos24 =
        dc->historico.tempo_acima_24 / 60;


    snprintf(
        linha,
        sizeof(linha),
        ">24C: %02luh%02lum",
        (unsigned long)
            (minutos24 / 60),
        (unsigned long)
            (minutos24 % 60)
    );

    lcd_linha(
        2,
        linha
    );


    snprintf(
        linha,
        sizeof(linha),
        "PICO: %.1fC",
        dc->historico.temperatura_pico
    );

    lcd_linha(
        3,
        linha
    );
}


/* =========================================================
 * LCD - ATUALIZACAO
 * ========================================================= */

void datacenter_lcd_atualizar(
    datacenter_t *dc
)
{
    switch (pagina_lcd)
    {
        case 0:

            lcd_pagina_principal(dc);

            break;


        case 1:

            lcd_pagina_energia(dc);

            break;


        case 2:

            lcd_pagina_clima(dc);

            break;


        default:

            pagina_lcd = 0;

            lcd_pagina_principal(dc);

            break;
    }


    pagina_lcd++;


    if (pagina_lcd >= 3)
    {
        pagina_lcd = 0;
    }


    dc->pagina_lcd =
        pagina_lcd;
}


/* =========================================================
 * GPIO E PWM (LEDC)
 * ========================================================= */

static void datacenter_gpio_init(void)
{
    gpio_config_t saidas =
    {
        .pin_bit_mask =
            (
                (1ULL << DC_LED_VERMELHO_GPIO) |
                (1ULL << DC_LED_AMARELO_GPIO) |
                (1ULL << DC_LED_VERDE_GPIO)
            ),

        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&saidas);

    gpio_set_level(DC_LED_VERMELHO_GPIO, 0);
    gpio_set_level(DC_LED_AMARELO_GPIO, 0);
    gpio_set_level(DC_LED_VERDE_GPIO, 0);


    ledc_timer_config_t buzzer_timer =
    {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 1500,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ledc_timer_config(&buzzer_timer);


    ledc_channel_config_t buzzer_channel =
    {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = DC_BUZZER_GPIO,
        .duty = 0,
        .hpoint = 0
    };

    ledc_channel_config(&buzzer_channel);


    gpio_config_t entrada =
    {
        .pin_bit_mask =
            (1ULL << DC_ENERGIA_GPIO),

        .mode = GPIO_MODE_INPUT,

        .pull_up_en = GPIO_PULLUP_ENABLE,

        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&entrada);
}


/* =========================================================
 * ADC
 * ========================================================= */

static esp_err_t datacenter_adc_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_config =
    {
        .unit_id =
            DC_GAS_ADC_UNIT
    };


    esp_err_t erro =
        adc_oneshot_new_unit(
            &unit_config,
            &gas_adc_handle
        );


    if (erro != ESP_OK)
    {
        return erro;
    }


    adc_oneshot_chan_cfg_t channel_config =
    {
        .atten =
            DC_GAS_ADC_ATTEN,

        .bitwidth =
            ADC_BITWIDTH_DEFAULT
    };


    erro =
        adc_oneshot_config_channel(
            gas_adc_handle,
            DC_GAS_ADC_CHANNEL,
            &channel_config
        );


    if (erro != ESP_OK)
    {
        return erro;
    }


    ESP_LOGI(
        TAG,
        "ADC gas configurado - GPIO7"
    );


    return ESP_OK;
}


/* =========================================================
 * INICIALIZACAO
 * ========================================================= */

esp_err_t datacenter_init(
    datacenter_t *dc
)
{
    if (dc == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    memset(
        dc,
        0,
        sizeof(datacenter_t)
    );


    /*
     * =====================================================
     * MODO INICIAL
     * =====================================================
     *
     * 0 = MANUAL
     * 1 = AUTOMATICO
     *
     * O sistema inicia em AUTOMATICO.
     */

    dc->modo_controle =
        DC_MODO_AUTOMATICO;

    dc->cenario_automatico =
        DC_CENARIO_NORMAL;


    dc->historico.temperatura_minima =
        999.0f;

    dc->historico.temperatura_maxima =
        -999.0f;

    dc->historico.temperatura_pico =
        -999.0f;


    /* =====================================================
     * GPIO E PWM
     * ===================================================== */

    datacenter_gpio_init();


    /* =====================================================
     * I2C
     * ===================================================== */

    esp_err_t erro =
        datacenter_i2c_init();

    if (erro != ESP_OK)
    {
        return erro;
    }


    /* =====================================================
     * LCD
     * ===================================================== */

    erro =
        lcd_init();

    if (erro != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "LCD 20x4 nao respondeu: %s",
            esp_err_to_name(erro)
        );
    }


    /* =====================================================
     * OLEDs
     * ===================================================== */

    oled_init(
        oled1_handle
    );

    oled_init(
        oled2_handle
    );


    oled_limpar(
        oled1_handle
    );

    oled_limpar(
        oled2_handle
    );


    /* =====================================================
     * ADC
     * ===================================================== */

    erro =
        datacenter_adc_init();

    if (erro != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Erro ADC: %s",
            esp_err_to_name(erro)
        );

        return erro;
    }


    /* =====================================================
     * ENERGIA
     * ===================================================== */

    dc->energia.presente =
        gpio_get_level(
            DC_ENERGIA_GPIO
        );


    energia_anterior =
        dc->energia.presente;


    /*
     * No modo automatico a energia sera simulada
     * como presente a cada ciclo.
     */


    /* =====================================================
     * IA V1
     * ===================================================== */

    dc->ia.disponivel =
        ia_v1_init();


    if (dc->ia.disponivel)
    {
        ESP_LOGI(
            TAG,
            "IA V1 disponivel"
        );
    }
    else
    {
        ESP_LOGW(
            TAG,
            "IA V1 indisponivel"
        );
    }


    ultimo_update =
        xTaskGetTickCount();


    ESP_LOGI(
        TAG,
        "========================================"
    );

    ESP_LOGI(
        TAG,
        "MONITORAMENTO INTELIGENTE DATACENTER"
    );

    ESP_LOGI(
        TAG,
        "ESP32-S3"
    );

    ESP_LOGI(
        TAG,
        "MODO CONTROLE: %d (%s)",
        dc->modo_controle,
        datacenter_modo_string(
            dc->modo_controle
        )
    );

    ESP_LOGI(
        TAG,
        "SIMULACAO AUTOMATICA: NORMAL / ATENCAO / CRITICO"
    );

    ESP_LOGI(
        TAG,
        "========================================"
    );


    return ESP_OK;
}


/* =========================================================
 * ATUALIZACAO
 * ========================================================= */

esp_err_t datacenter_atualizar(
    datacenter_t *dc
)
{
    if (dc == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }


    /* =====================================================
     * TEMPO
     * ===================================================== */

    uint32_t agora =
        xTaskGetTickCount();


    uint32_t delta =
        (
            agora -
            ultimo_update
        ) /
        configTICK_RATE_HZ;


    if (delta == 0)
    {
        delta = 1;
    }


    ultimo_update =
        agora;


    dc->tempo_funcionamento +=
        delta;


    /* =====================================================
     * MODO AUTOMATICO
     *
     * No automatico nao utilizamos os sensores fisicos
     * para a demonstracao.
     *
     * Geramos um novo cenario aleatorio a cada ciclo.
     * ===================================================== */

    if (
        dc->modo_controle ==
        DC_MODO_AUTOMATICO
    )
    {
        datacenter_simular_automatico(
            dc
        );
    }


    /* =====================================================
     * MODO MANUAL
     *
     * Leitura dos sensores reais.
     * ===================================================== */

    else
    {
        float temperatura;
        float umidade;


        /* =================================================
         * DHT AMBIENTE
         * ================================================= */

        esp_err_t erro =
            datacenter_ler_dht(
                DC_DHT_AMBIENTE_GPIO,
                &temperatura,
                &umidade
            );


        if (erro == ESP_OK)
        {
            dc->sensores.temperatura_ambiente =
                temperatura;

            dc->sensores.umidade_ambiente =
                umidade;
        }
        else
        {
            ESP_LOGW(
                TAG,
                "Falha DHT ambiente: %s",
                esp_err_to_name(erro)
            );
        }


        /* =================================================
         * DHT EQUIPAMENTOS
         * ================================================= */

        erro =
            datacenter_ler_dht(
                DC_DHT_EQUIPAMENTOS_GPIO,
                &temperatura,
                &umidade
            );


        if (erro == ESP_OK)
        {
            dc->sensores.temperatura_equipamentos =
                temperatura;

            dc->sensores.umidade_equipamentos =
                umidade;
        }
        else
        {
            ESP_LOGW(
                TAG,
                "Falha DHT equipamentos: %s",
                esp_err_to_name(erro)
            );
        }


        /* =================================================
         * GAS
         * ================================================= */

        dc->sensores.gas =
            datacenter_ler_gas();


        /* =================================================
         * ENERGIA
         * ================================================= */

        datacenter_atualizar_energia(
            dc,
            delta
        );
    }


    /* =====================================================
     * HISTORICO
     *
     * Funciona tanto com sensores reais quanto com
     * dados simulados.
     * ===================================================== */

    datacenter_atualizar_historico(
        dc,
        delta
    );


    /* =====================================================
     * ESTADO DETERMINISTICO
     *
     * Calcula as flags auxiliares:
     *
     * temperatura alta
     * temperatura critica
     * gas
     * climatizacao
     * energia
     *
     * No automatico a decisao final sera posteriormente
     * substituida pela classificacao da IA.
     * ===================================================== */

    datacenter_atualizar_estado(
        dc
    );


    /* =====================================================
     * IA V1
     *
     * Cada novo cenario passa pela IA.
     * ===================================================== */

    datacenter_atualizar_ia(
        dc
    );


    /* =====================================================
     * CONTROLADOR
     *
     * MANUAL:
     *     regras tradicionais
     *
     * AUTOMATICO:
     *     classificacao da IA
     * ===================================================== */

    datacenter_aplicar_modo_controle(
        dc
    );


    /* =====================================================
     * LEDS
     * ===================================================== */

    datacenter_atualizar_leds(
        dc
    );


    /* =====================================================
     * BUZZER
     * ===================================================== */

    datacenter_atualizar_alarme(
        dc
    );


    /* =====================================================
     * LOG FINAL
     *
     * No automatico mostramos explicitamente:
     *
     * - cenario gerado
     * - valores simulados
     * - classificacao da IA
     * - probabilidades
     * - status final
     *
     * Isso permite demonstrar que a IA esta avaliando
     * diversos cenarios.
     * ===================================================== */

    if (
        dc->modo_controle ==
        DC_MODO_AUTOMATICO
    )
    {
        if (dc->ia.disponivel)
        {
            ESP_LOGI(
                TAG,
                "RESULTADO | "
                "Cenario: %s | "
                "Amb: %.1f C / %.0f %% | "
                "Rack: %.1f C / %.0f %% | "
                "Gas: %d | "
                "IA: %s | "
                "N=%.4f A=%.4f C=%.4f | "
                "Status: %s",
                datacenter_cenario_string(
                    dc->cenario_automatico
                ),
                dc->sensores.temperatura_ambiente,
                dc->sensores.umidade_ambiente,
                dc->sensores.temperatura_equipamentos,
                dc->sensores.umidade_equipamentos,
                dc->sensores.gas,
                datacenter_ia_string(dc),
                dc->ia.probabilidade_normal,
                dc->ia.probabilidade_atencao,
                dc->ia.probabilidade_critico,
                datacenter_status_string(
                    dc->estado.status
                )
            );
        }
        else
        {
            ESP_LOGI(
                TAG,
                "RESULTADO | "
                "Cenario: %s | "
                "Amb: %.1f C / %.0f %% | "
                "Rack: %.1f C / %.0f %% | "
                "Gas: %d | "
                "IA: INDISPONIVEL | "
                "Status: %s",
                datacenter_cenario_string(
                    dc->cenario_automatico
                ),
                dc->sensores.temperatura_ambiente,
                dc->sensores.umidade_ambiente,
                dc->sensores.temperatura_equipamentos,
                dc->sensores.umidade_equipamentos,
                dc->sensores.gas,
                datacenter_status_string(
                    dc->estado.status
                )
            );
        }
    }

    else
    {
        if (dc->ia.disponivel)
        {
            ESP_LOGI(
                TAG,
                "MODO MANUAL | "
                "Amb: %.1f C / %.0f %% | "
                "Rack: %.1f C / %.0f %% | "
                "Gas: %d | "
                "IA: %s | "
                "N=%.4f A=%.4f C=%.4f | "
                "Status: %s",
                dc->sensores.temperatura_ambiente,
                dc->sensores.umidade_ambiente,
                dc->sensores.temperatura_equipamentos,
                dc->sensores.umidade_equipamentos,
                dc->sensores.gas,
                datacenter_ia_string(dc),
                dc->ia.probabilidade_normal,
                dc->ia.probabilidade_atencao,
                dc->ia.probabilidade_critico,
                datacenter_status_string(
                    dc->estado.status
                )
            );
        }
        else
        {
            ESP_LOGI(
                TAG,
                "MODO MANUAL | "
                "Amb: %.1f C / %.0f %% | "
                "Rack: %.1f C / %.0f %% | "
                "Gas: %d | "
                "IA: INDISPONIVEL | "
                "Status: %s",
                dc->sensores.temperatura_ambiente,
                dc->sensores.umidade_ambiente,
                dc->sensores.temperatura_equipamentos,
                dc->sensores.umidade_equipamentos,
                dc->sensores.gas,
                datacenter_status_string(
                    dc->estado.status
                )
            );
        }
    }


    return ESP_OK;
}