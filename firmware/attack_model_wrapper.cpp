#include "attack_model_wrapper.h"

// By putting this pragma here, we tell the compiler not to aggressively optimize this massive file,
// which drops compile times for this file from several minutes to just a couple of seconds!
#pragma GCC optimize ("O0")

#include "attack_model.h"

int32_t attack_model_predict_wrapper(const float *features, int32_t features_length) {
    return attack_model_predict(features, features_length);
}
