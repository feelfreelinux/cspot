#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Conecta WiFi (bloqueante). Usa credenciales guardadas en NVS o las de
// sdkconfig; si fallan, muestra el portal tactil de seleccion de red y
// espera hasta lograr conexion. Requiere ui_init() previo.
void wifi_ui_connect(void);

// Abre el portal de seleccion de red (llamar desde la tarea LVGL).
void wifi_ui_open_settings(void);

#ifdef __cplusplus
}
#endif
