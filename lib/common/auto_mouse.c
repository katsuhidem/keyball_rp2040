/*
Copyright 2022 @Yowkees
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

https://zenn.dev/takashicompany/articles/69b87160cda4b9
*/

#include QMK_KEYBOARD_H
#include "auto_mouse.h"
#include "lib/keyball/keyball.h"

enum click_state state;     // 現在のクリック入力受付の状態 Current click input reception status
uint16_t click_timer;       // タイマー。状態に応じて時間で判定する。 Timer. Time to determine the state of the system.
uint16_t move_timer;

uint16_t to_reset_time = 2000; // この秒数(千分の一秒)、CLICKABLE状態ならクリックレイヤーが無効になる。 For this number of seconds (milliseconds), the click layer is disabled if in CLICKABLE state.

const uint16_t click_layer = 5;   // マウス入力が可能になった際に有効になるレイヤー。Layers enabled when mouse input is enabled

int16_t scroll_v_mouse_interval_counter;   // 垂直スクロールの入力をカウントする。　Counting Vertical Scroll Inputs
int16_t scroll_h_mouse_interval_counter;   // 水平スクロールの入力をカウントする。  Counts horizontal scrolling inputs.

int16_t scroll_v_threshold = 50;    // この閾値を超える度に垂直スクロールが実行される。 Vertical scrolling is performed each time this threshold is exceeded.
int16_t scroll_h_threshold = 50;    // この閾値を超える度に水平スクロールが実行される。 Each time this threshold is exceeded, horizontal scrolling is performed.

int16_t after_click_lock_movement = 0;      // クリック入力後の移動量を測定する変数。 Variable that measures the amount of movement after a click input.

int16_t mouse_record_threshold = 30;    // ポインターの動きを一時的に記録するフレーム数。 Number of frames in which the pointer movement is temporarily recorded.
int16_t mouse_move_count_ratio = 5;     // ポインターの動きを再生する際の移動フレームの係数。 The coefficient of the moving frame when replaying the pointer movement.

int16_t mouse_movement;

// クリック用のレイヤーを有効にする。　Enable layers for clicks
void enable_click_layer(void) {
  layer_on(click_layer);
  click_timer = timer_read();
  state = CLICKABLE;
}

// クリック用のレイヤーを無効にする。 Disable layers for clicks.
void disable_click_layer(void) {
  state = NONE;
  layer_off(click_layer);
  scroll_v_mouse_interval_counter = 0;
  scroll_h_mouse_interval_counter = 0;
}

// 自前の絶対数を返す関数。 Functions that return absolute numbers.
int16_t my_abs(int16_t num) {
  if (num < 0) {
    num = -num;
  }

  return num;
}

// 自前の符号を返す関数。 Function to return the sign.
int16_t mmouse_move_y_sign(int16_t num) {
  if (num < 0) {
    return -1;
  }

  return 1;
}

// 現在クリックが可能な状態か。 Is it currently clickable?
bool is_clickable_mode(void) {
  return state == CLICKABLE || state == CLICKING || state == SCROLLING;
}

bool auto_mouse_process_record_kb(uint16_t keycode, keyrecord_t *record) {
  switch (keycode) {
    case KC_MS_BTN1:
    case KC_MS_BTN2:
    case KC_MS_BTN3:
    {
      report_mouse_t currentReport = pointing_device_get_report();

      // どこのビットを対象にするか。 Which bits are to be targeted?
      uint8_t btn = 1 << (keycode - KC_MS_BTN1);

      if (record->event.pressed) {
        // ビットORは演算子の左辺と右辺の同じ位置にあるビットを比較して、両方のビットのどちらかが「1」の場合に「1」にします。
        // Bit OR compares bits in the same position on the left and right sides of the operator and sets them to "1" if either of both bits is "1".
        currentReport.buttons |= btn;
        state = CLICKING;
        after_click_lock_movement = 30;
      } else {
        // ビットANDは演算子の左辺と右辺の同じ位置にあるビットを比較して、両方のビットが共に「1」の場合だけ「1」にします。
        // Bit AND compares the bits in the same position on the left and right sides of the operator and sets them to "1" only if both bits are "1" together.
        currentReport.buttons &= ~btn;
        enable_click_layer();
      }

      pointing_device_set_report(currentReport);
      pointing_device_send();
      return false;
    }

    case KC_MY_SCR:
      if (record->event.pressed) {
        state = SCROLLING;
      } else {
        enable_click_layer();   // スクロールキーを離した時に再度クリックレイヤーを有効にする。 Enable click layer again when the scroll key is released.
      }
      return false;
    
    case KC_TO_CLICKABLE_INC:
      if (record->event.pressed) {
        keyball_config.to_clickable_movement += 5;
      }
      return false;

    case KC_TO_CLICKABLE_DEC:
      if (record->event.pressed) {

        keyball_config.to_clickable_movement -= 5;

        if (keyball_config.to_clickable_movement < 5) {
            keyball_config.to_clickable_movement = 5;
        }
      }
      return false;

    default:
      if (record->event.pressed) {
        disable_click_layer();
      }
  
  }

  return true;
}

report_mouse_t auto_mouse_pointing_device_task_kb(report_mouse_t mouse_report) {
  int16_t current_x = mouse_report.x;
  int16_t current_y = mouse_report.y;
  int16_t current_h = 0;
  int16_t current_v = 0;

  if (current_x != 0 || current_y != 0) {
    switch (state) {
        case CLICKABLE:
            click_timer = timer_read();
            break;

        case CLICKING:
            after_click_lock_movement -= my_abs(current_x) + my_abs(current_y);

            if (after_click_lock_movement > 0) {
                current_x = 0;
                current_y = 0;
            }

            break;

        case SCROLLING:
          {
            int8_t rep_v = 0;
            int8_t rep_h = 0;

            // 垂直スクロールの方の感度を高める。 Increase sensitivity toward vertical scrolling.
            if (my_abs(current_y) * 2 > my_abs(current_x)) {

              scroll_v_mouse_interval_counter += current_y;
              while (my_abs(scroll_v_mouse_interval_counter) > scroll_v_threshold) {
                if (scroll_v_mouse_interval_counter < 0) {
                  scroll_v_mouse_interval_counter += scroll_v_threshold;
                  rep_v -= scroll_v_threshold;
                } else {
                  scroll_v_mouse_interval_counter -= scroll_v_threshold;
                  rep_v += scroll_v_threshold;
                }

              }
            } else {

              scroll_h_mouse_interval_counter += current_x;

              while (my_abs(scroll_h_mouse_interval_counter) > scroll_h_threshold) {
                if (scroll_h_mouse_interval_counter < 0) {
                  scroll_h_mouse_interval_counter += scroll_h_threshold;
                  rep_h += scroll_h_threshold;
                } else {
                  scroll_h_mouse_interval_counter -= scroll_h_threshold;
                  rep_h -= scroll_h_threshold;
                }
              }
            }

            current_h = rep_h / scroll_h_threshold;
            current_v = -rep_v / scroll_v_threshold;
            current_x = 0;
            current_y = 0;
          }
          break;

        case WAITING:

          mouse_movement += my_abs(current_x) + my_abs(current_y);

          if (mouse_movement >= keyball_config.to_clickable_movement)
          {
            mouse_movement = 0;
            enable_click_layer();
          }
          break;

        default:
          click_timer = timer_read();
          move_timer = timer_read();
          state = WAITING;
          mouse_movement = 0;
    }
  } else {
    switch (state) {
      case CLICKING:
      case SCROLLING:

        break;

      case CLICKABLE:
        if (timer_elapsed(click_timer) > to_reset_time) {
          disable_click_layer();
        }
        break;

      case WAITING:
        if (timer_elapsed(click_timer) > 50) {
          mouse_movement = 0;
          state = NONE;
        }
        break;

      default:
        mouse_movement = 0;
        state = NONE;
    }
  }

  mouse_report.x = current_x;
  mouse_report.y = current_y;
  mouse_report.h = current_h;
  mouse_report.v = current_v;

  return mouse_report;
}

void set_scroll_mode(bool mode) {
  if (mode) {
    state = SCROLLING;
  } else if (state == SCROLLING) {
    state = NONE;
  }
}

void oledkit_render_auto_mouse(void) {
    oled_write_P(PSTR(" MV:"), false);
    oled_write(get_u8_str(mouse_movement, ' '), false);
    oled_write_P(PSTR("/"), false);
    oled_write(get_u8_str(keyball_config.to_clickable_movement, ' '), false);
}