#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa I2C0 compartido (SDA=16, SCL=15), SPI + panel ILI9341V,
// touch FT6336G y el port LVGL (tarea + tick). Llamar una sola vez
// antes de crear la UI.
void board_display_init(void);

#ifdef __cplusplus
}
#endif
