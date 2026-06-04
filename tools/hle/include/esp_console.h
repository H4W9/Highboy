#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

void *esp_console_register_help_command(void);
esp_err_t esp_console_init(const void *config);
esp_err_t esp_console_cmd_register(const void *cmd);
esp_err_t esp_console_start(void);

#ifdef __cplusplus
}
#endif
