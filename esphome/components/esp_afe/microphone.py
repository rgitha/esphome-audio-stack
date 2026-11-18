"""esp_afe microphone platform: one raw_data channel of Espressif's AFE fetch result
as a standard ESPHome microphone (16 kHz, 16-bit, mono).

With two mics and BSS, raw_data holds 3 channels: 0 and 1 are the two separated
(BSS) streams and 2 is the unseparated stream that the AFE outputs (what the
esp_audio_stack microphone delivers). The stack's Mic Gain trim is applied, as on
the normal output.
"""

import esphome.codegen as cg
from esphome.components import audio, microphone
from esphome.components.esp_audio_stack import CONF_ESP_AUDIO_STACK_ID, ESPAudioStack
import esphome.config_validation as cv
from esphome.const import CONF_ID

from . import CONF_ESP_AFE_ID, EspAfe, esp_afe_ns

DEPENDENCIES = ["esp_afe", "esp_audio_stack"]

CONF_RAW_CHANNEL = "raw_channel"
CONF_PIPELINE_MICROPHONE = "pipeline_microphone"

AFE_OUTPUT_RATE = 16000  # the AFE runs at 16 kHz (esp_audio_stack output_sample_rate)

EspAfeRawMicrophone = esp_afe_ns.class_("EspAfeRawMicrophone", microphone.Microphone, cg.Component)


def _set_stream_limits(config):
    audio.set_stream_limits(
        min_bits_per_sample=16,
        max_bits_per_sample=16,
        min_channels=1,
        max_channels=1,
        min_sample_rate=AFE_OUTPUT_RATE,
        max_sample_rate=AFE_OUTPUT_RATE,
    )(config)
    return config


CONFIG_SCHEMA = cv.All(
    microphone.MICROPHONE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(EspAfeRawMicrophone),
            cv.GenerateID(CONF_ESP_AFE_ID): cv.use_id(EspAfe),
            cv.GenerateID(CONF_ESP_AUDIO_STACK_ID): cv.use_id(ESPAudioStack),
            cv.Required(CONF_RAW_CHANNEL): cv.int_range(min=0, max=3),
            # The esp_audio_stack microphone: started while this one runs, so the
            # audio stack and the AFE keep running.
            cv.Required(CONF_PIPELINE_MICROPHONE): cv.use_id(microphone.Microphone),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _set_stream_limits,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await microphone.register_microphone(var, config)
    cg.add(var.set_afe(await cg.get_variable(config[CONF_ESP_AFE_ID])))
    cg.add(var.set_stack(await cg.get_variable(config[CONF_ESP_AUDIO_STACK_ID])))
    cg.add(var.set_pipeline_microphone(await cg.get_variable(config[CONF_PIPELINE_MICROPHONE])))
    cg.add(var.set_raw_channel(config[CONF_RAW_CHANNEL]))
