#pragma once

#include <pthread.h>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <queue>
#include <vector>

namespace hle {

static constexpr size_t SPI_FRAME_SIZE = 260;  // 4 header + 256 payload
static constexpr size_t SPI_HEADER_SIZE = 4;
static constexpr size_t SPI_MAX_PAYLOAD = 256;
static constexpr uint8_t SPI_SYNC_BYTE = 0xAA;
static constexpr size_t STREAM_QUEUE_LEN = 8;

struct SPIFrame {
    uint8_t sync;
    uint8_t type;
    uint8_t id;
    uint8_t length;
    uint8_t payload[SPI_MAX_PAYLOAD];

    SPIFrame() : sync(0), type(0), id(0), length(0) { memset(payload, 0, sizeof(payload)); }

    bool is_valid() const { return sync == SPI_SYNC_BYTE; }

    void build_header(uint8_t frame_type, uint8_t cmd_id, uint8_t payload_len) {
        sync = SPI_SYNC_BYTE;
        type = frame_type;
        id = cmd_id;
        length = payload_len;
    }
};

class SPIBridgeChannel {
public:
    SPIBridgeChannel();
    ~SPIBridgeChannel();

    // ── Master (P4) side ─────────────────────────────────────────────────

    void master_send_command(uint8_t cmd_id, const uint8_t *payload, uint8_t payload_len);
    bool master_receive_response(uint8_t &out_id, uint8_t *out_payload, uint8_t &out_len,
                                 uint32_t timeout_ms);

    // ── Slave (C5) side ──────────────────────────────────────────────────

    bool slave_wait_command(uint8_t &out_cmd_id, uint8_t *out_payload, uint8_t &out_len);
    void slave_send_response(uint8_t cmd_id, uint8_t status, const uint8_t *payload,
                             uint8_t payload_len);
    void slave_send_stream(uint8_t stream_id, const uint8_t *payload, uint8_t payload_len);

    // ── Stream queue (C5 → P4) ───────────────────────────────────────────

    bool stream_push(uint8_t stream_id, const uint8_t *data, size_t len);
    bool stream_pop(uint8_t &out_stream_id, uint8_t *out_data, size_t &out_len);

    // ── IRQ simulation ───────────────────────────────────────────────────

    void slave_notify_irq();
    bool master_wait_irq(uint32_t timeout_ms);

    // ── Management ───────────────────────────────────────────────────────

    void reset();
    void close();
    bool is_closed() const { return s_closed; }

private:
    bool s_closed = false;

    // Master → Slave command queue
    SPIFrame s_command_frame;
    bool s_has_command = false;
    std::mutex s_cmd_mutex;
    std::condition_variable s_cmd_cv;

    // Slave → Master response
    SPIFrame s_response_frame;
    bool s_has_response = false;
    std::mutex s_resp_mutex;
    std::condition_variable s_resp_cv;

    // IRQ flag
    bool s_irq_raised = false;
    std::mutex s_irq_mutex;
    std::condition_variable s_irq_cv;

    // Stream queue (C5 pushes, P4 pops)
    struct StreamItem {
        uint8_t stream_id;
        uint8_t data[SPI_MAX_PAYLOAD];
        size_t len;
    };
    std::deque<StreamItem> s_stream_queue;
    std::mutex s_stream_mutex;
};

}  // namespace hle

// Global bridge channel pointer for firmware shims
extern "C" {
void hle_set_bridge_channel(hle::SPIBridgeChannel *ch);
hle::SPIBridgeChannel *hle_get_bridge_channel(void);
}
