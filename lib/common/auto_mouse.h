enum click_state {
    NONE = 0,
    WAITING,    // マウスレイヤーが有効になるのを待つ。 Wait for mouse layer to activate.
    CLICKABLE,  // マウスレイヤー有効になりクリック入力が取れる。 Mouse layer is enabled to take click input.
    CLICKING,   // クリック中。 Clicking.
    SCROLLING   // スクロール中。 Scrolling.
};

bool auto_mouse_process_record_kb(uint16_t keycode, keyrecord_t *record);

report_mouse_t auto_mouse_pointing_device_task_kb(report_mouse_t mouse_report);

void set_scroll_mode(bool mode);

void oledkit_render_auto_mouse(void);