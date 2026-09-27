#include "buzzer.h"

#include "driver/ledc.h"

#define BUZZER_GPIO 14

#define BUZZER_TIMER       LEDC_TIMER_0
#define BUZZER_MODE        LEDC_LOW_SPEED_MODE
#define BUZZER_CHANNEL     LEDC_CHANNEL_0
#define BUZZER_DUTY_RES    LEDC_TIMER_13_BIT
#define BUZZER_FREQUENCY   2000

int buzzer_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = BUZZER_MODE,
        .duty_resolution = BUZZER_DUTY_RES,
        .timer_num = BUZZER_TIMER,
        .freq_hz = BUZZER_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };

    esp_err_t erro = ledc_timer_config(
        &timer_config
    );

    if (erro != ESP_OK)
    {
        return -1;
    }

    ledc_channel_config_t channel_config = {
        .gpio_num = BUZZER_GPIO,
        .speed_mode = BUZZER_MODE,
        .channel = BUZZER_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BUZZER_TIMER,
        .duty = 0,
        .hpoint = 0
    };

    erro = ledc_channel_config(
        &channel_config
    );

    if (erro != ESP_OK)
    {
        return -2;
    }

    return 0;
}

void buzzer_on(void)
{
    ledc_set_duty(
        BUZZER_MODE,
        BUZZER_CHANNEL,
        4000
    );

    ledc_update_duty(
        BUZZER_MODE,
        BUZZER_CHANNEL
    );
}

void buzzer_off(void)
{
    ledc_set_duty(
        BUZZER_MODE,
        BUZZER_CHANNEL,
        0
    );

    ledc_update_duty(
        BUZZER_MODE,
        BUZZER_CHANNEL
    );
}

