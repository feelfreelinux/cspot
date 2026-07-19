// UI LVGL 9: temperatura/humedad grandes arriba, barra Spotify abajo.

#include "ui.h"

#include <stdio.h>
#include <string.h>

#include "esp_lvgl_port.h"
#include "lvgl.h"

static lv_obj_t* lbl_temp;
static lv_obj_t* lbl_hum;
static lv_obj_t* lbl_title;
static lv_obj_t* lbl_artist;
static lv_obj_t* lbl_status;
static lv_obj_t* btn_play_label;

static ui_control_cb_t control_cb = NULL;
static void* control_user = NULL;

static void btn_event_cb(lv_event_t* e) {
  ui_command_t cmd = (ui_command_t)(uintptr_t)lv_event_get_user_data(e);
  if (control_cb) {
    control_cb(cmd, control_user);
  }
}

static lv_obj_t* make_btn(lv_obj_t* parent, const char* symbol,
                          ui_command_t cmd) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_set_size(btn, 56, 44);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x2a2a2a), 0);
  lv_obj_set_style_radius(btn, 8, 0);
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED,
                      (void*)(uintptr_t)cmd);
  lv_obj_t* lbl = lv_label_create(btn);
  lv_label_set_text(lbl, symbol);
  lv_obj_center(lbl);
  return btn;
}

void ui_init(void) {
  lvgl_port_lock(0);

  lv_obj_t* scr = lv_screen_active();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  // --- Zona sensor ---
  lbl_temp = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(lbl_temp, lv_color_hex(0xffffff), 0);
  lv_label_set_text(lbl_temp, "--.-\xC2\xB0""C");
  lv_obj_align(lbl_temp, LV_ALIGN_TOP_MID, 0, 24);

  lbl_hum = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_hum, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(lbl_hum, lv_color_hex(0x7ec8e3), 0);
  lv_label_set_text(lbl_hum, LV_SYMBOL_TINT " --%");
  lv_obj_align(lbl_hum, LV_ALIGN_TOP_MID, 0, 84);

  lbl_status = lv_label_create(scr);
  lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x808080), 0);
  lv_label_set_text(lbl_status, "Esperando Spotify...");
  lv_obj_align(lbl_status, LV_ALIGN_TOP_MID, 0, 122);

  // --- Barra Spotify ---
  lv_obj_t* bar = lv_obj_create(scr);
  lv_obj_set_size(bar, 320, 96);
  lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x181c22), 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_set_style_pad_all(bar, 6, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

  lbl_title = lv_label_create(bar);
  lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(lbl_title, lv_color_hex(0xffffff), 0);
  lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(lbl_title, 308);
  lv_label_set_text(lbl_title, "-");
  lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 0, 0);

  lbl_artist = lv_label_create(bar);
  lv_obj_set_style_text_font(lbl_artist, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(lbl_artist, lv_color_hex(0x1db954), 0);
  lv_label_set_long_mode(lbl_artist, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lbl_artist, 150);
  lv_label_set_text(lbl_artist, "-");
  lv_obj_align(lbl_artist, LV_ALIGN_LEFT_MID, 0, 8);

  // Botones prev / play-pausa / next
  lv_obj_t* btn_prev = make_btn(bar, LV_SYMBOL_PREV, UI_CMD_PREV);
  lv_obj_align(btn_prev, LV_ALIGN_BOTTOM_RIGHT, -128, 0);

  lv_obj_t* btn_play = make_btn(bar, LV_SYMBOL_PLAY, UI_CMD_PLAY_PAUSE);
  lv_obj_align(btn_play, LV_ALIGN_BOTTOM_RIGHT, -64, 0);
  btn_play_label = lv_obj_get_child(btn_play, 0);

  lv_obj_t* btn_next = make_btn(bar, LV_SYMBOL_NEXT, UI_CMD_NEXT);
  lv_obj_align(btn_next, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

  lvgl_port_unlock();
}

void ui_set_control_cb(ui_control_cb_t cb, void* user) {
  control_cb = cb;
  control_user = user;
}

void ui_set_env(float temp_c, float hum_pct) {
  if (!lbl_temp) {
    return;
  }
  lvgl_port_lock(0);
  lv_label_set_text_fmt(lbl_temp, "%.1f\xC2\xB0""C", (double)temp_c);
  lv_label_set_text_fmt(lbl_hum, LV_SYMBOL_TINT " %.0f%%", (double)hum_pct);
  lvgl_port_unlock();
}

void ui_set_track(const char* title, const char* artist) {
  if (!lbl_title) {
    return;
  }
  lvgl_port_lock(0);
  lv_label_set_text(lbl_title, title && title[0] ? title : "-");
  lv_label_set_text(lbl_artist, artist && artist[0] ? artist : "-");
  lvgl_port_unlock();
}

void ui_set_playing(bool playing) {
  if (!btn_play_label) {
    return;
  }
  lvgl_port_lock(0);
  lv_label_set_text(btn_play_label, playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
  lvgl_port_unlock();
}

void ui_set_status(const char* status) {
  if (!lbl_status) {
    return;
  }
  lvgl_port_lock(0);
  lv_label_set_text(lbl_status, status);
  lvgl_port_unlock();
}
