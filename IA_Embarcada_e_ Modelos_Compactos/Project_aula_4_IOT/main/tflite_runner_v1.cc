#include "tflite_runner_v1.h"

#include <cmath>
#include <cstdint>
#include <cstring>

#include "esp_log.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "model/modelo_v1_int8.cc"


static const char *TAG = "IA_V1";


/* ============================================================
 * CONFIGURAÇÃO DO MODELO
 * ============================================================ */

#define IA_V1_INPUTS   6
#define IA_V1_OUTPUTS  3

#define IA_V1_TENSOR_ARENA_SIZE (12 * 1024)


/* ============================================================
 * TENSOR ARENA
 * ============================================================ */

static uint8_t tensor_arena[
    IA_V1_TENSOR_ARENA_SIZE
];


/* ============================================================
 * OBJETOS TFLITE MICRO
 * ============================================================ */

static const tflite::Model *modelo = nullptr;

static tflite::MicroInterpreter *interpretador = nullptr;

static TfLiteTensor *tensor_entrada = nullptr;

static TfLiteTensor *tensor_saida = nullptr;


/* Resolver de operações.
 *
 * O modelo possui camadas Dense/FullyConnected,
 * Softmax e operações associadas.
 */
static tflite::MicroMutableOpResolver<8> resolver;


/* ============================================================
 * SCALER V1
 *
 * Valores obtidos de scaler_v1.npz
 * ============================================================ */

static const float scaler_mean[IA_V1_INPUTS] =
{
    20.0763492f,
    57.4698413f,
    22.1184127f,
    57.5142857f,
    1898.17460f,
    1.0f
};

static const float scaler_scale[IA_V1_INPUTS] =
{
    4.79979785f,
    10.24888124f,
    4.70349522f,
    10.76777758f,
    992.07915774f,
    1.0f
};


/* ============================================================
 * QUANTIZAÇÃO DA ENTRADA
 * ============================================================ */

static const float INPUT_SCALE =
    0.01982070319354534f;

static const int INPUT_ZERO_POINT =
    -23;


/* ============================================================
 * QUANTIZAÇÃO DA SAÍDA
 * ============================================================ */

static const float OUTPUT_SCALE =
    0.00390625f;

static const int OUTPUT_ZERO_POINT =
    -128;


/* ============================================================
 * FUNÇÃO AUXILIAR
 * ============================================================ */

static int8_t quantizar_entrada(
    float valor,
    int indice
)
{
    /*
     * 1. StandardScaler
     */
    float normalizado =
        (valor - scaler_mean[indice])
        / scaler_scale[indice];

    /*
     * 2. Quantização INT8
     */
    float quantizado =
        (normalizado / INPUT_SCALE)
        + INPUT_ZERO_POINT;

    /*
     * 3. Arredondamento
     */
    int32_t valor_int =
        static_cast<int32_t>(
            std::round(quantizado)
        );

    /*
     * 4. Limite INT8
     */
    if (valor_int > 127)
    {
        valor_int = 127;
    }

    if (valor_int < -128)
    {
        valor_int = -128;
    }

    return static_cast<int8_t>(
        valor_int
    );
}


/* ============================================================
 * INICIALIZAÇÃO
 * ============================================================ */

bool ia_v1_init(void)
{
    ESP_LOGI(
        TAG,
        "Inicializando modelo IA V1"
    );

    /*
     * Carrega modelo FlatBuffer
     */
    modelo = tflite::GetModel(
        modelo_v1_int8_tflite
    );

    if (modelo == nullptr)
    {
        ESP_LOGE(
            TAG,
            "Modelo nao carregado"
        );

        return false;
    }

    /*
     * Verifica versão do schema
     */
    if (
        modelo->version()
        != TFLITE_SCHEMA_VERSION
    )
    {
        ESP_LOGE(
            TAG,
            "Versao do schema incompativel"
        );

        return false;
    }


    /* ========================================================
     * REGISTRAR OPERACOES
     * ======================================================== */

    if (
        resolver.AddFullyConnected()
        != kTfLiteOk
    )
    {
        ESP_LOGE(
            TAG,
            "Erro ao registrar FullyConnected"
        );

        return false;
    }

    if (
        resolver.AddRelu()
        != kTfLiteOk
    )
    {
        ESP_LOGE(
            TAG,
            "Erro ao registrar Relu"
        );

        return false;
    }

    if (
        resolver.AddSoftmax()
        != kTfLiteOk
    )
    {
        ESP_LOGE(
            TAG,
            "Erro ao registrar Softmax"
        );

        return false;
    }


    /* ========================================================
     * CRIAR INTERPRETADOR
     * ======================================================== */

    static tflite::MicroInterpreter
        interpretador_estatico(
            modelo,
            resolver,
            tensor_arena,
            IA_V1_TENSOR_ARENA_SIZE
        );

    interpretador =
        &interpretador_estatico;


    /* ========================================================
     * ALOCAR TENSORES
     * ======================================================== */

    TfLiteStatus status =
        interpretador->AllocateTensors();

    if (status != kTfLiteOk)
    {
        ESP_LOGE(
            TAG,
            "Falha ao alocar tensores"
        );

        return false;
    }


    /* ========================================================
     * OBTER TENSORES
     * ======================================================== */

    tensor_entrada =
        interpretador->input(0);

    tensor_saida =
        interpretador->output(0);


    if (
        tensor_entrada == nullptr
        ||
        tensor_saida == nullptr
    )
    {
        ESP_LOGE(
            TAG,
            "Tensor de entrada/saida invalido"
        );

        return false;
    }


    /* ========================================================
     * VALIDAR ENTRADA
     * ======================================================== */

    if (
        tensor_entrada->type
        != kTfLiteInt8
    )
    {
        ESP_LOGE(
            TAG,
            "Entrada nao e INT8"
        );

        return false;
    }


    /* ========================================================
     * VALIDAR SAIDA
     * ======================================================== */

    if (
        tensor_saida->type
        != kTfLiteInt8
    )
    {
        ESP_LOGE(
            TAG,
            "Saida nao e INT8"
        );

        return false;
    }


    ESP_LOGI(
        TAG,
        "Modelo IA V1 inicializado"
    );

    ESP_LOGI(
        TAG,
        "Entrada: INT8 [1,6]"
    );

    ESP_LOGI(
        TAG,
        "Saida: INT8 [1,3]"
    );


    return true;
}


/* ============================================================
 * INFERÊNCIA
 * ============================================================ */

bool ia_v1_predict(
    float temperatura_ambiente,
    float umidade_ambiente,
    float temperatura_rack,
    float umidade_rack,
    float gas,
    float energia,
    ia_v1_resultado_t *resultado
)
{
    if (
        interpretador == nullptr
        ||
        tensor_entrada == nullptr
        ||
        tensor_saida == nullptr
        ||
        resultado == nullptr
    )
    {
        return false;
    }


    /* ========================================================
     * ENTRADAS
     * ======================================================== */

    float valores[IA_V1_INPUTS] =
    {
        temperatura_ambiente,
        umidade_ambiente,
        temperatura_rack,
        umidade_rack,
        gas,
        energia
    };


    /* ========================================================
     * NORMALIZAÇÃO + QUANTIZAÇÃO
     * ======================================================== */

    for (int i = 0; i < IA_V1_INPUTS; i++)
    {
        tensor_entrada->data.int8[i] =
            quantizar_entrada(
                valores[i],
                i
            );
    }


    /* ========================================================
     * EXECUTAR MODELO
     * ======================================================== */

    TfLiteStatus status =
        interpretador->Invoke();

    if (status != kTfLiteOk)
    {
        ESP_LOGE(
            TAG,
            "Falha na inferencia"
        );

        return false;
    }


    /* ========================================================
     * LER SAIDA
     * ======================================================== */

    float probabilidades[
        IA_V1_OUTPUTS
    ];


    for (int i = 0; i < IA_V1_OUTPUTS; i++)
    {
        int valor_quantizado =
            tensor_saida->data.int8[i];

        probabilidades[i] =
            (
                valor_quantizado
                - OUTPUT_ZERO_POINT
            )
            * OUTPUT_SCALE;
    }


    /* ========================================================
     * IDENTIFICAR MAIOR PROBABILIDADE
     * ======================================================== */

    int classe = 0;

    float maior =
        probabilidades[0];

    for (int i = 1; i < IA_V1_OUTPUTS; i++)
    {
        if (
            probabilidades[i]
            > maior
        )
        {
            maior =
                probabilidades[i];

            classe = i;
        }
    }


    /* ========================================================
     * RESULTADO
     * ======================================================== */

    resultado->classe =
        static_cast<ia_v1_status_t>(
            classe
        );

    resultado->probabilidade_normal =
        probabilidades[0];

    resultado->probabilidade_atencao =
        probabilidades[1];

    resultado->probabilidade_critico =
        probabilidades[2];


    return true;
}


/* ============================================================
 * NOME DA CLASSE
 * ============================================================ */

const char *ia_v1_status_string(
    ia_v1_status_t status
)
{
    switch (status)
    {
        case IA_V1_NORMAL:
            return "NORMAL";

        case IA_V1_ATENCAO:
            return "ATENCAO";

        case IA_V1_CRITICO:
            return "CRITICO";

        default:
            return "DESCONHECIDO";
    }
}