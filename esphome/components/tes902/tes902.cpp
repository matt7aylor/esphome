#include "tes902.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tes902 {

static const char *const TAG = "tes902";

void TES902Component::update() { ESP_LOGV(TAG, "TES Update function"); }

uint16_t Calculate_CRC16(uint8_t *cmd, int cmd_length) {
  uint16_t ret = 0xffff, polynomial = 0xa001;

  for (int i = cmd_length - 1; i >= 0; i--) {
    uint16_t code = static_cast<uint16_t>(cmd[cmd_length - 1 - i] & 0xff);
    ret ^= code;

    for (int shift = 0; shift <= 7; shift++) {
      if (ret & 0x1) {
        ret >>= 1;
        ret ^= polynomial;
      } else {
        ret >>= 1;
      }
    }
  }
  return ret;
}

void TES902Component::loop() {
  static char buffer[MAX_READ_LENGTH];
  while (this->available() != 0) {
    if (readmsg_(read(), buffer, MAX_READ_LENGTH) > 0) {
      // ESP_LOGD(TAG, "TES Read loop");
      // for (int i = 0; i < sizeof(buffer); ++i) {
      //           ESP_LOGD(TAG, "%02X ", buffer[i]);
      //       }
      if (buffer[2] == 0x15) {
        int co2 = buffer[4] + buffer[5] * 256;
        // ESP_LOGD(TAG, "CO2: %d ", co2);
        if (this->co2_sensor_ != nullptr) {
          co2_sensor_->publish_state(co2);
        }
      }
    }
  }
}

float TES902Component::get_setup_priority() const { return setup_priority::DATA; }

void TES902Component::dump_config() {
  ESP_LOGCONFIG(TAG, "TES902:");
  LOG_UPDATE_INTERVAL(this);
  this->check_uart_settings(9600);
}

enum class ReadState {
  WAIT_SYNC1,   // Waiting for 0xBB
  WAIT_SYNC2,   // Waiting for 0x66 after 0xBB
  WAIT_TYPE,    // Waiting for response type byte
  WAIT_LENGTH,  // Waiting for length byte
  WAIT_DATA,    // Collecting data bytes based on length
  WAIT_CRC1,    // Waiting for the first CRC byte
  WAIT_CRC2,    // Waiting for the second CRC byte
};

int TES902Component::readmsg_(int readch, char *buffer, int len) {
  static int pos = 0;
  static int data_len = 0;
  static ReadState state = ReadState::WAIT_SYNC1;
  static uint16_t received_crc = 0;

  if (readch > 0) {
    switch (state) {
      case ReadState::WAIT_SYNC1:
        if (readch == 0xBB) {
          buffer[pos++] = readch;
          state = ReadState::WAIT_SYNC2;
        } else {
          pos = 0;  // Reset buffer if any other data is encountered before sync byte
        }
        break;

      case ReadState::WAIT_SYNC2:
        if (readch == 0x66) {
          buffer[pos++] = readch;
          state = ReadState::WAIT_TYPE;
        } else {
          pos = 0;
          state = ReadState::WAIT_SYNC1;  // Start over if second sync byte is incorrect
        }
        break;

      case ReadState::WAIT_TYPE:
        buffer[pos++] = readch;
        state = ReadState::WAIT_LENGTH;
        break;

      case ReadState::WAIT_LENGTH:
        data_len = readch;
        buffer[pos++] = readch;
        if (data_len > 0) {
          state = ReadState::WAIT_DATA;
        } else {
          state = ReadState::WAIT_CRC1;  // If no data length, skip to CRC
        }
        break;

      case ReadState::WAIT_DATA:
        buffer[pos++] = readch;
        data_len--;
        if (data_len <= 0) {
          state = ReadState::WAIT_CRC1;  // Move to CRC after collecting all data
        }
        break;

      case ReadState::WAIT_CRC1:
        received_crc = readch;  // First CRC byte (most significant byte)
        state = ReadState::WAIT_CRC2;
        break;

      case ReadState::WAIT_CRC2:
        received_crc |= readch << 8;  // Combine with the second CRC byte (least significant byte)
        // for (int i = 0; i < pos - 0; i++) {
        //   ESP_LOGV(TAG, "Data Byte[%d]: %02X", i-2, static_cast<unsigned char>(buffer[i]));
        // }
        uint16_t computed_crc =
            Calculate_CRC16(reinterpret_cast<uint8_t *>(buffer + 0), pos - 0);  // Compute CRC from type byte to data

        if (received_crc == computed_crc) {
          int rpos = pos;
          pos = 0;
          state = ReadState::WAIT_SYNC1;  // Reset for the next message
          return rpos;                    // Return the position, indicating the message is valid and complete
        } else {
          // Invalid CRC, reset and start looking for a new message
          ESP_LOGD(TAG, "Invalid CRC - Computed: %04X, Received: %04X", computed_crc, received_crc);
          pos = 0;
          state = ReadState::WAIT_SYNC1;
        }
        break;
    }
  }

  // Return -1 if the end of the message hasn't been found.
  return -1;
}

}  // namespace tes902
}  // namespace esphome
