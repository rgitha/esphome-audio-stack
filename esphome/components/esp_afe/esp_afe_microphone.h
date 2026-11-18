#pragma once

#ifdef USE_ESP32

// One raw_data channel of the AFE fetch result as a standard microphone (see
// microphone.py). Data is delivered from the AFE fetch task.

#include "esphome/core/component.h"
#include "esphome/components/microphone/microphone.h"
#include "esp_afe.h"

#include <atomic>
#include <cstdint>
#include <vector>

namespace esphome::esp_audio_stack {
class ESPAudioStack;
}  // namespace esphome::esp_audio_stack

namespace esphome::esp_afe {

class EspAfeRawMicrophone : public microphone::Microphone, public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void start() override;
  void stop() override;

  void set_afe(EspAfe *afe) { this->afe_ = afe; }
  void set_stack(esp_audio_stack::ESPAudioStack *stack) { this->stack_ = stack; }
  void set_pipeline_microphone(microphone::Microphone *mic) { this->pipeline_mic_ = mic; }
  void set_raw_channel(int channel) { this->raw_channel_ = channel; }

 protected:
  static constexpr size_t MAX_FRAME_BYTES = 4096;

  static void tap_cb_(const EspAfe::ResultTap &tap, void *ctx);
  void on_tap_(const EspAfe::ResultTap &tap);

  EspAfe *afe_{nullptr};
  esp_audio_stack::ESPAudioStack *stack_{nullptr};
  microphone::Microphone *pipeline_mic_{nullptr};
  int raw_channel_{0};

  std::vector<uint8_t> buffer_;  // reserved in setup(); used only on the fetch task
  std::atomic<bool> running_{false};
  uint32_t listeners_{0};  // start()/stop() pairs, main loop only
  std::atomic<uint32_t> missing_frames_{0};
};

}  // namespace esphome::esp_afe

#endif  // USE_ESP32
