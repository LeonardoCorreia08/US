#include <cstdint>
#include "tflite_runner.h"
#include "hello_world_int8.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

namespace {
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input_tensor = nullptr;
    TfLiteTensor* output_tensor = nullptr;

    // Tamanho da arena de tensores 
    constexpr int kTensorArenaSize = 4 * 1024;
    alignas(16) uint8_t tensor_arena[kTensorArenaSize];

    // Persistência do interpretador e do resolver na memória estática
    tflite::MicroMutableOpResolver<2> resolver;
}

extern "C" {

int model_init(void) {
    
    model = tflite::GetModel(g_hello_world_int8);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        return -1;
    }

    // Registra as operações d modelo hello_world 
    resolver.AddFullyConnected();
    resolver.AddRelu();

    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        return -2;
    }

    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);

    return 0;
}

int model_run(float input, float *output) {
    if (!interpreter || !input_tensor || !output_tensor || !output) {
        return -1;
    }

    // Quantização de float para int8
    int32_t zero_point_in = input_tensor->params.zero_point;
    float scale_in = input_tensor->params.scale;
    int8_t x_quantized = static_cast<int8_t>(input / scale_in + zero_point_in);

    input_tensor->data.int8[0] = x_quantized;

    // Exec da inferência
    if (interpreter->Invoke() != kTfLiteOk) {
        return -2;
    }

    // int8 para float
    int8_t y_quantized = output_tensor->data.int8[0];
    int32_t zero_point_out = output_tensor->params.zero_point;
    float scale_out = output_tensor->params.scale;

    *output = (static_cast<float>(y_quantized) - zero_point_out) * scale_out;

    return 0;
}

} 