#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Lanza la tarea que lee el SHT31-D (I2C0 compartido, addr 0x44) cada 5 s
// y publica temperatura/humedad en la UI. Requiere board_display_init().
void sht31_start(void);

#ifdef __cplusplus
}
#endif
