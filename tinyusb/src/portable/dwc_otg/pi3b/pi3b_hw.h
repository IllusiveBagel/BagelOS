#ifndef PI3B_HW_H
#define PI3B_HW_H

#include <stdint.h>

// DWC OTG USB controller base address for Raspberry Pi 3B
#define DWC_OTG_BASE 0x3F980000

// Example register definitions (add more as needed)
#define DWC_OTG_GUSBCFG (*(volatile uint32_t *)(DWC_OTG_BASE + 0x0C))
#define DWC_OTG_GINTSTS (*(volatile uint32_t *)(DWC_OTG_BASE + 0x14))
#define DWC_OTG_GINTMSK (*(volatile uint32_t *)(DWC_OTG_BASE + 0x18))
#define DWC_OTG_GRSTCTL (*(volatile uint32_t *)(DWC_OTG_BASE + 0x10))
#define DWC_OTG_HPRT (*(volatile uint32_t *)(DWC_OTG_BASE + 0x440))
#define DWC_OTG_HCFG (*(volatile uint32_t *)(DWC_OTG_BASE + 0x400))
// ...add more registers and bitfields as needed for your implementation

#endif // PI3B_HW_H