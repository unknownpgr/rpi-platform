#pragma once
#include <stdint.h>

#define KEY_NONE 0x00
#define KEY_CTRL 0x03 // 3

#define KEY_ENTER 0x0A // 10
#define KEY_ESC 0x1B   // 27
#define KEY_SPACE 0x20 // 32

#define KEY_0 0x30 // 48
#define KEY_1 0x31
#define KEY_2 0x32
#define KEY_3 0x33
#define KEY_4 0x34
#define KEY_5 0x35
#define KEY_6 0x36
#define KEY_7 0x37
#define KEY_8 0x38
#define KEY_9 0x39 // 57

#define KEY_UP 0x41 // 65
#define KEY_DOWN 0x42
#define KEY_RIGHT 0x43
#define KEY_LEFT 0x44 // 68

#define KEY_A 0x61 // 97
#define KEY_B 0x62
#define KEY_C 0x63
#define KEY_D 0x64
#define KEY_E 0x65
#define KEY_F 0x66
#define KEY_G 0x67
#define KEY_H 0x68
#define KEY_I 0x69
#define KEY_J 0x6A
#define KEY_K 0x6B
#define KEY_L 0x6C
#define KEY_M 0x6D
#define KEY_N 0x6E
#define KEY_O 0x6F
#define KEY_P 0x70
#define KEY_Q 0x71
#define KEY_R 0x72
#define KEY_S 0x73
#define KEY_T 0x74
#define KEY_U 0x75
#define KEY_V 0x76
#define KEY_W 0x77
#define KEY_X 0x78
#define KEY_Y 0x79
#define KEY_Z 0x7A // 122

#define KEY_BACKSPACE 0x7F // 127

void keyboard_init();
uint16_t keyboard_get_key();
void keyboard_wait_any_key();