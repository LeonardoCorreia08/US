#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "datacenter.h"

static const char *TAG = "MAIN";

void app_main(void)
void app_main(void)
{
    datacenter_t datacenter;

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   MONITORAMENTO INTELIGENTE DATACENTER");
    ESP_LOGI(TAG, "========================================");

    esp_err_t erro = datacenter_init(&datacenter);

    if (erro != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Falha ao inicializar datacenter: %s",
            esp_err_to_name(erro)
        );

        return;
    }

    ESP_LOGI(
        TAG,
        "Sistema inicializado com sucesso"
    );

    while (1)
    {
        erro = datacenter_atualizar(&datacenter);

        if (erro != ESP_OK)
        {
            ESP_LOGE(
                TAG,
                "Erro na atualizacao: %s",
                esp_err_to_name(erro)
            );
        }

        datacenter_oled_atual(&datacenter);

        datacenter_oled_analise(&datacenter);

        datacenter_lcd_atualizar(&datacenter);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}