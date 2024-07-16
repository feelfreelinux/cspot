#ifndef VS1053_H
#define VS1053_H

#include <cstring>     //for memset
#include <deque>       //for dequeue
#include <functional>  //for function
#include <iostream>
#include <memory>

#include "esp_err.h"

#include <driver/gpio.h>
#include <driver/spi_master.h>

#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"

#include "vs10xx_uc.h"
#ifdef CONFIG_VS_DSD64
#include "patches_dsd.h"
#endif
#ifdef CONFIG_VS_FLAC
#include "patches_flac.h"
#endif
#ifdef CONFIG_VS_FLAC_LATM
#include "patches_flac_latm.h"
#endif
#ifdef CONFIG_VS_LATM
#include "patches_latm.h"
#endif
#ifdef CONFIG_VS_PITCH
#include "patches_pitch.h"
#endif
#ifdef CONFIG_VS_SPECTRUM_ANALYZER
#include "spectrum_analyzer.h"
#endif

#define VERSION 1
#define VS1053_CHUNK_SIZE 16  // chunck size
#define VS1053_PACKET_SIZE 8
#define BUF_SIZE_CMD 1028
#define BUF_SIZE_FEED 4096 * 4

#define SDI_END_FILL_BYTES_FLAC 12288
#define SDI_END_FILL_BYTES 2050

#define REPORT_INTERVAL 4096
#define REPORT_INTERVAL_MIDI 512
/** 
 * 
 * callback for the command_pipeline
 * 
 */
class VS1053_SINK;
class VS1053_TRACK {
 public:
  VS1053_TRACK(VS1053_SINK* vsSink, size_t track_id = 0,
               size_t buffer_size = BUF_SIZE_FEED);
  ~VS1053_TRACK();
  /**
     * feed data to dataBuffer
     * @param data pointer to byte array
     * @param len length of the byte array
     * @warning always call data_request, before feeding data, 
     * to get available space in dataBuffer
    */
  size_t feed_data(uint8_t* data, size_t len, bool STORAGE_VOLATILE = 0);
  void empty_feed();
  void run_track(size_t FILL_BUFFER_BEFORE_PLAYSTART = 0);
  VS1053_SINK* audioSink;
  TaskHandle_t track_handle = NULL;
  enum VS_TRACK_STATE {
    tsPlaybackStart = 0,
    tsPlayback = 1,
    tsPlaybackSeekable = 2,
    tsPlaybackPaused = 3,
    tsSoftCancel = 4,
    tsCancel = 5,
    tsCancelAwait = 6,
    tsStopped = 7
  } track_state = tsStopped;
  size_t header_size = 0;
  size_t track_id;
  StreamBufferHandle_t dataBuffer;

 private:
};
class VS1053_SINK {
 public:
  /**
     * `CONSTRUCTOR`
     * 
     * PULLS CS and DCS HIGH.
     * If RESET >= 0, VS1053 gets hardreset.
     * For sharing the SPI_BUS with a SD-Card, the CS-pins
     * of the other devices are needed to be pulled high
     * before mounting the SD-card.
     * After mounting the SD-card, you can add the other devices
     * 
     * @param RESET GPIO_PIN_NUM, if 
    */
  VS1053_SINK();

  /**
     * DECONSTRUCTOR
    */
  ~VS1053_SINK();
  typedef std::function<void(uint8_t)> command_callback;
  //typedef std::function<void(void*)> command_callback;
  /**
     * `init feed`
     *  add the VS 1053 to the SPI_BUS, test the device and
     *  add vs_loop to freertosTask
     * 
     * @param SPI pointer to the used SPI peripheral
     * @param SPI_semaphore semaphore for the SPI_BUS, NULL if uninitiallized
     * 
     * @return `ESP_OK` if setup worked.
     * @return `ESP_ERR_INVALID_RESPONSE` if the register response is wrong.
     * @return `ESP_ERR_NOT_SUPPORTED` if the chip is not a VS1053.
     * @return `ESP_ERR_NOT_FOUND` if the chipNumber is not recognized.
     */
  esp_err_t init(spi_host_device_t SPI,
                 SemaphoreHandle_t* SPI_semaphore = NULL);
  /**
     * stops data feed imeaditly
    */
  void stop_feed();
  /**
     * stops data feed if dataBuffer contains no more
    */
  void soft_stop_feed();
  /**
     * feed data to commandBuffer
     * @param command_callback callback function 
     * 
    */
  uint8_t feed_command(command_callback commandCallback);
  void set_volume_logarithmic(size_t vol);
  /**
     * set volume through cmd_pipeline, sets left and right volume to vol
     * @param vol 0...100, gets capped if bigger
     */
  void set_volume(uint8_t vol);
  /**
     * set volume through cmd_pipeline, sets separate volume for left and right channel
     * @param left 0...100, gets capped if bigger
     * @param right 0...100, gets capped if bigger
     */
  void set_volume(uint8_t left, uint8_t right);
  /**
     * get available Space in dataBuffer
     * @return free space available in dataBuffer
     */
  size_t data_request();
  /**
     * loads Usercode(PATCH)
     * @param plugin uint8_t * to plugin array
     * @param sizeofpatch length of the array
     */
  void load_user_code(const unsigned short* plugin, uint16_t sizeofpatch);
  /**
     * test SPI communication and if the board is a VS1053
     * @return `ESP_OK` if setup worked.
     * @return `ESP_ERR_INVALID_RESPONSE` if the register response is wrong.
     * @return `ESP_ERR_NOT_SUPPORTED` if the chip is not a VS1053.
     * @return `ESP_ERR_NOT_FOUND` if the chipNumber is not recognized.
     */
  esp_err_t test_comm(const char* header);
  std::function<void(uint8_t)> state_callback = nullptr;
  enum Audio_Format {
    afUnknown,
    afRiff,
    afOggVorbis,
    afMp1,
    afMp2,
    afMp3,
    afAacMp4,
    afAacAdts,
    afAacAdif,
    afFlac,
    afWma,
    afMidi,
    afDsd64,
    afLatm
  } audioFormat = afUnknown;
  void get_audio_format(Audio_Format* audioFormat, size_t* endFillBytes);
  void control_mode_on();
  void control_mode_off();
  void data_mode_on();
  void data_mode_off();
  uint16_t read_register(uint8_t _reg);
  bool write_register(uint8_t _reg, uint16_t _value);
  uint32_t read_mem32(uint16_t addr);
  uint32_t read_mem32_counter(uint16_t addr);
  uint16_t read_mem(uint16_t addr);
  void write_mem(uint16_t addr, uint16_t data);
  void write_mem32(uint16_t addr, uint32_t data);
  bool sdi_send_buffer(uint8_t* data, size_t len);
  void remove_track(std::shared_ptr<VS1053_TRACK> track) {
    if (this->track->track_id == track->track_id)
      this->track = nullptr;
  };
  void start_track(std::shared_ptr<VS1053_TRACK>, size_t);
  bool is_seekable(VS1053_TRACK::VS_TRACK_STATE* state);
  size_t track_seekable(size_t);
  void cancel_track(VS1053_TRACK::VS_TRACK_STATE* state);
  bool is_cancelled(VS1053_TRACK::VS_TRACK_STATE* state);
  size_t get_track_info(size_t);
  void new_state(VS1053_TRACK::VS_TRACK_STATE);
  void new_track(std::shared_ptr<VS1053_TRACK> track);
  VS1053_TRACK newTrack(size_t track_id, size_t buffer_size = BUF_SIZE_FEED);

  void delete_track(void);
  size_t spaces_available(size_t);
  std::shared_ptr<VS1053_TRACK> track = nullptr;
  std::shared_ptr<VS1053_TRACK> future_track = nullptr;
  size_t command_pointer = 0, command_reader = 0;
  std::deque<command_callback> command_callbacks;

 private:
  spi_device_handle_t SPIHandleLow;
  spi_device_handle_t SPIHandleFast;
  uint8_t curvol;           // Current volume setting 0..100%
  uint8_t endFillByte = 0;  // Byte to send when stopping song
  size_t endFillBytes = SDI_END_FILL_BYTES;
  int playMode = 0;
  uint8_t chipVersion;  // Version of hardware
  SemaphoreHandle_t* SPI_semaphore = NULL;
  TaskHandle_t VS_TASK;
  void await_data_request();
  bool sdi_send_fillers(size_t len);
  void wram_write(uint16_t address, uint16_t data);
  uint16_t wram_read(uint16_t address);
  size_t (*data_callback)(uint8_t*, size_t) = NULL;
};

#endif