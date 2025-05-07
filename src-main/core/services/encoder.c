#include <services/encoder.h>

#include <stdint.h>

#include <state.h>
#include <ports/dev.h>
#include <ports/timer.h>

#define ENCODER_L_A 19
#define ENCODER_L_B 20
#define ENCODER_R_A 21
#define ENCODER_R_B 22

static loop_t loop_encoder;

static uint8_t prev_l = 0;
static uint8_t prev_r = 0;

static const int diff_dict[16] = {
    0,  // 00 -> 00 (stay)
    1,  // 00 -> 01
    -1, // 00 -> 10
    0,  // 00 -> 11 (invalid)
    -1, // 01 -> 00
    0,  // 01 -> 01 (stay)
    0,  // 01 -> 10 (invalid)
    1,  // 01 -> 11
    1,  // 10 -> 00
    0,  // 10 -> 01 (invalid)
    0,  // 10 -> 10 (stay)
    -1, // 10 -> 11
    0,  // 11 -> 00 (invalid)
    -1, // 11 -> 01
    1,  // 11 -> 10
    0,  // 11 -> 11 (stay)
};

static inline int rotary_encoder_update(uint8_t last, uint8_t cur)
{
    uint8_t diff = (last << 2) | cur;
    return diff_dict[diff];
}

static void encoder_setup()
{
    dev_gpio_set_mode(ENCODER_L_A, GPIO_FSEL_IN);
    dev_gpio_set_mode(ENCODER_L_B, GPIO_FSEL_IN);
    dev_gpio_set_mode(ENCODER_R_A, GPIO_FSEL_IN);
    dev_gpio_set_mode(ENCODER_R_B, GPIO_FSEL_IN);

    state->encoder_left = 0;
    state->encoder_right = 0;

    loop_init(&loop_encoder, 1000); // 1us = 1MHz
}

static void encoder_loop()
{
    uint32_t dt_ns;
    if (loop_update(&loop_encoder, &dt_ns))
    {
        bool la = dev_gpio_get_pin(ENCODER_L_A);
        bool lb = dev_gpio_get_pin(ENCODER_L_B);
        bool ra = dev_gpio_get_pin(ENCODER_R_A);
        bool rb = dev_gpio_get_pin(ENCODER_R_B);

        uint8_t cur_l = (la << 1) | lb;
        uint8_t cur_r = (ra << 1) | rb;

        int diff_l = rotary_encoder_update(prev_l, cur_l);
        int diff_r = rotary_encoder_update(prev_r, cur_r);

        prev_l = cur_l;
        prev_r = cur_r;

        state->encoder_left += diff_l;
        state->encoder_right += diff_r;
    }
}

em_service_t service_encoder = {
    .state_mask = EM_STATE_ALL,
    .setup = encoder_setup,
    .loop = encoder_loop,
    .teardown = NULL,
};
