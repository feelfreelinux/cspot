# ESP32 CSpot integration

Integrates CSpot with esp32 platform. To configure WiFi SSID and password use idf.py menuconfig.


## Selecting your ESP32

Cspot has been tested to be working on AI-Thinker ESP32 Audio kit board: https://www.aliexpress.com/wholesale?SearchText=esp32-a1s

If you want to use another board, notice that the typical ESP32 RAM size is 160-520kb, which is not enough to run CSpot.

To run CSpot, select a ESP32 with PSRAM. ESP32-WROVER are confirmed to work.
CSpot will not run on a ESP32-WROOM unless the board has external PSRAM.

ref. https://products.espressif.com/#/product-selector

