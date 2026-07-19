#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  UI_CMD_PLAY_PAUSE,
  UI_CMD_NEXT,
  UI_CMD_PREV,
} ui_command_t;

typedef void (*ui_control_cb_t)(ui_command_t cmd, void* user);

// Crea la pantalla. Requiere board_display_init() previo.
void ui_init(void);

// Callback para los botones táctiles (se invoca en la tarea LVGL).
void ui_set_control_cb(ui_control_cb_t cb, void* user);

// Setters seguros desde cualquier tarea (bloquean LVGL internamente).
void ui_set_env(float temp_c, float hum_pct);
void ui_set_track(const char* title, const char* artist);
void ui_set_playing(bool playing);
void ui_set_status(const char* status);

#ifdef __cplusplus
}
#endif
