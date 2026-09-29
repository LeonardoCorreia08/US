#include "preprocessamento.h"

/* Parâmetros obtidos do StandardScaler utilizado durante o treinamento do modelo. */

static const float medias[6] = {
    -0.0266507937f,
    -0.0232698413f,
     0.9951904762f,
    -0.8198253968f,
    -1.0233809524f,
     0.2413809524f
};

static const float escalas[6] = {
     1.4866662573f,
     1.5059434970f,
     0.0049963706f,
    83.5675569584f,
    84.7429307413f,
    40.7634388961f
};

/* Parâmetros de quantização INT8 obtidos do modelo TensorFlow Lite. */
static const float INPUT_SCALE = 0.0190876536f;
static const int INPUT_ZERO_POINT = 1;

void preprocessar_dados(
    const float dados[6],
    int8_t resultado[6]
)
{
    for (int i = 0; i < 6; i++)
    {
        /* 1. StandardScaler  */
        float normalizado =
            (dados[i] - medias[i]) / escalas[i];

        /* 2. Quantização para INT8       */
        float quantizado =
            (normalizado / INPUT_SCALE)
            + INPUT_ZERO_POINT;

        /* Arredondamento. */
        int valor = (int)(quantizado + 0.5f);

        /*Limites do INT8. */
        if (valor > 127)
        {
            valor = 127;
        }

        if (valor < -128)
        {
            valor = -128;
        }

        resultado[i] = (int8_t)valor;
    }
}

