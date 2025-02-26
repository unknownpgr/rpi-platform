#include <encoder.h>
#include <dev.h>

#define ENCODER_L_A 19
#define ENCODER_L_B 20
#define ENCODER_R_A 21
#define ENCODER_R_B 22

static uint8_t prev_l = 0;
static uint8_t prev_r = 0;

static uint32_t count_l = 0;
static uint32_t count_r = 0;

static int diff_dict[16] = {
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

static int rotary_encoder_update(uint8_t last, uint8_t cur)
{
    uint8_t diff = (last << 2) | cur;
    return diff_dict[diff];
}

bool encoder_init()
{
    dev_gpio_set_mode(ENCODER_L_A, GPIO_FSEL_IN);
    dev_gpio_set_mode(ENCODER_L_B, GPIO_FSEL_IN);
    dev_gpio_set_mode(ENCODER_R_A, GPIO_FSEL_IN);
    dev_gpio_set_mode(ENCODER_R_B, GPIO_FSEL_IN);
    return true;
}

void encoder_update()
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
    count_l += diff_l;
    count_r += diff_r;
}

void encoder_get_counts(uint32_t *left, uint32_t *right)
{
    *left = count_l;
    *right = count_r;
}