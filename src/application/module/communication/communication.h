#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include "tx_api.h"

#include "stm32f4xx_hal.h"

#include "ano.h"

#include <math.h>

#include "sensor.h"

#define COMMUNICATION_PRIO 14
#define COMMUNICATION_STACKSIZE 2048
extern TX_THREAD communication_tcb;
extern UCHAR communication_stack[COMMUNICATION_STACKSIZE];


void communication_entry(ULONG thread_input);

#endif
