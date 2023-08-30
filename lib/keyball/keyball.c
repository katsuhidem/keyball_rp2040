/*
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "quantum.h"
#ifdef SPLIT_KEYBOARD
#    include "transactions.h"
#endif

#include "keyball.h"
#include "lib/common/auto_mouse.h"
#include "lib/common/twpair_on_jis.h"

const uint8_t CPI_DEFAULT    = KEYBALL_CPI_DEFAULT / 100;
const uint16_t CPI_MAX       = 16000;
const uint8_t SCROLL_DIV_MAX = 7;

keyball_config_t keyball_config = {};

keyball_t keyball = {
    .this_have_ball = false,
    .that_enable    = false,
    .that_have_ball = false,

    .this_motion = {0},
    .that_motion = {0},

    .cpi_value   = 0,
    .cpi_changed = false,

    .scroll_mode = false,
};

//////////////////////////////////////////////////////////////////////////////
// Static utilities

static const char *format_4d(uint16_t d) {
    static char buf[5] = {0}; // max width (4) + NUL (1)
    char        lead   = ' ';
    if (d < 0) {
        d    = -d;
        lead = '-';
    }
    buf[3] = (d % 10) + '0';
    d /= 10;
    if (d == 0) {
        buf[2] = lead;
        lead   = ' ';
    } else {
        buf[2] = (d % 10) + '0';
        d /= 10;
    }
    if (d == 0) {
        buf[1] = lead;
        lead   = ' ';
    } else {
        buf[1] = (d % 10) + '0';
        d /= 10;
    }
    buf[0] = lead;
    return buf;
}

static char to_1x(uint8_t x) {
    x &= 0x0f;
    return x < 10 ? x + '0' : x + 'a' - 10;
}

static void add_cpi(int8_t delta) {
    int16_t v = pmw33xx_get_cpi(0) + delta * 100;
    pmw33xx_set_cpi(0, v < 1 ? 1 : v);
}

//////////////////////////////////////////////////////////////////////////////
// OLED utility

#ifdef OLED_ENABLE
// clang-format off
const char PROGMEM code_to_name[] = {
    'a', 'b', 'c', 'd', 'e', 'f',  'g', 'h', 'i',  'j',
    'k', 'l', 'm', 'n', 'o', 'p',  'q', 'r', 's',  't',
    'u', 'v', 'w', 'x', 'y', 'z',  '1', '2', '3',  '4',
    '5', '6', '7', '8', '9', '0',  'R', 'E', 'B',  'T',
    '_', '-', '=', '[', ']', '\\', '#', ';', '\'', '`',
    ',', '.', '/',
};
// clang-format on
#endif

void keyball_oled_render_ballinfo(void) {
#ifdef OLED_ENABLE
    // CPI
    oled_write_P(PSTR("CPI"), false);
    oled_write(format_4d(pmw33xx_get_cpi(0)/100), false);
    oledkit_render_auto_mouse();
    oled_advance_page(true);
#endif
}

void keyball_oled_render_keyinfo(void) {
#ifdef OLED_ENABLE
    // Format: `Key :  R{row}  C{col} K{kc}  '{name}`
    //
    // Where `kc` is lower 8 bit of keycode.
    // Where `name` is readable label for `kc`, valid between 4 and 56.
    //
    // It is aligned to fit with output of keyball_oled_render_ballinfo().
    // For example:
    //
    //     Key :  R2  C3 K06  'c
    //     Ball:   0   0   0   0
    //
    uint8_t keycode = keyball.last_kc;

    oled_write_P(PSTR("Key :  R"), false);
    oled_write_char(to_1x(keyball.last_pos.row), false);
    oled_write_P(PSTR("  C"), false);
    oled_write_char(to_1x(keyball.last_pos.col), false);
    if (keycode) {
        oled_write_P(PSTR(" K"), false);
        oled_write_char(to_1x(keycode >> 4), false);
        oled_write_char(to_1x(keycode), false);
    }
    if (keycode >= 4 && keycode < 57) {
        oled_write_P(PSTR("  '"), false);
        char name = pgm_read_byte(code_to_name + keycode - 4);
        oled_write_char(name, false);
    } else {
        oled_advance_page(true);
    }
    oled_write_P(PSTR("Layer:"), false);
    oled_write(get_u8_str(get_highest_layer(layer_state), ' '), false);
    oled_write_P(PSTR(" Mode:"), false);
    if (keyball_config.jis){
        oled_write_ln("JIS", false);
    } else {
        oled_write_ln(" US", false);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////////
// Public API functions

void keyball_set_scroll_mode(bool mode) {
    set_scroll_mode(mode);
}

uint8_t keyball_get_cpi(void) {
    return keyball.cpi_value == 0 ? CPI_DEFAULT : keyball.cpi_value;
}

void keyball_set_cpi(uint16_t cpi) {
    if (cpi > CPI_MAX) {
        cpi = CPI_MAX;
    }
    keyball.cpi_value   = cpi;
    keyball.cpi_changed = true;
    pmw33xx_set_cpi(0, cpi == 0 ? CPI_DEFAULT : cpi);
}

//////////////////////////////////////////////////////////////////////////////
// Keyboard hooks

void eeconfig_init_kb() {
    keyball_config.cpi = CPI_DEFAULT;
    keyball_config.to_clickable_movement = 20;
    keyball_config.jis = 0;
    eeconfig_update_kb(keyball_config.raw);
    eeconfig_init_user();
}

void keyboard_post_init_kb(void) {
    keyball_config.raw = eeconfig_read_kb();
    keyball_set_cpi(keyball_config.cpi * 100);
    set_keyboard_lang_to_jis(keyball_config.jis);
    keyboard_post_init_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    // store last keycode, row, and col for OLED
    keyball.last_kc  = keycode;
    keyball.last_pos = record->event.key;

    if (!process_record_user(keycode, record)) {
        return false;
    }

    // strip QK_MODS part.
    if (keycode >= QK_MODS && keycode <= QK_MODS_MAX) {
        keycode &= 0xff;
    }

    switch (keycode) {
#ifndef MOUSEKEY_ENABLE
        // process KC_MS_BTN1~8 by myself
        // See process_action() in quantum/action.c for details.
        case KC_MS_BTN1 ... KC_MS_BTN8: {
            extern void register_mouse(uint8_t mouse_keycode, bool pressed);
            register_mouse(keycode, record->event.pressed);
            // to apply QK_MODS actions, allow to process others.
            return true;
        }
#endif
    }

    bool ret = true;

    // process events which works on pressed only.
    if (record->event.pressed) {
        switch (keycode) {
            case KBC_RST:
                keyball_set_cpi(0);
                ret = false;
                break;
            case KBC_SAVE:
                keyball_config.cpi = pmw33xx_get_cpi(0)/100;
                eeconfig_update_kb(keyball_config.raw);
                ret = false;
                break;
            case CPI_I100:
                add_cpi(1);
                ret = false;
                break;
            case CPI_D100:
                add_cpi(-1);
                ret = false;
                break;
            case CPI_I1K:
                add_cpi(10);
                ret = false;
                break;
            case CPI_D1K:
                add_cpi(-10);
                ret = false;
                break;
            case EnJIS:
                set_keyboard_lang_to_jis(true);
                ret = false;
                break;
            case EnUS:
                set_keyboard_lang_to_jis(false);
                ret = false;
                break;

            default:
                if (keyball_config.jis){
                    ret = twpair_on_jis(keycode, record);
                }
                break;
        }
    }

    if (ret) {
        ret = auto_mouse_process_record_kb(keycode, record);
    }

    return ret;
}


report_mouse_t pointing_device_task_kb(report_mouse_t mouse_report) {
    mouse_report = auto_mouse_pointing_device_task_kb(mouse_report);
    mouse_report = pointing_device_task_user(mouse_report);

    return mouse_report;
}