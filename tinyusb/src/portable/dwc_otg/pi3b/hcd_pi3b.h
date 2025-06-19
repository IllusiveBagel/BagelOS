#ifndef HCD_PI3B_H
#define HCD_PI3B_H

#include <stdbool.h>
#include <stdint.h>
#include "tusb_option.h"

// TinyUSB Host Controller Driver API for Raspberry Pi 3B DWC OTG

#ifdef __cplusplus
extern "C"
{
#endif

    bool hcd_init(uint8_t rhport);
    void hcd_int_enable(uint8_t rhport);
    void hcd_int_disable(uint8_t rhport);
    uint32_t hcd_frame_number(uint8_t rhport);
    uint32_t hcd_port_connect_status(uint8_t rhport);
    void hcd_port_reset(uint8_t rhport);
    uint8_t hcd_port_speed_get(uint8_t rhport);

    bool hcd_pipe_open(uint8_t rhport, uint8_t dev_addr, uint8_t ep_addr, uint8_t ep_type, uint16_t max_packet_size);
    bool hcd_pipe_close(uint8_t rhport, uint8_t pipe_hdl);
    bool hcd_pipe_xfer(uint8_t rhport, uint8_t pipe_hdl, uint8_t *buffer, uint16_t buflen, bool direction);
    void hcd_pipe_abort(uint8_t rhport, uint8_t pipe_hdl);

    void hcd_int_handler(uint8_t rhport);

#ifdef __cplusplus
}
#endif

#endif // HCD_PI3B_H