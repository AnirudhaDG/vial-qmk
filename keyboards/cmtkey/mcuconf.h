#pragma once

#include_next <mcuconf.h>

#undef STM32_I2C_USE_I2C1
#define STM32_I2C_USE_I2C1 TRUE

// uncommet for G series DMA support
#undef STM32_I2C_USE_DMA
#define STM32_I2C_USE_DMA FALSE
