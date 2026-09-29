#ifndef TFLITE_RUNNER_H
#define TFLITE_RUNNER_H

#ifdef __cplusplus
extern "C" {
#endif

int model_init(void);
int model_run(float input, float *output);

#ifdef __cplusplus
}
#endif

#endif