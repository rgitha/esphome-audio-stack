#include "esp_afe_microphone.h"

#ifdef USE_ESP32

#include "esphome/core/log.h"
#include "../esp_audio_stack/esp_audio_stack.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace esphome::esp_afe {

static const char *const TAG = "esp_afe.raw_mic";

void EspAfeRawMicrophone::setup() {
  this->audio_stream_info_ = audio::AudioStreamInfo(16, 1, 16000);
  this->buffer_.reserve(MAX_FRAME_BYTES);
  if (!this->afe_->add_result_tap(&EspAfeRawMicrophone::tap_cb_, this)) {
    ESP_LOGE(TAG, "No free AFE result tap for raw channel %d", this->raw_channel_);
    this->mark_failed();
  }
}

void EspAfeRawMicrophone::dump_config() {
  ESP_LOGCONFIG(TAG,
                "AFE raw-stream microphone:\n"
                "  raw_data channel: %d\n"
                "  16000 Hz, 16 bits, mono; Mic Gain trim applied",
                this->raw_channel_);
}

void EspAfeRawMicrophone::start() {
  if (this->is_failed())
    return;
  if (this->listeners_++ == 0) {
    this->pipeline_mic_->start();  // keeps the audio stack and the AFE running
    this->state_ = microphone::STATE_RUNNING;
    this->running_.store(true, std::memory_order_release);
  }
}

void EspAfeRawMicrophone::stop() {
  if (this->listeners_ == 0)
    return;
  if (--this->listeners_ == 0) {
    this->running_.store(false, std::memory_order_release);
    this->state_ = microphone::STATE_STOPPED;
    this->pipeline_mic_->stop();
  }
}

void EspAfeRawMicrophone::loop() {
  const uint32_t missing = this->missing_frames_.exchange(0, std::memory_order_relaxed);
  if (missing > 0)
    ESP_LOGW(TAG, "raw channel %d: %u frames without it (raw_data missing or too few channels)", this->raw_channel_,
             static_cast<unsigned>(missing));
}

void EspAfeRawMicrophone::tap_cb_(const EspAfe::ResultTap &tap, void *ctx) {
  static_cast<EspAfeRawMicrophone *>(ctx)->on_tap_(tap);
}

void EspAfeRawMicrophone::on_tap_(const EspAfe::ResultTap &tap) {
  if (!this->running_.load(std::memory_order_acquire))
    return;
  const int ch = tap.raw_data_channels;
  if (tap.raw_data == nullptr || ch <= this->raw_channel_) {
    this->missing_frames_.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  const size_t n = static_cast<size_t>(tap.samples);
  const size_t bytes = n * sizeof(int16_t);
  if (bytes > this->buffer_.capacity()) {
    this->missing_frames_.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  this->buffer_.resize(bytes);
  auto *out = reinterpret_cast<int16_t *>(this->buffer_.data());
  const float gain = this->stack_->get_mic_gain();  // the Mic Gain trim, as on the normal output
  const int16_t *in = tap.raw_data + this->raw_channel_;
  for (size_t i = 0; i < n; i++) {
    const float v = static_cast<float>(in[i * ch]) * gain;
    out[i] = static_cast<int16_t>(std::clamp(std::lrintf(v), -32768L, 32767L));
  }
  this->data_callbacks_.call(this->buffer_);
}

}  // namespace esphome::esp_afe

#endif  // USE_ESP32
