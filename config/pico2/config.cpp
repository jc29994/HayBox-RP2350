#include "comms/backend_init.hpp"
#include "config_defaults.hpp"
#include "core/CommunicationBackend.hpp"
#include "core/KeyboardMode.hpp"
#include "core/Persistence.hpp"
#include "core/mode_selection.hpp"
#include "core/pinout.hpp"
#include "core/state.hpp"
#include "input/DebouncedGpioButtonInput.hpp"
// #include "input/NunchukInput.hpp"
#include "reboot.hpp"
#include "stdlib.hpp"

#include "boards/pico2.h"

#include <config.pb.h>

Config config = default_config;

GpioButtonMapping button_mappings[] = {
    { BTN_LF1, 6  },
    { BTN_LF2, 4  },
    { BTN_LF3, 3  },
    { BTN_LF4, 2  },
    { BTN_LF5, 5  },

    { BTN_LT1, 28 },
    { BTN_LT2, 27 },
    { BTN_LT3, 22 },
    { BTN_LT4, 21 },

    { BTN_MB1, 7  },

    { BTN_RT1, 19 },
    { BTN_RT2, 17 },
    { BTN_RT3, 18 },
    { BTN_RT4, 16 },
    { BTN_RT5, 20 },

    { BTN_RF1, 12 },
    { BTN_RF2, 13 },
    { BTN_RF3, 14 },
    { BTN_RF4, 15 },

    { BTN_RF5, 8  },
    { BTN_RF6, 9  },
    { BTN_RF7, 10 },
    { BTN_RF8, 11 },
};
const size_t button_count = sizeof(button_mappings) / sizeof(GpioButtonMapping);

DebouncedGpioButtonInput<button_count> gpio_input(button_mappings);

const Pinout pinout = {
    .joybus_data = 28,
    .nes_data = -1,
    .nes_clock = -1,
    .nes_latch = -1,
    .mux = -1,
    .nunchuk_detect = -1,
    .nunchuk_sda = -1,
    .nunchuk_scl = -1,
};

CommunicationBackend **backends = nullptr;
size_t backend_count;
KeyboardMode *current_kb_mode = nullptr;

void setup() {
    static InputState inputs;

    // Create GPIO input source and use it to read button states for checking button holds.
    gpio_input.UpdateInputs(inputs);

    // Check bootsel button hold as early as possible for safety.
    if (inputs.rt2) {
        reboot_bootloader();
    }

    // Turn on LED to indicate firmware booted.
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    // gpio_put(PICO_DEFAULT_LED_PIN, 1);

    // Attempt to load config, or write default config to flash if failed to load config.
    // if (!persistence.LoadConfig(config)) {
        // persistence.SaveConfig(config);
    // }

    // Create array of input sources to be used.
    static InputSource *input_sources[] = {};
    size_t input_source_count = sizeof(input_sources) / sizeof(InputSource *);

    backend_count =
        initialize_backends(backends, inputs, input_sources, input_source_count, config, pinout);

    setup_mode_activation_bindings(config.game_mode_configs, config.game_mode_configs_count);
}

void loop() {
    select_mode(backends, backend_count, config);

    for (size_t i = 0; i < backend_count; i++) {
        backends[i]->SendReport();
    }

    if (current_kb_mode != nullptr) {
        current_kb_mode->SendReport(backends[0]->GetInputs());
    }
}

/* Button inputs are read from the second core */

void setup1() {
    while (backends == nullptr) {
        tight_loop_contents();
    }
}

void loop1() {
    if (backends != nullptr) {
        gpio_input.UpdateInputs(backends[0]->GetInputs());
        gpio_put(PICO_DEFAULT_LED_PIN, backends[0]->GetInputs().mb1);
    }
}
