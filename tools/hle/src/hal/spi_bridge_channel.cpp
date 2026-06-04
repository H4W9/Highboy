#include "hle/spi_bridge_channel.h"

namespace hle {

SPIBridgeChannel::SPIBridgeChannel() = default;

SPIBridgeChannel::~SPIBridgeChannel() {
    slave_notify_irq(); // unblock any waiting master
}

void SPIBridgeChannel::reset() {
    s_has_command = false;
    s_has_response = false;
    s_irq_raised = false;
    s_cmd_cv.notify_all();
    s_resp_cv.notify_all();
    s_irq_cv.notify_all();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Master (P4) side
// ═══════════════════════════════════════════════════════════════════════════════

void SPIBridgeChannel::master_send_command(uint8_t cmd_id, const uint8_t *payload,
                                            uint8_t payload_len) {
    std::lock_guard<std::mutex> lock(s_cmd_mutex);
    s_command_frame.build_header(0x01, cmd_id, payload_len);
    if (payload && payload_len > 0) {
        memcpy(s_command_frame.payload, payload, payload_len);
    }
    s_has_command = true;
    s_cmd_cv.notify_all();
}

bool SPIBridgeChannel::master_receive_response(uint8_t &out_id, uint8_t *out_payload,
                                                uint8_t &out_len, uint32_t timeout_ms) {
    std::unique_lock<std::mutex> lock(s_resp_mutex);
    if (!s_has_response) {
        s_resp_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms));
    }
    if (!s_has_response) return false;

    out_id = s_response_frame.id;
    out_len = s_response_frame.length;
    if (out_payload && out_len > 0) {
        memcpy(out_payload, s_response_frame.payload, out_len);
    }
    s_has_response = false;
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Slave (C5) side
// ═══════════════════════════════════════════════════════════════════════════════

bool SPIBridgeChannel::slave_wait_command(uint8_t &out_cmd_id, uint8_t *out_payload,
                                           uint8_t &out_len) {
    std::unique_lock<std::mutex> lock(s_cmd_mutex);
    s_cmd_cv.wait(lock, [this] { return s_has_command; });
    if (!s_has_command) return false;

    out_cmd_id = s_command_frame.id;
    out_len = s_command_frame.length;
    if (out_payload && out_len > 0) {
        memcpy(out_payload, s_command_frame.payload, out_len);
    }
    s_has_command = false;
    return true;
}

void SPIBridgeChannel::slave_send_response(uint8_t cmd_id, uint8_t status,
                                            const uint8_t *payload, uint8_t payload_len) {
    std::lock_guard<std::mutex> lock(s_resp_mutex);
    s_response_frame.build_header(0x02, cmd_id, payload_len + 1);
    s_response_frame.payload[0] = status;
    if (payload && payload_len > 0) {
        memcpy(s_response_frame.payload + 1, payload, payload_len);
    }
    s_has_response = true;
    s_resp_cv.notify_all();
}

void SPIBridgeChannel::slave_send_stream(uint8_t stream_id, const uint8_t *payload,
                                          uint8_t payload_len) {
    stream_push(stream_id, payload, payload_len);
    slave_notify_irq();
}

// ═══════════════════════════════════════════════════════════════════════════════
// Stream queue
// ═══════════════════════════════════════════════════════════════════════════════

bool SPIBridgeChannel::stream_push(uint8_t stream_id, const uint8_t *data, size_t len) {
    std::lock_guard<std::mutex> lock(s_stream_mutex);
    if (s_stream_queue.size() >= STREAM_QUEUE_LEN) return false;
    if (len > SPI_MAX_PAYLOAD) len = SPI_MAX_PAYLOAD;

    StreamItem item;
    item.stream_id = stream_id;
    item.len = len;
    memcpy(item.data, data, len);
    s_stream_queue.push_back(item);
    return true;
}

bool SPIBridgeChannel::stream_pop(uint8_t &out_stream_id, uint8_t *out_data, size_t &out_len) {
    std::lock_guard<std::mutex> lock(s_stream_mutex);
    if (s_stream_queue.empty()) return false;

    auto &item = s_stream_queue.front();
    out_stream_id = item.stream_id;
    out_len = item.len;
    if (out_data) memcpy(out_data, item.data, item.len);
    s_stream_queue.pop_front();
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// IRQ simulation
// ═══════════════════════════════════════════════════════════════════════════════

void SPIBridgeChannel::slave_notify_irq() {
    std::lock_guard<std::mutex> lock(s_irq_mutex);
    s_irq_raised = true;
    s_irq_cv.notify_all();
}

bool SPIBridgeChannel::master_wait_irq(uint32_t timeout_ms) {
    std::unique_lock<std::mutex> lock(s_irq_mutex);
    if (!s_irq_raised) {
        s_irq_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms));
    }
    bool raised = s_irq_raised;
    s_irq_raised = false;
    return raised;
}

}  // namespace hle
