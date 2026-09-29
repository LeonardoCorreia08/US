#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tflite_runner.h"

void app_main(void) {
    printf("Inicializando modelo TFLite...\n");

    if (model_init() != 0) {
        printf("Falha ao inicializar o modelo TFLite!\n");
        return;
    }

    printf("Modelo carregado com sucesso!\n");

    float x = 0.0f;
    float y = 0.0f;

    // Loop contínuo da infer
    while (1) {
        if (model_run(x, &y) == 0) {
            printf("x: %.2f | y (inferencia): %.6f\n", x, y);
        } else {
            printf("Erro na inferencia!\n");
        }

        // Increm a entrada de 0 a ~6.28 (2 * PI) para varrer o seno
        x += 0.2f;
        if (x > 6.28f) {
            x = 0.0f;
        }

        // Aguarda 500 ms antes da próxima infer
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}