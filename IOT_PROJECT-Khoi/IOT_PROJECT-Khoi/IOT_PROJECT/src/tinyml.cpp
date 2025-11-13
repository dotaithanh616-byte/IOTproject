#include "tinyml.h"
#include "dht_anomaly_model.h"    // generated header from training step
#include "global.h"               // for glob_temperature, glob_humidity
#include <math.h>
#include <cmath>

// TFLM headers (usually pulled by tinyml.h already; keep here for safety)
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/micro_error_reporter.h"


// === training scaler (from train log) ===
static constexpr float kMeanT = 27.50973814f;
static constexpr float kMeanH = 55.33604769f;
static constexpr float kStdT  = 2.61667653f;
static constexpr float kStdH  = 12.34397559f;

// Standardize like in training
static inline void standardize(float T, float H, float& zT, float& zH) {
  zT = (T - kMeanT) / kStdT;
  zH = (H - kMeanH) / kStdH;
}

// -----------------------------------------------
// Globals (same pattern you had)
// -----------------------------------------------
namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;

  // Bump arena to avoid AllocateTensors() failures on ESP32
  constexpr int kTensorArenaSize = 16 * 1024; // was 8 KB
  static uint8_t tensor_arena[kTensorArenaSize];
} // namespace

// -----------------------------------------------
// INT8 quantization helpers + I/O shims
// -----------------------------------------------
static inline int8_t q8(float x, float scale, int zp) {
  int v = (int)lroundf(x / scale + zp);
  if (v < -128) v = -128;
  if (v > 127)  v = 127;
  return (int8_t)v;
}

static inline float deq8(int8_t q, float scale, int zp) {
  return (q - zp) * scale;
}

static void write_input(TfLiteTensor* in, float temp, float hum) {
  // Expecting shape [1,2]
  if (in->type == kTfLiteInt8) {
    in->data.int8[0] = q8(temp, in->params.scale, in->params.zero_point);
    in->data.int8[1] = q8(hum , in->params.scale, in->params.zero_point);
  } else {
    // Float fallback (if you ever use a float model)
    in->data.f[0] = temp;
    in->data.f[1] = hum;
  }
}

static float read_output_prob(const TfLiteTensor* out) {
  if (out->type == kTfLiteInt8) {
    return deq8(out->data.int8[0], out->params.scale, out->params.zero_point); // ~0..1
  } else {
    return out->data.f[0];
  }
}

// -----------------------------------------------
// Optional: print model I/O once (debug)
// -----------------------------------------------
static void print_model_info(TfLiteTensor* in, TfLiteTensor* out) {
  auto ttype = [](TfLiteType t){
    switch (t) {
      case kTfLiteFloat32: return "F32";
      case kTfLiteInt8:    return "INT8";
      case kTfLiteUInt8:   return "UINT8";
      default:             return "OTHER";
    }
  };
  Serial.println("=== TFLM model info ===");
  Serial.printf("Input : type=%s dims=%d [", ttype(in->type), in->dims->size);
  for (int i=0; i<in->dims->size; ++i) {
    Serial.printf("%d%s", in->dims->data[i], (i+1<in->dims->size)?",":"");
  }
  Serial.printf("] scale=%.6f zp=%d\n", in->params.scale, in->params.zero_point);

  Serial.printf("Output: type=%s dims=%d [", ttype(out->type), out->dims->size);
  for (int i=0; i<out->dims->size; ++i) {
    Serial.printf("%d%s", out->dims->data[i], (i+1<out->dims->size)?",":"");
  }
  Serial.printf("] scale=%.6f zp=%d\n", out->params.scale, out->params.zero_point);
  Serial.println("========================");
}

// -----------------------------------------------
// One-shot setup (keeps your signature)
// -----------------------------------------------
void setupTinyML() {
  Serial.println("TensorFlow Lite Init....");
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(dht_anomaly_model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Model schema %d != supported %d.",
                           model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    error_reporter->Report("AllocateTensors() failed (increase arena?)");
    return;
  }

  input  = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("TensorFlow Lite Micro initialized on ESP32.");
  print_model_info(input, output);

  // Optional quick sanity probes (run once here)
  auto test_once = [&](float T, float H){
    float zT, zH;
    standardize(T, H, zT, zH);        // <<< NEW: scale like training
    write_input(input, zT, zH);       // feed standardized values
    if (interpreter->Invoke()==kTfLiteOk){
      float p = read_output_prob(output);
      Serial.printf("[Sanity] T=%.1f H=%.0f -> p=%.3f (%s)\n",
                    T, H, p, (p>=0.5f)?"ANOMALY":"NORMAL");
    } else {
      Serial.println("[Sanity] Invoke failed");
    }
  };
  test_once(27.5f, 55.0f); // expect NORMAL-ish
  test_once(32.0f, 55.0f); // expect ANOMALY-ish
  test_once(27.5f, 80.0f); // expect ANOMALY-ish
}

// -----------------------------------------------
// FreeRTOS task 
// -----------------------------------------------
void tiny_ml_task(void *pvParameters) {
  setupTinyML();

  // If setup failed, interpreter could be null
  if (!interpreter || !input || !output) {
    Serial.println("[TinyML] Setup failed; exiting task.");
    vTaskDelete(NULL);
    return;
  }

  for (;;) {
    float T, H;

    // Read globals safely (your mutex names)
    if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      T = glob_temperature;
      H = glob_humidity;
      xSemaphoreGive(xDataMutex);
    } else {
      T = -1.0f; H = -1.0f;
    }

    // Validate sample
    bool valid = !isnan(T) && !isnan(H) &&
                 T >= -40.0f && T <= 125.0f &&
                 H >= 0.0f   && H <= 100.0f;

    if (valid) {
      // 🔹 NEW: standardize before feeding to the model
      float zT, zH;
      standardize(T, H, zT, zH);
      write_input(input, zT, zH);

      // Run inference
      if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("[TinyML] Invoke failed");
      } else {
        // Dequantize/read probability
        float p = read_output_prob(output);
        bool anomaly = (p >= 0.5f);

        // Consistent, single-line, non-interleaving print
        Serial.printf("[TinyML] p=%.3f  %s  (T=%.1fC  H=%.0f%%)\n",
                      p, anomaly ? "ANOMALY" : "NORMAL", T, H);

        // Optional CSV for logging
         Serial.printf("%.1f,%.1f,%s\n", T, H, anomaly ? "anomaly" : "normal");
      }
    } else {
      Serial.println("[TinyML] Skipping invalid sensor sample");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
