#include <gtest/gtest.h>
#include <thread>
#include "hle/spi_bridge_channel.h"

using namespace hle;

TEST(SPIBridgeChannel, PingPong) {
    SPIBridgeChannel ch;

    // Slave thread: waits for command, responds
    std::thread slave([&ch]() {
        uint8_t cmd_id, payload[256], len;
        EXPECT_TRUE(ch.slave_wait_command(cmd_id, payload, len));
        EXPECT_EQ(cmd_id, 0x01);  // SYSTEM_PING
        ch.slave_send_response(cmd_id, 0x00, nullptr, 0);  // OK
    });

    // Master: sends command, waits for response
    ch.master_send_command(0x01, nullptr, 0);
    uint8_t resp_id, resp_payload[256], resp_len;
    EXPECT_TRUE(ch.master_receive_response(resp_id, resp_payload, resp_len, 1000));
    EXPECT_EQ(resp_id, 0x01);
    EXPECT_EQ(resp_len, 1u);      // 1 byte status
    EXPECT_EQ(resp_payload[0], 0x00);  // SPI_STATUS_OK

    slave.join();
}

TEST(SPIBridgeChannel, PayloadRoundTrip) {
    SPIBridgeChannel ch;

    uint8_t test_data[64];
    for (int i = 0; i < 64; i++) test_data[i] = (uint8_t)i;

    std::thread slave([&ch, &test_data]() {
        uint8_t cmd_id, payload[256], len;
        EXPECT_TRUE(ch.slave_wait_command(cmd_id, payload, len));
        EXPECT_EQ(len, 64u);
        EXPECT_EQ(memcmp(payload, test_data, 64), 0);

        // Echo back
        ch.slave_send_response(cmd_id, 0x00, payload, len);
    });

    ch.master_send_command(0x10, test_data, 64);
    uint8_t resp_id, resp_payload[256], resp_len;
    EXPECT_TRUE(ch.master_receive_response(resp_id, resp_payload, resp_len, 1000));
    EXPECT_EQ(resp_len, 65u);  // 1 status + 64 payload
    EXPECT_EQ(resp_payload[0], 0x00);
    EXPECT_EQ(memcmp(resp_payload + 1, test_data, 64), 0);

    slave.join();
}

TEST(SPIBridgeChannel, TimeoutWhenNoResponse) {
    SPIBridgeChannel ch;

    ch.master_send_command(0x01, nullptr, 0);
    uint8_t resp_id, resp_payload[256], resp_len;
    EXPECT_FALSE(ch.master_receive_response(resp_id, resp_payload, resp_len, 100));

    // Wait for the dangling command to be consumed by the channel
    // (no slave to consume it, so we just reset)
}

TEST(SPIBridgeChannel, StreamPushPop) {
    SPIBridgeChannel ch;

    uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    EXPECT_TRUE(ch.stream_push(0x25, data, 4));
    EXPECT_TRUE(ch.stream_push(0x25, data, 4));
    EXPECT_TRUE(ch.stream_push(0x25, data, 4));

    uint8_t out_data[256];
    size_t out_len;
    uint8_t out_id;

    EXPECT_TRUE(ch.stream_pop(out_id, out_data, out_len));
    EXPECT_EQ(out_id, 0x25);
    EXPECT_EQ(out_len, 4u);

    EXPECT_TRUE(ch.stream_pop(out_id, out_data, out_len));
    EXPECT_TRUE(ch.stream_pop(out_id, out_data, out_len));
    EXPECT_FALSE(ch.stream_pop(out_id, out_data, out_len));  // empty
}

TEST(SPIBridgeChannel, StreamOverflow) {
    SPIBridgeChannel ch;

    uint8_t data[4] = {1, 2, 3, 4};
    for (int i = 0; i < 8; i++) {
        EXPECT_TRUE(ch.stream_push(0x25, data, 4));
    }
    // 9th should fail
    EXPECT_FALSE(ch.stream_push(0x25, data, 4));
}

TEST(SPIBridgeChannel, IRQSimulation) {
    SPIBridgeChannel ch;

    // Slave raises IRQ
    std::thread slave([&ch]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ch.slave_notify_irq();
    });

    // Master waits for IRQ
    EXPECT_TRUE(ch.master_wait_irq(1000));

    slave.join();

    // Second wait without new IRQ should timeout
    EXPECT_FALSE(ch.master_wait_irq(100));
}
