#include "modelo_ia.h"

// Escalas de Normalizacao (MinMax)
const float input_min[5] = {0.000000f, 6.630000f, 25.043360f, 274.091127f, 365.856833f};
const float input_scale[5] = {15.000000f, 1.170000f, 1.746247f, 2685.395628f, 433.940708f};
const float output_min[4] = {5019.700000f, 271.000000f, 1942.300000f, 133.300000f};
const float output_scale[4] = {16092.600000f, 381.000000f, 3579.400000f, 119.000000f};

// Pesos e Vieses da Camada 1
const float w1[8][5] = {
    {0.470368f, -0.352888f, 0.420052f, -0.018595f, -0.290045f},
    {0.378502f, 0.231903f, 0.473964f, -0.077403f, -0.412263f},
    {0.407402f, 0.153862f, -0.424759f, 0.771264f, 0.687625f},
    {0.305176f, -0.432976f, -0.238834f, 0.031455f, -0.625278f},
    {0.716994f, -0.128319f, -0.172627f, -0.482869f, 0.630580f},
    {-0.223752f, 0.739467f, 0.674956f, -0.045623f, 0.629960f},
    {0.390982f, 0.270799f, 0.448895f, -0.256479f, -0.247598f},
    {-0.617279f, 0.501790f, -0.517199f, -0.536177f, -0.572400f}
};
const float b1[8] = {0.176536f, -0.043673f, 0.130880f, 0.139174f, 0.099894f, 0.163991f, -0.043337f, 0.000000f};

// Pesos e Vieses da Camada 2
const float w2[8][8] = {
    {-0.531086f, -0.309044f, -0.352364f, 0.555404f, -0.076124f, -0.585646f, -0.452770f, 0.522216f},
    {-0.091424f, -0.591102f, -0.565623f, 0.289654f, 0.478568f, 0.090612f, 0.137371f, 0.188664f},
    {-0.063402f, -0.629728f, 0.268024f, -0.066914f, -0.467249f, 0.776427f, -0.164667f, 0.599435f},
    {0.263594f, 0.320715f, -0.118152f, 0.601477f, -0.327282f, -0.146059f, 0.028096f, 0.439007f},
    {-0.273124f, -0.023960f, -0.275592f, -0.329391f, 0.505231f, 0.346287f, -0.494324f, 0.504422f},
    {0.059390f, -0.260347f, -0.330148f, -0.018170f, -0.603013f, -0.428978f, 0.030856f, -0.045568f},
    {0.107767f, -0.052203f, -0.171936f, -0.532643f, -0.591158f, -0.121431f, 0.496376f, 0.138462f},
    {-0.298980f, 0.206247f, 0.653371f, -0.060543f, -0.832873f, 0.370763f, -0.184044f, 0.012163f}
};
const float b2[8] = {0.000000f, 0.000000f, 0.152789f, -0.037715f, -0.133368f, 0.000000f, -0.041670f, 0.120104f};

// Pesos e Vieses da Camada 3 (Saida)
const float w3[4][8] = {
    {0.366966f, 0.198902f, 0.586931f, -0.410597f, 0.038930f, 0.604289f, 0.419304f, -0.193074f},
    {0.015746f, -0.073575f, 0.232543f, -0.392390f, 0.326252f, -0.245261f, 0.000500f, 0.172258f},
    {0.665791f, -0.059270f, -0.189391f, -0.463055f, -0.297970f, 0.255707f, -0.590963f, 0.705378f},
    {0.476444f, 0.180754f, 0.086501f, -0.000007f, 0.730820f, 0.609488f, 0.132705f, 0.406121f}
};
const float b3[4] = {0.111818f, 0.096836f, 0.116615f, 0.000295f};

static float relu(float x) {
    return (x > 0.0f) ? x : 0.0f;
}

void prever_bioprocesso(float dia, float ph, float temp, float turbidez, float co2, float* resultados_saida) {
    float entradas[5] = {dia, ph, temp, turbidez, co2};
    float oculta1[8] = {0};
    float oculta2[8] = {0};

    // 1. Normalizacao
    for (int i = 0; i < 5; i++) {
        entradas[i] = (entradas[i] - input_min[i]) / input_scale[i];
    }

    // 2. Camada 1
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 5; j++) {
            oculta1[i] += entradas[j] * w1[i][j];
        }
        oculta1[i] = relu(oculta1[i] + b1[i]);
    }

    // 3. Camada 2
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            oculta2[i] += oculta1[j] * w2[i][j];
        }
        oculta2[i] = relu(oculta2[i] + b2[i]);
    }

    // 4. Camada de Saida
    for (int i = 0; i < 4; i++) {
        float soma = 0.0f;
        for (int j = 0; j < 8; j++) {
            soma += oculta2[j] * w3[i][j];
        }
        soma += b3[i];
        // Desnormalizacao
        resultados_saida[i] = (soma * output_scale[i]) + output_min[i];
    }
}
