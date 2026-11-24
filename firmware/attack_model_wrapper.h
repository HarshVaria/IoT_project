#ifndef ATTACK_MODEL_WRAPPER_H
#define ATTACK_MODEL_WRAPPER_H

#include <stdint.h>

// Wrapper function to predict using the emlearn model
// This prevents the massive model header from being recompiled every time the .ino changes.
int32_t attack_model_predict_wrapper(const float *features, int32_t features_length);

#endif
