#ifndef _BSP_DELAY_H
#define _BSP_DELAY_H

#include "stm32f4xx_hal.h"


//void delay_init(uint8_t SYSCLK);

void delay_us(uint32_t us);
void delay_ms(uint16_t ms);


#endif
