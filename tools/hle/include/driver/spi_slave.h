#pragma once

#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int spi_slave_transaction_t;

typedef struct {
    int spics_io_num;
    int queue_size;
    int mode;
} spi_slave_interface_config_t;

esp_err_t spi_slave_initialize(int host, const void *bus, const spi_slave_interface_config_t *cfg, int dma);
esp_err_t spi_slave_transmit(int host, void *tx, void *rx, size_t len, int timeout);

#ifdef __cplusplus
}
#endif
