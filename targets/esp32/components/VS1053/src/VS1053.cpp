#include "VS1053.h"

#include <math.h>
#include "esp_log.h"

static const char* TAG = "VS_SINK";
void vs_feed(void* sink) {
  ((VS1053_SINK*)sink)->run_feed(1024);
}

unsigned char pcm_wav_header[44] = {
    0x52, 0x49, 0x46,
    0x46,  // RIFF
    0xFF, 0xFF, 0xFF,
    0xFF,  // size
    0x57, 0x41, 0x56,
    0x45,  // WAVE
    0x66, 0x6d, 0x74,
    0x20,  // fmt
    0x10, 0x00, 0x00,
    0x00,        // subchunk1size
    0x01, 0x00,  // audio format - pcm
    0x02, 0x00,  // numof channels
    0x44, 0xac, 0x00,
    0x00,  //, //samplerate 44k1: 0x44, 0xac, 0x00, 0x00       48k: 48000: 0x80, 0xbb, 0x00, 0x00,
    0x10, 0xb1, 0x02,
    0x00,        // byterate
    0x04, 0x00,  // blockalign
    0x10, 0x00,  // bits per sample - 16
    0x64, 0x61, 0x74,
    0x61,  // subchunk3id -"data"
    0xFF, 0xFF, 0xFF,
    0xFF  // subchunk3size (endless)
};
const char* afName[] = {
    "unknown",  "RIFF",     "Ogg",  "MP1", "MP2",  "MP3",   "AAC MP4",
    "AAC ADTS", "AAC ADIF", "FLAC", "WMA", "MIDI", "DSD64", "LATM/LOAS",
};
VS1053_SINK::VS1053_SINK() {
  // PIN CONFIG
  // DREQ
  ESP_LOGI(TAG, "VS1053_DREQ=%d", CONFIG_GPIO_VS_DREQ);
  isRunning = true;
  gpio_config_t gpio_conf;
  gpio_conf.mode = GPIO_MODE_INPUT;
  gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.pin_bit_mask = ((uint64_t)(((uint64_t)1) << CONFIG_GPIO_VS_DREQ));
  ESP_ERROR_CHECK(gpio_config(&gpio_conf));
  // CS
  ESP_LOGI(TAG, "VS1053_CS=%d", CONFIG_GPIO_VS_CS);
  gpio_reset_pin((gpio_num_t)CONFIG_GPIO_VS_CS);
  gpio_set_direction((gpio_num_t)CONFIG_GPIO_VS_CS, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)CONFIG_GPIO_VS_CS, 1);
  // DCS
  ESP_LOGI(TAG, "VS1053_DCS=%d", CONFIG_GPIO_VS_DCS);
  gpio_reset_pin((gpio_num_t)CONFIG_GPIO_VS_DCS);
  gpio_set_direction((gpio_num_t)CONFIG_GPIO_VS_DCS, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)CONFIG_GPIO_VS_DCS, 1);
  // RESET
  ESP_LOGI(TAG, "VS1053_RESET=%d", CONFIG_GPIO_VS_RESET);
  if (CONFIG_GPIO_VS_RESET >= 0) {
    gpio_reset_pin((gpio_num_t)CONFIG_GPIO_VS_RESET);
    gpio_set_direction((gpio_num_t)CONFIG_GPIO_VS_RESET, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)CONFIG_GPIO_VS_RESET, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level((gpio_num_t)CONFIG_GPIO_VS_RESET, 1);
  }
}

esp_err_t VS1053_SINK::init(spi_host_device_t SPI,
                            SemaphoreHandle_t* SPI_semaphore) {
  esp_err_t ret;
  this->SPI_semaphore = SPI_semaphore;
  // SPI CONFIG
  uint32_t freq = spi_get_actual_clock(APB_CLK_FREQ, 1400000, 128);
  spi_device_interface_config_t devcfg;
  memset(&devcfg, 0, sizeof(spi_device_interface_config_t));
  ESP_LOGI(TAG, "VS1053 LOWFreq: %d", freq);

  devcfg.clock_speed_hz = freq, devcfg.command_bits = 8,
  devcfg.address_bits = 8, devcfg.dummy_bits = 0, devcfg.duty_cycle_pos = 0,
  devcfg.cs_ena_pretrans = 0, devcfg.cs_ena_posttrans = 1, devcfg.flags = 0,
  devcfg.mode = 0, devcfg.spics_io_num = CONFIG_GPIO_VS_CS,
  devcfg.queue_size = 1,
  devcfg.pre_cb = NULL,  // Specify pre-transfer callback to handle D/C line
      devcfg.post_cb = NULL;

  ESP_LOGI(TAG, "spi device interface config done, VERSION : %i", VERSION);
  ret = spi_bus_add_device(SPI, &devcfg, &this->SPIHandleLow);
  ESP_LOGI(TAG, "spi_bus_add_device=%d", ret);
  assert(ret == ESP_OK);
  // SPI TEST SLOW-/HIGH-SPEED
  vTaskDelay(20 / portTICK_PERIOD_MS);
  write_register(SCI_MODE, SM_SDINEW | SM_TESTS | SM_RESET);
  ret = test_comm("Slow SPI,Testing VS1053 read/write registers...\n");
  if (ret != ESP_OK)
    return ret;
  {
    freq = spi_get_actual_clock(APB_CLK_FREQ, 6670000, 128);
    write_register(
        SCI_CLOCKF,
        HZ_TO_SC_FREQ(12288000) | SC_MULT_53_45X |
            SC_ADD_53_00X);  // Normal clock settings multiplyer 3.0 = 12.2 MHz
    ESP_LOGI(TAG, "VS1053 HighFreq: %d", freq);
    devcfg.clock_speed_hz = freq, devcfg.command_bits = 0,
    devcfg.address_bits = 0, devcfg.spics_io_num = CONFIG_GPIO_VS_DCS;
    ret = spi_bus_add_device(SPI, &devcfg, &this->SPIHandleFast);
    ESP_LOGI(TAG, "spi_bus_add_device=%d", ret);
    assert(ret == ESP_OK);
    ret = test_comm("Slow SPI,Testing VS1053 read/write registers...\n");
    if (ret != ESP_OK)
      return ret;
  }
  vTaskDelay(100 / portTICK_PERIOD_MS);

  write_mem(PAR_CONFIG1, PAR_CONFIG1_AAC_SBR_SELECTIVE_UPSAMPLE);
  write_register(SCI_VOL, 0x0c0c);
#ifndef CONFIG_VS1053_NO_PLUGIN
  //load_user_code(PLUGIN, PLUGIN_SIZE);
#endif
  vTaskDelay(100 / portTICK_PERIOD_MS);
  xTaskCreate(vs_feed, "track_feed", 4098, (void*)this, 10, &task_handle);
  return ESP_OK;
}

VS1053_SINK::~VS1053_SINK() {
  if (task_handle != NULL)
    vTaskDelete(task_handle);
  task_handle = NULL;
  isRunning = false;
}
// LOOP
VS1053_TRACK::VS1053_TRACK(size_t track_id, size_t buffer_size) {
  this->track_id = track_id;
  this->dataBuffer = xStreamBufferCreate(buffer_size, 1);
  // this->run_track();
}
VS1053_TRACK::~VS1053_TRACK() {
  if (dataBuffer != NULL)
    vStreamBufferDelete(dataBuffer);
  dataBuffer = NULL;
}

void VS1053_SINK::new_track(std::shared_ptr<VS1053_TRACK> track) {
  tracks.push_back(track);
}
bool VS1053_SINK::is_seekable(VS1053_TRACK::VS_TRACK_STATE* state) {
  if (read_register(SCI_STATUS) >> SS_DO_NOT_JUMP_B == 0) {
    new_state(*state, VS1053_TRACK::VS_TRACK_STATE::tsPlaybackSeekable);
    return true;
  }
  return false;
}
size_t VS1053_SINK::track_seekable(size_t track_id) {
  if (tracks.size())
    if (tracks[0]->track_id == track_id)
      return tracks[0]->header_size;
  return 0;
}
void VS1053_SINK::cancel_track(VS1053_TRACK::VS_TRACK_STATE* state) {
  unsigned short oldMode;
  oldMode = read_register(SCI_MODE);
  write_register(SCI_MODE, oldMode | SM_CANCEL);
  new_state(*state, VS1053_TRACK::VS_TRACK_STATE::tsCancelAwait);
}
bool VS1053_SINK::is_cancelled(VS1053_TRACK::VS_TRACK_STATE* state,
                               uint8_t endFillByte, size_t endFillBytes) {
  if (read_register(SCI_MODE) & SM_CANCEL)
    sdi_send_fillers(endFillByte, 2);
  else {
    sdi_send_fillers(endFillByte, endFillBytes);
    new_state(*state, VS1053_TRACK::VS_TRACK_STATE::tsStopped);
    return true;
  }
  return false;
}
void VS1053_SINK::delete_all_tracks(void) {
  if (this->tracks.size() > 1)
    this->tracks.erase(tracks.begin() + 1, tracks.end());
  if (this->tracks[0]->state != VS1053_TRACK::VS_TRACK_STATE::tsStopped)
    new_state(this->tracks[0]->state, VS1053_TRACK::VS_TRACK_STATE::tsCancel);
}
size_t VS1053_SINK::get_track_info(size_t pos, uint8_t& endFillByte,
                                   size_t& endFillBytes) {
#ifdef CONFIG_REPORT_ON_SCREEN
  uint16_t sampleRate;
  uint32_t byteRate;
#endif
  get_audio_format(&audioFormat, &endFillBytes);

  /* It is important to collect endFillByte while still in normal
      playback. If we need to later cancel playback or run into any
      trouble with e.g. a broken file, we need to be able to repeatedly
      send this byte until the decoder has been able to exit. */

  endFillByte = read_mem(PAR_END_FILL_BYTE);
#ifdef CONFIG_REPORT_ON_SCREEN

  sampleRate = read_register(SCI_AUDATA);
  byteRate = read_mem(PAR_BYTERATE);
  /* FLAC:   byteRate = bitRate / 32
         Others: byteRate = bitRate /  8
         Here we compensate for that difference. */
  if (audioFormat == afFlac)
    byteRate *= 4;

  ESP_LOGI(TAG,
           "%dKiB "
           "%1ds %1.1f"
           "kb/s %dHz %s %s",
           pos / (1024 / VS1053_PACKET_SIZE), read_register(SCI_DECODE_TIME),
           byteRate * (8.0 / 1000.0), sampleRate & 0xFFFE,
           (sampleRate & 1) ? "stereo" : "mono", afName[audioFormat]);
#endif
  return (audioFormat == afMidi || audioFormat == afUnknown)
             ? REPORT_INTERVAL_MIDI
             : REPORT_INTERVAL;
}

size_t VS1053_TRACK::feed_data(uint8_t* data, size_t len,
                               bool STORAGE_VOLATILE) {
  if (!len || xStreamBufferSpacesAvailable(this->dataBuffer) < len)
    return 0;
  if (STORAGE_VOLATILE)
    if (this->header_size)
      xStreamBufferReset(this->dataBuffer);
  size_t res =
      xStreamBufferSend(this->dataBuffer, (void*)data, len, pdMS_TO_TICKS(100));
  return res;
}
size_t VS1053_SINK::spaces_available(size_t track_id) {
  if (this->tracks.size())
    for (auto track : tracks)
      if (track->track_id == track_id)
        return xStreamBufferSpacesAvailable(track->dataBuffer);
  return 0;
}
void VS1053_TRACK::empty_feed() {
  if (this->dataBuffer != NULL)
    xStreamBufferReset(this->dataBuffer);
}
void VS1053_SINK::new_state(VS1053_TRACK::VS_TRACK_STATE& state,
                            VS1053_TRACK::VS_TRACK_STATE new_state) {
  state = new_state;
  if (state_callback != NULL) {
    state_callback((uint8_t)new_state);
  }
}

void VS1053_SINK::run_feed(size_t FILL_BUFFER_BEFORE_PLAYBACK) {
  while (isRunning) {
    uint8_t* item = (uint8_t*)malloc(VS1053_PACKET_SIZE);
    if (tracks.size()) {
      size_t itemSize = 0;
      uint32_t pos = 0;
      long nextReportPos = 0;  // File pointer where to next collect/report
      std::shared_ptr<VS1053_TRACK> track = tracks.front();
      uint8_t endFillByte = 0;  // Byte to send when stopping song
      size_t endFillBytes = SDI_END_FILL_BYTES;
      playMode = read_mem(PAR_PLAY_MODE);
      sdi_send_fillers(endFillByte, endFillBytes);
      write_register(SCI_DECODE_TIME, 0);  // Reset DECODE_TIME
      new_state(track->state, VS1053_TRACK::VS_TRACK_STATE::tsPlaybackStart);
      if (FILL_BUFFER_BEFORE_PLAYBACK <
          xStreamBufferBytesAvailable(track->dataBuffer))
        vTaskDelay(10 / portTICK_PERIOD_MS);
      while (track->state != VS1053_TRACK::VS_TRACK_STATE::tsStopped) {
        if (this->command_callbacks.size()) {
          this->command_callbacks[0](track->track_id);
          this->command_callbacks.pop_front();
        }

        switch (track->state) {
          case VS1053_TRACK::VS_TRACK_STATE::tsPlaybackStart:
            this->new_state(
                track->state,
                VS1053_TRACK::VS1053_TRACK::VS_TRACK_STATE::tsPlayback);
            goto tsPlaybackSeekable;
          case VS1053_TRACK::VS_TRACK_STATE::tsPlayback:
            if (this->is_seekable(&track->state))
              if (!track->header_size)
                track->header_size = VS1053_PACKET_SIZE * pos;
            goto tsPlaybackSeekable;
          case VS1053_TRACK::VS_TRACK_STATE::tsPlaybackSeekable:

          tsPlaybackSeekable:
            itemSize = xStreamBufferReceive(track->dataBuffer, (void*)item,
                                            VS1053_PACKET_SIZE, 10);
            if (itemSize) {
              this->sdi_send_buffer(item, itemSize);
              pos++;
            }
            break;
          case VS1053_TRACK::VS_TRACK_STATE::tsSoftCancel:
            if (xStreamBufferBytesAvailable(track->dataBuffer))
              goto tsPlaybackSeekable;
            this->new_state(track->state,
                            VS1053_TRACK::VS_TRACK_STATE::tsCancel);
            [[fallthrough]];
          case VS1053_TRACK::VS_TRACK_STATE::tsCancel:
            free(item);
            track->empty_feed();
            this->cancel_track(&track->state);
            [[fallthrough]];
          case VS1053_TRACK::VS_TRACK_STATE::tsCancelAwait:
            if (this->is_cancelled(&track->state, endFillByte, endFillBytes)) {}
            break;
          default:
            vTaskDelay(20 / portTICK_PERIOD_MS);
            break;
        }
        if (pos >= nextReportPos) {
          nextReportPos += this->get_track_info(pos, endFillByte, endFillBytes);
        }
      }
      vStreamBufferDelete(track->dataBuffer);
      track->dataBuffer = NULL;
      tracks.pop_front();
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
  task_handle = NULL;
  vTaskDelete(NULL);
}
// FEED FUNCTIONS

size_t VS1053_SINK::data_request() {
  return xStreamBufferSpacesAvailable(track->dataBuffer);
}

void VS1053_SINK::stop_feed() {
  if (this->tracks[0]->state < 3)
    new_state(this->tracks[0]->state, VS1053_TRACK::VS_TRACK_STATE::tsCancel);
}
void VS1053_SINK::soft_stop_feed() {
  if (this->tracks[0]->state < 3)
    new_state(this->tracks[0]->state,
              VS1053_TRACK::VS_TRACK_STATE::tsSoftCancel);
}

// COMMAND FUCNTIONS
/* The command pipeline recieves command in a structure of uint8_t[].
 */
uint8_t VS1053_SINK::feed_command(command_callback commandCallback) {
  if (this->tracks.size()) {
    command_callbacks.push_back(commandCallback);
  } else
    commandCallback(0);
  return 0;
}
void VS1053_SINK::set_volume_logarithmic(size_t vol) {
  float value = log10((float)vol / 0xFFFF * 100 + 1);
  if (value >= 2.0)
    value = 2.0;
  size_t logVolume = value / 2 * 100;  // *100

  this->set_volume(logVolume);
}
/**
 * set_volume accepts values from 0 to 100
 */
void VS1053_SINK::set_volume(uint8_t vol) {
  vol = vol > 100 ? 0 : 100 - vol;
  uint16_t value = (vol << 8) | vol;
  write_register(SCI_VOL, value);  // Volume left and right
}

void VS1053_SINK::set_volume(uint8_t left, uint8_t right) {
  left = left > 100 ? 0 : 100 - left;
  right = right > 100 ? 0 : 100 - right;
  uint16_t value = (left << 8) | right;
  write_register(SCI_VOL, value);  // Volume left and right
}

void VS1053_SINK::get_audio_format(Audio_Format* audioFormat,
                                   size_t* endFillBytes) {
  uint16_t h1 = read_register(SCI_HDAT1);
  *audioFormat = afUnknown;
  switch (h1) {
    case 0x7665:
      *audioFormat = afRiff;
      goto sdi_end_fill_bytes;
    case 0x4444:
      *audioFormat = afDsd64;
      goto sdi_end_fill_bytes_flac;
    case 0x4c41:
      *audioFormat = afLatm;
      // set OUT_OF_WAV bit of SCI_MODE;
      goto sdi_end_fill_bytes_flac;
    case 0x4154:
      *audioFormat = afAacAdts;
      goto sdi_end_fill_bytes;
    case 0x4144:
      *audioFormat = afAacAdif;
      goto sdi_end_fill_bytes;
    case 0x574d:
      *audioFormat = afWma;
      goto sdi_end_fill_bytes;
    case 0x4f67:
      *audioFormat = afOggVorbis;
      goto sdi_end_fill_bytes;
    case 0x664c:
      *audioFormat = afFlac;
      goto sdi_end_fill_bytes_flac;
    case 0x4d34:
      *audioFormat = afAacMp4;
      goto sdi_end_fill_bytes;
    case 0x4d54:
      *audioFormat = afMidi;
      goto sdi_end_fill_bytes;
    default:
      h1 &= 0xffe6;
      if (h1 == 0xffe2)
        *audioFormat = afMp2;
      else if (h1 == 0xffe4)
        *audioFormat = afMp2;
      else if (h1 == 0xffe6)
        *audioFormat = afMp1;
      if (*audioFormat == afUnknown)
        goto sdi_end_fill_bytes_flac;
    sdi_end_fill_bytes:
      *endFillBytes = SDI_END_FILL_BYTES;
      break;
    sdi_end_fill_bytes_flac:
      *endFillBytes = SDI_END_FILL_BYTES_FLAC;
      break;
  }
}

// SPI COMMUNICATION TEST
const uint16_t chipNumber[16] = {1001, 1011, 1011, 1003, 1053, 1033, 1063, 1103,
                                 0,    0,    0,    0,    0,    0,    0,    0};

esp_err_t VS1053_SINK::test_comm(const char* header) {
  // Test the communication with the VS1053 module.  The result wille be returned.
  // If DREQ is low, there is problably no VS1053 connected.	Pull the line HIGH
  // in order to prevent an endless loop waiting for this signal.  The rest of the
  // software will still work, but readbacks from VS1053 will fail.

  write_register(SCI_AICTRL1, 0xABAD);
  write_register(SCI_AICTRL2, 0x7E57);
  if (read_register(SCI_AICTRL1) != 0xABAD ||
      read_register(SCI_AICTRL2) != 0x7E57) {
    ESP_LOGI(TAG, "There is something wrong with VS10xx SCI registers\n");
    return ESP_ERR_INVALID_RESPONSE;
  }
  write_register(SCI_AICTRL1, 0);
  write_register(SCI_AICTRL2, 0);
  uint16_t ssVer = ((read_register(SCI_STATUS) >> 4) & 15);
  if (chipNumber[ssVer]) {
    ESP_LOGI(TAG, "Chip is VS%d\n", chipNumber[ssVer]);
    if (chipNumber[ssVer] != 1053) {
      ESP_LOGI(TAG, "Incorrect chip\n");
      return ESP_ERR_NOT_SUPPORTED;
    }
  } else {
    ESP_LOGI(TAG, "Unknown VS10xx SCI_MODE field SS_VER = %d\n", ssVer);
    return ESP_ERR_NOT_FOUND;
  }

  return ESP_OK;
}

// PLUGIN FUNCTION

void VS1053_SINK::load_user_code(const unsigned short* plugin,
                                 uint16_t sizeofpatch) {
  ESP_LOGI(TAG, "Loading patch");
  await_data_request();
  int i = 0;
  while (i < sizeofpatch) {
    unsigned short addr, n, val;
    addr = plugin[i++];
    n = plugin[i++];
    if (n & 0x8000U) { /* RLE run, replicate n samples */
      n &= 0x7FFF;
      val = plugin[i++];
      while (n--) {
        write_register(addr, val);
      }
    } else { /* Copy run, copy n samples */
      while (n--) {
        val = plugin[i++];
        write_register(addr, val);
      }
    }
  }
}

// GPIO FUNCTIONS

void VS1053_SINK::await_data_request() {
  while (!gpio_get_level((gpio_num_t)CONFIG_GPIO_VS_DREQ))
    vTaskDelay(1);
}
// WRITE/READ FUNCTIONS

uint16_t VS1053_SINK::read_register(uint8_t _reg) {
  spi_transaction_t SPITransaction;
  esp_err_t ret;
  await_data_request();  // Wait for DREQ to be HIGH
  memset(&SPITransaction, 0, sizeof(spi_transaction_t));
  SPITransaction.length = 16;
  SPITransaction.flags |= SPI_TRANS_USE_RXDATA;
  SPITransaction.cmd = VS_READ_COMMAND;
  SPITransaction.addr = _reg;
  if (SPI_semaphore != NULL)
    while (xSemaphoreTake(*SPI_semaphore, 1) != pdTRUE)
      vTaskDelay(1);
  ret = spi_device_transmit(this->SPIHandleLow, &SPITransaction);
  assert(ret == ESP_OK);
  uint16_t result = (((SPITransaction.rx_data[0] & 0xFF) << 8) |
                     ((SPITransaction.rx_data[1]) & 0xFF));
  await_data_request();  // Wait for DREQ to be HIGH again
  if (SPI_semaphore != NULL)
    xSemaphoreGive(*SPI_semaphore);
  return result;
}

bool VS1053_SINK::write_register(uint8_t _reg, uint16_t _value) {
  spi_transaction_t SPITransaction;
  esp_err_t ret;

  await_data_request();  // Wait for DREQ to be HIGH
  memset(&SPITransaction, 0, sizeof(spi_transaction_t));
  SPITransaction.flags |= SPI_TRANS_USE_TXDATA;
  SPITransaction.cmd = VS_WRITE_COMMAND;
  SPITransaction.addr = _reg;
  SPITransaction.tx_data[0] = (_value >> 8) & 0xFF;
  SPITransaction.tx_data[1] = (_value & 0xFF);
  SPITransaction.length = 16;
  if (SPI_semaphore != NULL)
    while (xSemaphoreTake(*SPI_semaphore, 1) != pdTRUE)
      vTaskDelay(1);
  ret = spi_device_transmit(this->SPIHandleLow, &SPITransaction);
  assert(ret == ESP_OK);
  await_data_request();  // Wait for DREQ to be HIGH again
  if (SPI_semaphore != NULL)
    xSemaphoreGive(*SPI_semaphore);
  return true;
}

uint32_t VS1053_SINK::read_mem32(uint16_t addr) {
  uint16_t result;

  write_register(SCI_WRAMADDR, addr);
  // Note: transfer16 does not seem to work
  result = read_register(SCI_WRAM);
  return result | ((uint32_t)read_register(SCI_WRAM) << 16);
}

uint32_t VS1053_SINK::read_mem32_counter(uint16_t addr) {
  uint16_t msbV1, lsb, msbV2;
  uint32_t res;
  write_register(SCI_WRAMADDR, addr);
  msbV1 = read_register(SCI_WRAM);
  write_register(SCI_WRAMADDR, addr);
  lsb = read_register(SCI_WRAM);
  msbV2 = read_register(SCI_WRAM);
  if (lsb < 0x8000U) {
    msbV1 = msbV2;
  }
  res = ((uint32_t)msbV1 << 16) | lsb;

  return res;
}

uint16_t VS1053_SINK::read_mem(uint16_t addr) {
  write_register(SCI_WRAMADDR, addr);
  return read_register(SCI_WRAM);
}

/*
  Write 16-bit value to given VS10xx address
*/
void VS1053_SINK::write_mem(uint16_t addr, uint16_t data) {
  write_register(SCI_WRAMADDR, addr);
  write_register(SCI_WRAM, data);
}

/*
  Write 32-bit value to given VS10xx address
*/
void VS1053_SINK::write_mem32(uint16_t addr, uint32_t data) {
  write_register(SCI_WRAMADDR, addr);
  write_register(SCI_WRAM, (uint16_t)data);
  write_register(SCI_WRAM, (uint16_t)(data >> 16));
}
bool VS1053_SINK::sdi_send_buffer(uint8_t* data, size_t len) {
  size_t chunk_length;  // Length of chunk 32 byte or shorter
  spi_transaction_t SPITransaction;
  esp_err_t ret;
  if (SPI_semaphore != NULL)
    while (xSemaphoreTake(*SPI_semaphore, 1) != pdTRUE)
      vTaskDelay(1);
  while (len)  // More to do?
  {
    await_data_request();  // Wait for space available
    chunk_length = len;
    if (len > VS1053_CHUNK_SIZE) {
      chunk_length = VS1053_CHUNK_SIZE;
    }
    len -= chunk_length;
    memset(&SPITransaction, 0, sizeof(spi_transaction_t));
    SPITransaction.length = chunk_length * 8;
    SPITransaction.tx_buffer = data;
    // while(spi_device_acquire_bus(this->SPIHandleFast,portMAX_DELAY)!=ESP_OK){};
    ret = spi_device_transmit(this->SPIHandleFast, &SPITransaction);
    // spi_device_release_bus(this->SPIHandleFast);
    assert(ret == ESP_OK);
    data += chunk_length;
  }
  if (SPI_semaphore != NULL)
    xSemaphoreGive(*SPI_semaphore);

  return true;
}

bool VS1053_SINK::sdi_send_fillers(uint8_t endFillByte, size_t len) {
  size_t chunk_length;  // Length of chunk 32 byte or shorter
  spi_transaction_t SPITransaction;
  esp_err_t ret;
  uint8_t data[VS1053_CHUNK_SIZE];
  for (int i = 0; i < VS1053_CHUNK_SIZE; i++)
    data[i] = endFillByte;
  if (SPI_semaphore != NULL)
    while (xSemaphoreTake(*SPI_semaphore, 1) != pdTRUE)
      vTaskDelay(1);
  while (len)  // More to do?
  {
    await_data_request();  // Wait for space available
    chunk_length = len;
    if (len > VS1053_CHUNK_SIZE) {
      chunk_length = VS1053_CHUNK_SIZE;
    }
    len -= chunk_length;

    memset(&SPITransaction, 0, sizeof(spi_transaction_t));
    SPITransaction.length = chunk_length * 8;
    SPITransaction.tx_buffer = data;
    spi_device_acquire_bus(this->SPIHandleFast, portMAX_DELAY);
    ret = spi_device_transmit(this->SPIHandleFast, &SPITransaction);
    spi_device_release_bus(this->SPIHandleFast);
    assert(ret == ESP_OK);
  }
  if (SPI_semaphore != NULL)
    xSemaphoreGive(*SPI_semaphore);
  return true;
}

void VS1053_SINK::wram_write(uint16_t address, uint16_t data) {
  write_register(SCI_WRAMADDR, address);
  write_register(SCI_WRAM, data);
}

uint16_t VS1053_SINK::wram_read(uint16_t address) {
  write_register(SCI_WRAMADDR, address);  // Start reading from WRAM
  return read_register(SCI_WRAM);         // Read back result
}
