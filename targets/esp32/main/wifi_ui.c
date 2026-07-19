// Gestor WiFi con portal tactil: escanea redes, pide contraseña con teclado
// LVGL y guarda credenciales en NVS (namespace "wificfg").

#include "wifi_ui.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs.h"
#include "sdkconfig.h"
#include "ui.h"

#define WIFI_NS "wificfg"
#define BIT_CONNECTED BIT0
#define BIT_FAILED BIT1
#define MAX_RETRIES 5

static const char* TAG = "wifi_ui";

static EventGroupHandle_t eg;
static bool provisioning = false;
static int retries = 0;
static char cur_ssid[33];
static char cur_pass[65];

static lv_obj_t* scr_prov;
static lv_obj_t* scr_main;
static lv_obj_t* net_list;
static lv_obj_t* ta_pass;
static lv_obj_t* kb;
static lv_obj_t* lbl_info;

static void save_creds(void) {
  nvs_handle_t h;
  if (nvs_open(WIFI_NS, NVS_READWRITE, &h) == ESP_OK) {
    nvs_set_str(h, "ssid", cur_ssid);
    nvs_set_str(h, "pass", cur_pass);
    nvs_commit(h);
    nvs_close(h);
  }
}

static bool load_creds(void) {
  nvs_handle_t h;
  size_t l1 = sizeof(cur_ssid), l2 = sizeof(cur_pass);
  if (nvs_open(WIFI_NS, NVS_READONLY, &h) != ESP_OK) {
    return false;
  }
  bool ok = nvs_get_str(h, "ssid", cur_ssid, &l1) == ESP_OK &&
            nvs_get_str(h, "pass", cur_pass, &l2) == ESP_OK;
  nvs_close(h);
  return ok && cur_ssid[0];
}

static void close_provisioning(void) {
  provisioning = false;
  lvgl_port_lock(0);
  if (scr_prov) {
    lv_screen_load(scr_main);
    lv_obj_delete(scr_prov);
    scr_prov = NULL;
    net_list = NULL;
    ta_pass = NULL;
    kb = NULL;
    lbl_info = NULL;
  }
  lvgl_port_unlock();
  ui_set_status("WiFi conectado");
}

static void on_wifi_event(void* arg, esp_event_base_t base, int32_t id,
                          void* data) {
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    if (retries < MAX_RETRIES) {
      retries++;
      esp_wifi_connect();
    } else {
      xEventGroupSetBits(eg, BIT_FAILED);
      if (provisioning && lbl_info) {
        lvgl_port_lock(0);
        lv_label_set_text(lbl_info, "Fallo la conexion, prueba de nuevo");
        lvgl_port_unlock();
      }
    }
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    retries = 0;
    save_creds();
    xEventGroupSetBits(eg, BIT_CONNECTED);
    if (provisioning) {
      close_provisioning();
    }
  }
}

static void try_connect(void) {
  wifi_config_t wc = {0};
  strncpy((char*)wc.sta.ssid, cur_ssid, sizeof(wc.sta.ssid) - 1);
  strncpy((char*)wc.sta.password, cur_pass, sizeof(wc.sta.password) - 1);
  wc.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
  retries = 0;
  xEventGroupClearBits(eg, BIT_CONNECTED | BIT_FAILED);
  esp_wifi_set_config(WIFI_IF_STA, &wc);
  esp_wifi_disconnect();
  esp_wifi_connect();
}

// --- Portal LVGL ---

static void do_connect_from_ui(void) {
  const char* pass = lv_textarea_get_text(ta_pass);
  strncpy(cur_pass, pass, sizeof(cur_pass) - 1);
  cur_pass[sizeof(cur_pass) - 1] = 0;
  if (!cur_ssid[0]) {
    lv_label_set_text(lbl_info, "Elige una red de la lista");
    return;
  }
  lv_label_set_text_fmt(lbl_info, "Conectando a %s...", cur_ssid);
  try_connect();
}

static void kb_ready_cb(lv_event_t* e) {
  do_connect_from_ui();
}

static void net_btn_cb(lv_event_t* e) {
  lv_obj_t* btn = lv_event_get_target(e);
  const char* txt = lv_list_get_button_text(net_list, btn);
  strncpy(cur_ssid, txt, sizeof(cur_ssid) - 1);
  cur_ssid[sizeof(cur_ssid) - 1] = 0;
  lv_label_set_text_fmt(lbl_info, "Red: %s — escribe la contraseña", cur_ssid);
  lv_obj_clear_flag(ta_pass, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_state(ta_pass, LV_STATE_FOCUSED);
}

static void scan_task(void* arg) {
  wifi_scan_config_t sc = {0};
  static wifi_ap_record_t recs[20];
  uint16_t n = 20;
  esp_err_t err = esp_wifi_scan_start(&sc, true);
  if (err == ESP_OK) {
    esp_wifi_scan_get_ap_records(&n, recs);
  } else {
    n = 0;
    ESP_LOGW(TAG, "Scan fallo: %s", esp_err_to_name(err));
  }
  lvgl_port_lock(0);
  if (net_list) {
    lv_obj_clean(net_list);
    for (int i = 0; i < n; i++) {
      if (!recs[i].ssid[0]) {
        continue;
      }
      lv_obj_t* b = lv_list_add_button(net_list, LV_SYMBOL_WIFI,
                                       (const char*)recs[i].ssid);
      lv_obj_add_event_cb(b, net_btn_cb, LV_EVENT_CLICKED, NULL);
    }
    if (lbl_info) {
      lv_label_set_text(lbl_info, n ? "Elige tu red"
                                    : "Sin redes. Reinicia para reintentar");
    }
  }
  lvgl_port_unlock();
  vTaskDelete(NULL);
}

// Construye y muestra el portal. Llamar con el lock LVGL tomado.
static void build_portal_locked(void) {
  provisioning = true;
  scr_main = lv_screen_active();

  scr_prov = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_prov, lv_color_hex(0x101418), 0);
  lv_obj_set_style_bg_opa(scr_prov, LV_OPA_COVER, 0);
  lv_obj_clear_flag(scr_prov, LV_OBJ_FLAG_SCROLLABLE);

  lbl_info = lv_label_create(scr_prov);
  lv_obj_set_style_text_color(lbl_info, lv_color_hex(0xffffff), 0);
  lv_label_set_long_mode(lbl_info, LV_LABEL_LONG_DOT);
  lv_obj_set_width(lbl_info, 312);
  lv_label_set_text(lbl_info, "Buscando redes...");
  lv_obj_align(lbl_info, LV_ALIGN_TOP_LEFT, 4, 4);

  net_list = lv_list_create(scr_prov);
  lv_obj_set_size(net_list, 312, 86);
  lv_obj_align(net_list, LV_ALIGN_TOP_MID, 0, 24);
  lv_obj_set_style_bg_color(net_list, lv_color_hex(0x181c22), 0);

  ta_pass = lv_textarea_create(scr_prov);
  lv_textarea_set_one_line(ta_pass, true);
  lv_textarea_set_password_mode(ta_pass, false);
  lv_textarea_set_placeholder_text(ta_pass, "Contraseña");
  lv_obj_set_size(ta_pass, 312, 32);
  lv_obj_align(ta_pass, LV_ALIGN_TOP_MID, 0, 112);
  lv_obj_add_flag(ta_pass, LV_OBJ_FLAG_HIDDEN);

  kb = lv_keyboard_create(scr_prov);
  lv_obj_set_size(kb, 320, 92);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_keyboard_set_textarea(kb, ta_pass);
  lv_obj_add_event_cb(kb, kb_ready_cb, LV_EVENT_READY, NULL);
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

  lv_screen_load(scr_prov);
}

void wifi_ui_open_settings(void) {
  if (scr_prov) {
    return;  // ya abierto
  }
  build_portal_locked();
  xTaskCreate(scan_task, "wifi_scan", 4096, NULL, 4, NULL);
}

void wifi_ui_connect(void) {
  eg = xEventGroupCreate();

  esp_netif_create_default_wifi_sta();
  wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             on_wifi_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             on_wifi_event, NULL));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_set_ps(WIFI_PS_NONE);

  if (!load_creds()) {
    strncpy(cur_ssid, CONFIG_EXAMPLE_WIFI_SSID, sizeof(cur_ssid) - 1);
    strncpy(cur_pass, CONFIG_EXAMPLE_WIFI_PASSWORD, sizeof(cur_pass) - 1);
  }

  if (cur_ssid[0]) {
    try_connect();
    EventBits_t bits = xEventGroupWaitBits(eg, BIT_CONNECTED | BIT_FAILED,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(20000));
    if (bits & BIT_CONNECTED) {
      return;
    }
  }

  // Sin credenciales validas: portal tactil
  ESP_LOGW(TAG, "Abriendo portal de configuracion WiFi");
  ui_set_status("Configura el WiFi en pantalla");
  lvgl_port_lock(0);
  if (!scr_prov) {
    build_portal_locked();
  }
  lvgl_port_unlock();
  xTaskCreate(scan_task, "wifi_scan", 4096, NULL, 4, NULL);

  xEventGroupWaitBits(eg, BIT_CONNECTED, pdFALSE, pdFALSE, portMAX_DELAY);
}
