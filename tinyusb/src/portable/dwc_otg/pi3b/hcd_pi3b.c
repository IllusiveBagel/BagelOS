#include "hcd_pi3b.h"
#include "tusb_option.h"
#include "tusb.h"
#include <stdint.h>
#include <stdint.h>
#include <stddef.h>

// Include your hardware register definitions
#include "pi3b_hw.h"

// TinyUSB Host Controller Driver API implementation for Raspberry Pi 3B DWC OTG

#define DWC_OTG_MAX_CHANNELS 8
#define MAX_RETRIES 3
#define TRANSFER_TIMEOUT_MS 1000 // Example timeout
#define DWC_OTG_GAHBCFG (*(volatile uint32_t *)(DWC_OTG_BASE + 0x008))

typedef struct
{
    bool used;
    uint8_t dev_addr;
    uint8_t ep_addr;
    uint8_t ep_type;
    uint16_t max_packet_size;
    uint8_t *buffer;
    uint16_t buflen;
    uint16_t xferred;
    bool direction; // 0 = OUT, 1 = IN
    uint16_t bytes_written;
    uint16_t bytes_read;
    bool zlp_sent;
    uint8_t retry_count;
    uint32_t last_activity_tick; // For timeouts
    uint8_t fallback_buf[4096] __attribute__((aligned(4)));
    bool use_fallback;
} channel_t;

static channel_t channels[DWC_OTG_MAX_CHANNELS];

static uint8_t dma_fallback_buf[4096] __attribute__((aligned(4)));

#define CACHE_LINE_SIZE 64

static inline void *align_down(void *addr, size_t align)
{
    return (void *)((uintptr_t)addr & ~(align - 1));
}
static inline size_t align_up_size(void *addr, size_t size, size_t align)
{
    uintptr_t start = (uintptr_t)addr;
    uintptr_t end = (start + size + align - 1) & ~(align - 1);
    return end - (start & ~(align - 1));
}

// Helper: simple delay (replace with timer if available)
static void delay(int count)
{
    volatile int i;
    for (i = 0; i < count * 1000; i++)
        ;
}

bool hcd_init(uint8_t rhport)
{
    // 1. Soft reset the core
    DWC_OTG_GRSTCTL |= (1 << 0); // Core soft reset
    while (DWC_OTG_GRSTCTL & (1 << 0))
        ; // Wait for reset to complete

    // 2. Force host mode
    DWC_OTG_GUSBCFG |= (1 << 29); // Force host mode

    // 3. Power up the port
    DWC_OTG_HPRT |= (1 << 12); // Power port

    // 4. Configure host configuration register
    DWC_OTG_HCFG = 1; // FS/LS PHY clock select

    DWC_OTG_GAHBCFG |= (1 << 5); // Enable DMA (bit 5)

    delay(100); // Wait for hardware to settle

    // 6. Return true if successful
    return true;
}

void hcd_int_enable(uint8_t rhport)
{
    // Enable only the most relevant interrupts for USB host operation.
    // Reference: BCM2835 ARM Peripherals manual, DWC OTG section.

    // Example: Enable interrupts for USB reset, port change, and host channel (transfer complete)
    // Bit definitions (see DWC OTG documentation):
    //  - 0: Current Mode of Operation
    //  - 18: USB Reset
    //  - 24: Port Interrupt
    //  - 25: Host Channels Interrupt

    // Enable USB Reset (bit 12), Port Interrupt (bit 24), Host Channels Interrupt (bit 25)
    DWC_OTG_GINTMSK = (1 << 12) | (1 << 24) | (1 << 25);

    // If your platform requires enabling the USB interrupt in the ARM interrupt controller,
    // you would do that here (platform-specific, not shown).
}

void hcd_int_disable(uint8_t rhport)
{
    // Disable all USB interrupts for the DWC OTG controller.
    DWC_OTG_GINTMSK = 0;

    // If your platform requires disabling the USB interrupt in the ARM interrupt controller,
    // you would do that here (platform-specific, not shown).
}

uint32_t hcd_frame_number(uint8_t rhport)
{
    // The frame number is stored in the HFNUM register (Host Frame Number/Frame Time Remaining)
    // Bits [15:0] are the frame number.
    // Reference: BCM2835 ARM Peripherals manual, DWC OTG section.

#define DWC_OTG_HFNUM (*(volatile uint32_t *)(DWC_OTG_BASE + 0x408))

    return DWC_OTG_HFNUM & 0xFFFF;
}

uint32_t hcd_port_connect_status(uint8_t rhport)
{
    // The port connect status is bit 0 of the HPRT register.
    // 1 = device connected, 0 = not connected.
    return (DWC_OTG_HPRT & 0x1) ? 1 : 0;
}

void hcd_port_reset(uint8_t rhport)
{
    // Set the Port Reset bit (bit 8) in HPRT to initiate a reset.
    DWC_OTG_HPRT |= (1 << 8);

    // Wait for at least 50ms (USB spec requires 10ms minimum, 50ms is safe)
    for (volatile int i = 0; i < 500000; i++)
        ;

    // Clear the Port Reset bit to complete the reset
    DWC_OTG_HPRT &= ~(1 << 8);

    // Wait a short time for the port to stabilize
    for (volatile int i = 0; i < 100000; i++)
        ;
}

uint8_t hcd_port_speed_get(uint8_t rhport)
{
    // Bits 17:16 of HPRT indicate the port speed:
    // 00: High speed, 01: Full speed, 10: Low speed, 11: Undefined
    uint32_t speed = (DWC_OTG_HPRT >> 17) & 0x3;

    switch (speed)
    {
    case 0x0:
        return TUSB_SPEED_HIGH;
    case 0x1:
        return TUSB_SPEED_FULL;
    case 0x2:
        return TUSB_SPEED_LOW;
    default:
        return TUSB_SPEED_FULL; // Fallback
    }
}

// Pipe (endpoint/channel) management
int hcd_pipe_open(uint8_t rhport, uint8_t dev_addr, uint8_t ep_addr, uint8_t ep_type, uint16_t max_packet_size)
{
    // Search for a free channel
    for (int ch = 0; ch < DWC_OTG_MAX_CHANNELS; ch++)
    {
        if (!channels[ch].used)
        {
            channels[ch].used = true;
            channels[ch].dev_addr = dev_addr;
            channels[ch].ep_addr = ep_addr;
            channels[ch].ep_type = ep_type;
            channels[ch].max_packet_size = max_packet_size;

            // Configure hardware registers for this channel
            volatile uint32_t *HCCHAR = (volatile uint32_t *)(DWC_OTG_BASE + 0x500 + 0x20 * ch);

            uint32_t hcchar = 0;
            hcchar |= (ep_addr & 0x7F);                 // Endpoint number (bits 0-3)
            hcchar |= ((dev_addr & 0x7F) << 22);        // Device address (bits 22-28)
            hcchar |= ((max_packet_size & 0x7FF) << 0); // Max packet size (bits 0-10)

            // Set endpoint direction
            if (ep_addr & 0x80)
            { // IN endpoint
                hcchar |= (1 << 15);
            }

            // Set endpoint type
            switch (ep_type)
            {
            case TUSB_XFER_CONTROL:
                hcchar |= (0 << 18);
                break;
            case TUSB_XFER_ISOCHRONOUS:
                hcchar |= (1 << 18);
                break;
            case TUSB_XFER_BULK:
                hcchar |= (2 << 18);
                break;
            case TUSB_XFER_INTERRUPT:
                hcchar |= (3 << 18);
                break;
            }

            *HCCHAR = hcchar;

            // Enable interrupts for this channel (transfer complete, errors, etc.)
            volatile uint32_t *HCINTMSK = (volatile uint32_t *)(DWC_OTG_BASE + 0x508 + 0x20 * ch);
            *HCINTMSK = 0x3F; // Enable all basic interrupts for demonstration

            return ch; // Return channel number as pipe handle
        }
    }
    return -1; // No free channel
}

bool hcd_pipe_close(uint8_t rhport, uint8_t pipe_hdl)
{
    if (pipe_hdl < DWC_OTG_MAX_CHANNELS && channels[pipe_hdl].used)
    {
        channels[pipe_hdl].used = false;

        // Disable the channel in hardware
        volatile uint32_t *HCCHAR = (volatile uint32_t *)(DWC_OTG_BASE + 0x500 + 0x20 * pipe_hdl);
        *HCCHAR |= (1 << 30);  // Set CHDIS to disable the channel
        *HCCHAR &= ~(1 << 31); // Clear CHENA to ensure channel is not enabled

        // Clear any pending interrupts for this channel
        volatile uint32_t *HCINT = (volatile uint32_t *)(DWC_OTG_BASE + 0x50C + 0x20 * pipe_hdl);
        *HCINT = 0xFFFFFFFF; // Clear all pending interrupts

        // Clear the interrupt mask for this channel
        volatile uint32_t *HCINTMSK = (volatile uint32_t *)(DWC_OTG_BASE + 0x508 + 0x20 * pipe_hdl);
        *HCINTMSK = 0;

        return true;
    }
    return false;
}

void cache_clean(void *addr, size_t size)
{
    uintptr_t start = (uintptr_t)align_down(addr, CACHE_LINE_SIZE);
    uintptr_t end = (uintptr_t)addr + size;

    for (uintptr_t p = start; p < end; p += CACHE_LINE_SIZE)
    {
        __asm__ volatile("dc civac, %0" ::"r"(p) : "memory");
    }
    __asm__ volatile("dsb sy" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}

void cache_invalidate(void *addr, size_t size)
{
    uintptr_t start = (uintptr_t)align_down(addr, CACHE_LINE_SIZE);
    uintptr_t end = (uintptr_t)addr + size;

    for (uintptr_t p = start; p < end; p += CACHE_LINE_SIZE)
    {
        __asm__ volatile("dc ivac, %0" ::"r"(p) : "memory");
    }
    __asm__ volatile("dsb sy" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}

bool hcd_pipe_xfer(uint8_t rhport, uint8_t pipe_hdl, uint8_t *buffer, uint16_t buflen, bool direction)
{
    if (pipe_hdl >= DWC_OTG_MAX_CHANNELS || !channels[pipe_hdl].used)
        return false;

    channel_t *ch = &channels[pipe_hdl];
    ch->buflen = buflen;
    ch->xferred = 0;
    ch->direction = direction;
    ch->bytes_written = 0;
    ch->bytes_read = 0;
    ch->retry_count = 0;
    ch->zlp_sent = false;
    ch->last_activity_tick = board_millis();
    ch->use_fallback = false;
    ch->user_buffer = buffer;

    // Register pointers
    volatile uint32_t *HCCHAR = (volatile uint32_t *)(DWC_OTG_BASE + 0x500 + 0x20 * pipe_hdl);
    volatile uint32_t *HCTSIZ = (volatile uint32_t *)(DWC_OTG_BASE + 0x510 + 0x20 * pipe_hdl);
    volatile uint32_t *HCDMA = (volatile uint32_t *)(DWC_OTG_BASE + 0x514 + 0x20 * pipe_hdl);

    uint16_t mps = ch->max_packet_size;
    uint32_t num_packets = (buflen + mps - 1) / mps;
    if (num_packets == 0)
        num_packets = 1;

    *HCTSIZ = (buflen & 0x7FFFF) | ((num_packets & 0x3FF) << 19) | (0 << 29); // DATA0 PID

    // DMA fallback logic (per-channel)
    uint8_t *dma_buf = buffer;
    if (((uintptr_t)buffer % 4) != 0)
    {
        if (buflen > sizeof(ch->fallback_buf))
            return false; // Too large for fallback
        if (!direction)
        {
            memcpy(ch->fallback_buf, buffer, buflen); // OUT: copy to fallback
        }
        dma_buf = ch->fallback_buf;
        ch->use_fallback = true;
    }
    ch->buffer = dma_buf;

    // --- Cache management before DMA OUT ---
    if (!direction)
    {
        cache_clean(dma_buf, buflen);
    }

    *HCDMA = (uint32_t)dma_buf;

    // Set endpoint type in HCCHAR (bits 18:19)
    *HCCHAR = (*HCCHAR & ~(3 << 18)) | (ch->ep_type << 18);

    // Enable the channel: set CHENA (bit 31), clear CHDIS (bit 30)
    *HCCHAR = (*HCCHAR & ~(1 << 30)) | (1 << 31);

    return true;
}

// In your transfer complete event handler (for IN transfers):
void on_dma_in_xfer_complete(uint8_t pipe_hdl, uint32_t actual_len)
{
    channel_t *ch = &channels[pipe_hdl];

    // --- Cache management after DMA IN ---
    if (ch->direction)
    {
        cache_invalidate(ch->buffer, actual_len);
    }

    // If fallback buffer was used, copy data back to the user's original buffer
    if (ch->use_fallback && ch->direction)
    {
        memcpy(ch->user_buffer, ch->fallback_buf, actual_len);
    }

    // Notify TinyUSB of transfer completion
    hcd_event_xfer_complete(pipe_hdl, actual_len, true);

    // Reset fallback usage for next transfer
    ch->use_fallback = false;
}

void hcd_pipe_abort(uint8_t rhport, uint8_t pipe_hdl)
{
    // This example assumes pipe_hdl is the channel number.
    // To abort a transfer, set the Channel Disable (CHDIS) bit in the HCCHAR register.

    volatile uint32_t *HCCHAR = (volatile uint32_t *)(DWC_OTG_BASE + 0x500 + 0x20 * pipe_hdl);

    // Set the Channel Disable (CHDIS) bit (bit 30)
    *HCCHAR |= (1 << 30);

    // Optionally, clear any pending interrupts for this channel
    volatile uint32_t *HCINT = (volatile uint32_t *)(DWC_OTG_BASE + 0x50C + 0x20 * pipe_hdl);
    *HCINT = 0xFFFFFFFF; // Clear all pending interrupts

    // Optionally, clear the interrupt mask for this channel
    volatile uint32_t *HCINTMSK = (volatile uint32_t *)(DWC_OTG_BASE + 0x508 + 0x20 * pipe_hdl);
    *HCINTMSK = 0;
}

extern uint32_t board_millis(void);

// Interrupt handler (call this from your USB IRQ handler)
void hcd_int_handler(uint8_t rhport)
{
    uint32_t gintsts = DWC_OTG_GINTSTS;
    uint32_t gintmsk = DWC_OTG_GINTMSK;
    uint32_t pending = gintsts & gintmsk;

    // --- Device connect/disconnect and VBUS power management ---
    if (pending & (1 << 24))
    { // Port interrupt (GINTSTS bit 24)
        volatile uint32_t *HPRT = (volatile uint32_t *)(DWC_OTG_BASE + 0x440);
        uint32_t hprt = *HPRT;

        if (hprt & (1 << 3))
        { // PRTCONNDET: Port Connect Detected (change)
            if (hprt & (1 << 0))
            {
                // Device connected
                hcd_event_device_attach(0, true); // Notify TinyUSB
                *HPRT = hprt;                     // Clear change bit by writing 1
                *HPRT |= (1 << 12);               // Power on VBUS (PRTPWR)
            }
            else
            {
                // Device disconnected
                hcd_event_device_remove(0, true); // Notify TinyUSB
                *HPRT = hprt;                     // Clear change bit by writing 1
                *HPRT &= ~(1 << 12);              // Power off VBUS (PRTPWR)
                // Optionally: reset/cleanup all channels here
                for (int ch = 0; ch < DWC_OTG_MAX_CHANNELS; ch++)
                {
                    if (channels[ch].used)
                    {
                        hcd_pipe_close(rhport, ch);
                    }
                }
            }
        }
    }

    // RXFLVL (Receive FIFO Non-Empty) interrupt (bit 4)
    if (pending & (1 << 4))
    {
        volatile uint32_t *GRXSTSR = (volatile uint32_t *)(DWC_OTG_BASE + 0x1C);
        uint32_t rx_status = *GRXSTSR;
        uint32_t ch_num = (rx_status >> 0) & 0xF;
        uint32_t pkt_status = (rx_status >> 17) & 0xF;
        uint32_t byte_count = (rx_status >> 4) & 0x7FF;

        if (pkt_status == 0x2)
        { // IN data packet received
            volatile uint32_t *HCFIFO = (volatile uint32_t *)(DWC_OTG_BASE + 0x1000 + 0x1000 * ch_num);
            uint8_t *buf = channels[ch_num].buffer + channels[ch_num].bytes_read;
            uint32_t bytes_to_read = byte_count;
            while (bytes_to_read >= 4)
            {
                uint32_t word = *HCFIFO;
                for (int j = 0; j < 4; j++)
                {
                    buf[j] = (word >> (8 * j)) & 0xFF;
                }
                buf += 4;
                bytes_to_read -= 4;
                channels[ch_num].bytes_read += 4;
            }
            if (bytes_to_read > 0)
            {
                uint32_t word = *HCFIFO;
                for (uint32_t j = 0; j < bytes_to_read; j++)
                {
                    buf[j] = (word >> (8 * j)) & 0xFF;
                }
                channels[ch_num].bytes_read += bytes_to_read;
            }
            // Short packet handling for IN transfers
            if (byte_count < channels[ch_num].max_packet_size)
            {
                hcd_event_xfer_complete(ch_num, channels[ch_num].bytes_read, true);
                channels[ch_num].retry_count = 0;
                channels[ch_num].last_activity_tick = board_millis();
            }
        }
        // Handle other packet statuses as needed
    }

    // TXFE (Transmit FIFO Empty) interrupt (bit 7)
    if (pending & (1 << 7))
    {
        for (int ch = 0; ch < DWC_OTG_MAX_CHANNELS; ch++)
        {
            if (channels[ch].used && !channels[ch].direction)
            {
                volatile uint32_t *TXFSTS = (volatile uint32_t *)(DWC_OTG_BASE + 0x518 + 0x20 * ch);
                volatile uint32_t *HCFIFO = (volatile uint32_t *)(DWC_OTG_BASE + 0x1000 + 0x1000 * ch);
                uint8_t *buf = channels[ch].buffer + channels[ch].bytes_written;
                uint32_t buflen = channels[ch].buflen;
                uint32_t bytes_left = buflen - channels[ch].bytes_written;
                uint32_t fifo_words = *TXFSTS & 0xFFFF;

                while (fifo_words > 0 && bytes_left > 0)
                {
                    uint32_t word = 0;
                    for (int j = 0; j < 4 && bytes_left > 0; j++, bytes_left--)
                    {
                        word |= (*buf++) << (8 * j);
                        channels[ch].bytes_written++;
                    }
                    *HCFIFO = word;
                    fifo_words--;
                }
                // Zero-length packet handling for OUT transfers
                if (channels[ch].bytes_written >= buflen)
                {
                    if ((buflen % channels[ch].max_packet_size) == 0 && !channels[ch].zlp_sent)
                    {
                        channels[ch].zlp_sent = true;
                        volatile uint32_t *HCTSIZ = (volatile uint32_t *)(DWC_OTG_BASE + 0x510 + 0x20 * ch);
                        *HCTSIZ = (0 & 0x7FFFF) | ((1 & 0x3FF) << 19) | (0 << 29); // 1 packet of 0 bytes
                        volatile uint32_t *HCCHAR = (volatile uint32_t *)(DWC_OTG_BASE + 0x500 + 0x20 * ch);
                        *HCCHAR = (*HCCHAR & ~(1 << 30)) | (1 << 31);
                    }
                    else
                    {
                        DWC_OTG_GINTMSK &= ~(1 << 7);
                    }
                }
            }
        }
    }

    // Host Channel Interrupt (bit 25)
    if (pending & (1 << 25))
    {
        for (int ch = 0; ch < DWC_OTG_MAX_CHANNELS; ch++)
        {
            volatile uint32_t *HCINT = (volatile uint32_t *)(DWC_OTG_BASE + 0x50C + 0x20 * ch);
            volatile uint32_t *HCTSIZ = (volatile uint32_t *)(DWC_OTG_BASE + 0x510 + 0x20 * ch);
            uint32_t hcint = *HCINT;
            if (hcint)
            {
                bool xfer_complete = hcint & (1 << 1); // Xfer complete
                bool stall = hcint & (1 << 3);         // STALL
                bool nak = hcint & (1 << 4);           // NAK
                bool txerr = hcint & (1 << 7);         // TXERR
                bool bberr = hcint & (1 << 8);         // BBERR
                bool frmor = hcint & (1 << 9);         // FRMOR

                uint8_t ep_type = channels[ch].ep_type;

                if (xfer_complete)
                {
                    uint32_t actual_len = channels[ch].buflen - ((*HCTSIZ) & 0x7FFFF);

                    if (ep_type == TUSB_XFER_ISOCHRONOUS)
                    {
                        hcd_event_xfer_complete(ch, actual_len, true);
                        channels[ch].retry_count = 0;
                        channels[ch].last_activity_tick = board_millis();
                    }
                    else if (ep_type == TUSB_XFER_INTERRUPT)
                    {
                        hcd_event_xfer_complete(ch, actual_len, true);
                        channels[ch].retry_count = 0;
                        channels[ch].last_activity_tick = board_millis();
                        channels[ch].last_poll_tick = board_millis();
                    }
                    else
                    {
                        if (!channels[ch].direction && channels[ch].zlp_sent)
                        {
                            channels[ch].zlp_sent = false;
                            hcd_event_xfer_complete(ch, actual_len, true);
                            channels[ch].retry_count = 0;
                            channels[ch].last_activity_tick = board_millis();
                        }
                        else if (channels[ch].direction)
                        {
                            hcd_event_xfer_complete(ch, actual_len, true);
                            channels[ch].retry_count = 0;
                            channels[ch].last_activity_tick = board_millis();
                        }
                    }
                }
                if (stall)
                {
                    hcd_event_xfer_error(ch, true);
                    channels[ch].retry_count = 0;
                }
                if (nak)
                {
                    if (ep_type == TUSB_XFER_ISOCHRONOUS)
                    {
                        hcd_event_xfer_complete(ch, 0, true);
                        channels[ch].retry_count = 0;
                    }
                    else if (ep_type == TUSB_XFER_INTERRUPT)
                    {
                        if (board_millis() - channels[ch].last_poll_tick >= channels[ch].interval_ms)
                        {
                            channels[ch].retry_count++;
                            if (channels[ch].retry_count < MAX_RETRIES)
                            {
                                volatile uint32_t *HCCHAR = (volatile uint32_t *)(DWC_OTG_BASE + 0x500 + 0x20 * ch);
                                *HCCHAR = (*HCCHAR & ~(1 << 30)) | (1 << 31);
                                channels[ch].last_poll_tick = board_millis();
                            }
                            else
                            {
                                hcd_event_xfer_error(ch, true);
                                channels[ch].retry_count = 0;
                            }
                        }
                    }
                    else
                    {
                        channels[ch].retry_count++;
                        if (channels[ch].retry_count < MAX_RETRIES)
                        {
                            volatile uint32_t *HCCHAR = (volatile uint32_t *)(DWC_OTG_BASE + 0x500 + 0x20 * ch);
                            *HCCHAR = (*HCCHAR & ~(1 << 30)) | (1 << 31);
                            channels[ch].last_activity_tick = board_millis();
                        }
                        else
                        {
                            hcd_event_xfer_error(ch, true);
                            channels[ch].retry_count = 0;
                        }
                    }
                }
                if (txerr || bberr || frmor)
                {
                    if (ep_type == TUSB_XFER_ISOCHRONOUS)
                    {
                        hcd_event_xfer_complete(ch, 0, false);
                        channels[ch].retry_count = 0;
                    }
                    else
                    {
                        channels[ch].retry_count++;
                        if (channels[ch].retry_count < MAX_RETRIES)
                        {
                            volatile uint32_t *HCCHAR = (volatile uint32_t *)(DWC_OTG_BASE + 0x500 + 0x20 * ch);
                            *HCCHAR |= (1 << 30);                         // Disable channel
                            *HCCHAR = (*HCCHAR & ~(1 << 30)) | (1 << 31); // Re-enable
                            channels[ch].last_activity_tick = board_millis();
                        }
                        else
                        {
                            hcd_event_xfer_error(ch, true);
                            channels[ch].retry_count = 0;
                        }
                    }
                }
                *HCINT = hcint;
            }
        }
    }

    // Timeout handling (example, call this periodically from main loop)
    uint32_t now = board_millis();
    for (int ch = 0; ch < DWC_OTG_MAX_CHANNELS; ch++)
    {
        if (channels[ch].used && (now - channels[ch].last_activity_tick > TRANSFER_TIMEOUT_MS))
        {
            hcd_event_xfer_error(ch, true);
            channels[ch].retry_count = 0;
        }
    }

    DWC_OTG_GINTSTS = pending; // Clear handled interrupts
}

// Timeout handling (example, call this periodically from main loop)
uint32_t now = board_millis();
for (int ch = 0; ch < DWC_OTG_MAX_CHANNELS; ch++)
{
    if (channels[ch].used && (now - channels[ch].last_activity_tick > TRANSFER_TIMEOUT_MS))
    {
        hcd_event_xfer_error(ch, true);
        channels[ch].retry_count = 0;
        // Optionally reset/halt the channel here
    }
}

DWC_OTG_GINTSTS = pending;
}

// Optionally, implement other required TinyUSB HCD functions...
