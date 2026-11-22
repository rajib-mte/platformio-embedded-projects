#ifndef __STM8S_CONF_H
#define __STM8S_CONF_H

#include "stm8s.h"

/* Only the SPL modules this project uses (keeps flash small) */
#include "stm8s_clk.h"
#include "stm8s_gpio.h"
#include "stm8s_tim4.h"
#include "stm8s_uart1.h"

/* Parameter checks off to save flash */
#define assert_param(expr) ((void)0)

#endif /* __STM8S_CONF_H */
