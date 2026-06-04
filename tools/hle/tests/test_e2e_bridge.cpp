#include <gtest/gtest.h>
#include <thread>
#include <cstring>
#include "hle/spi_bridge_channel.h"
#include "hle/cc1101_emu.h"

using namespace hle;

extern "C" {
#include "esp_err.h"
void hle_set_bridge_channel(hle::SPIBridgeChannel *ch);
}

TEST(E2EBridge, SystemPingViaBridge) {
    SPIBridgeChannel ch;
    hle_set_bridge_channel(&ch);

    // Emulate C5 side (normally runs in bridge_task)
    std::thread c5_side([&ch]() {
        uint8_t cmd_id, payload[256], payload_len;
        ASSERT_TRUE(ch.slave_wait_command(cmd_id, payload, payload_len));

        // Handle SYSTEM_PING
        if (cmd_id == 0x01) {
            ch.slave_send_response(cmd_id, 0x00, nullptr, 0);
            ch.slave_notify_irq();
        }
    });

    // P4 side: send ping command
    ch.master_send_command(0x01, nullptr, 0);
    ASSERT_TRUE(ch.master_wait_irq(1000));

    uint8_t resp_id, resp_payload[256], resp_len;
    ASSERT_TRUE(ch.master_receive_response(resp_id, resp_payload, resp_len, 100));
    EXPECT_EQ(resp_id, 0x01);
    EXPECT_GE(resp_len, 1u);
    EXPECT_EQ(resp_payload[0], 0x00); // OK

    c5_side.join();
}

TEST(E2EBridge, SystemStatus) {
    SPIBridgeChannel ch;
    hle_set_bridge_channel(&ch);

    std::thread c5_side([&ch]() {
        uint8_t cmd_id, payload[256], payload_len;
        ASSERT_TRUE(ch.slave_wait_command(cmd_id, payload, payload_len));

        // Emulate system status response with mock data
        if (cmd_id == 0x02) {
            // STATUS response: [status_byte, wifi_active, bt_running, firmware_ver_major, minor, patch]
            uint8_t status_data[] = {0x01, 0x00, 0x01, 0x00, 0x05};  // wifi inactive, bt running, v1.0.5
            ch.slave_send_response(cmd_id, 0x00, status_data, sizeof(status_data));
            ch.slave_notify_irq();
        }
    });

    ch.master_send_command(0x02, nullptr, 0);
    ASSERT_TRUE(ch.master_wait_irq(1000));

    uint8_t resp_id, resp_payload[256], resp_len;
    ASSERT_TRUE(ch.master_receive_response(resp_id, resp_payload, resp_len, 100));
    EXPECT_EQ(resp_id, 0x02);
    EXPECT_EQ(resp_payload[0], 0x00); // OK
    // status_data follows at offset 1
    EXPECT_EQ(resp_payload[1], 0x01); // wifi_active
    EXPECT_EQ(resp_payload[2], 0x00); // bt_running

    c5_side.join();
}

TEST(E2EBridge, WiFiScanDataFetch) {
    SPIBridgeChannel ch;
    hle_set_bridge_channel(&ch);

    const int ap_count = 3;

    std::thread c5_side([&ch, ap_count]() {
        uint8_t cmd_id, payload[256], payload_len;
        ASSERT_TRUE(ch.slave_wait_command(cmd_id, payload, payload_len));

        // Respond with count
        if (cmd_id == 0x20) {
            uint8_t count[2] = {uint8_t(ap_count), 0};
            ch.slave_send_response(cmd_id, 0x00, count, 2);
            ch.slave_notify_irq();
        }
    });

    ch.master_send_command(0x20, nullptr, 0);
    ASSERT_TRUE(ch.master_wait_irq(1000));

    uint8_t resp_id, resp_payload[256], resp_len;
    ASSERT_TRUE(ch.master_receive_response(resp_id, resp_payload, resp_len, 200));
    EXPECT_EQ(resp_id, 0x20);
    EXPECT_EQ(resp_payload[1], (uint8_t)ap_count);

    c5_side.join();
}

TEST(E2EBridge, StreamBackpressure) {
    SPIBridgeChannel ch;
    hle_set_bridge_channel(&ch);

    uint8_t data[64] = {};
    for (int i = 0; i < 64; i++) data[i] = (uint8_t)i;

    // Fill stream queue to capacity (8)
    for (int i = 0; i < 8; i++) {
        ASSERT_TRUE(ch.stream_push(0x25, data, 64));
    }
    // 9th should fail
    EXPECT_FALSE(ch.stream_push(0x25, data, 64));

    // Drain one
    uint8_t out_id, out_data[256];
    size_t out_len;
    ASSERT_TRUE(ch.stream_pop(out_id, out_data, out_len));

    // Now should accept one more
    EXPECT_TRUE(ch.stream_push(0x25, data, 64));
}
