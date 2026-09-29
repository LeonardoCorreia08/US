#ifndef TFLITE_RUNNER_H
#define TFLITE_RUNNER_H

#ifdef __cplusplus
extern "C" {
#endif

// Init o modelo e o interpretador TFLite
int model_init(void);

// Exec a infer passando a entrada e recebendo a saída
int model_run(float input, float *output);

#ifdef __cplusplus
}
#endif

#endif