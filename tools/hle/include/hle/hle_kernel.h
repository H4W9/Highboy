#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void hle_kernel_init(void);
void hle_kernel_shutdown(void);
void hle_set_button_mask(unsigned char mask);

#ifdef __cplusplus
}
#endif
