
/*
 * Project : STM8S003F3P6 Register-Level LED Blink
 * MCU     : STM8S003F3P6
 * Pin     : PA1
 * Clock   : 16 MHz (HSI)
 */

#include <stdint.h>

/* ================= REGISTER DEFINITIONS ================= */

/* Clock Control Register */
#define CLK_CKDIVR  (*(volatile uint8_t *)0x50C6)

/* GPIO Port A Registers */
#define PA_ODR      (*(volatile uint8_t *)0x5000)
#define PA_IDR      (*(volatile uint8_t *)0x5001)
#define PA_DDR      (*(volatile uint8_t *)0x5002)
#define PA_CR1      (*(volatile uint8_t *)0x5003)
#define PA_CR2      (*(volatile uint8_t *)0x5004)

/* GPIO Pin Definition */
#define PA1         (1U << 1)

/* ================= FUNCTION PROTOTYPES ================= */

void GPIO_Init(void);
void delay(volatile uint16_t count);

/* ================= MAIN FUNCTION ================= */

int main(void)
{
    /* Configure HSI clock to 16 MHz */
    CLK_CKDIVR = 0x00;

    /* Initialize PA1 */
    GPIO_Init();

    while (1)
    {
        /* LED ON */
        PA_ODR |= PA1;

        delay(10);

        /* LED OFF */
        PA_ODR &= (uint8_t)(~PA1);

        delay(10);
    }
}

/* ================= GPIO INITIALIZATION ================= */

void GPIO_Init(void)
{
    /* Set PA1 as output */
    PA_DDR |= PA1;

    /* Configure PA1 as push-pull */
    PA_CR1 |= PA1;

    /* Disable fast output mode */
    PA_CR2 &= (uint8_t)(~PA1);

    /* Initially LOW */
    PA_ODR &= (uint8_t)(~PA1);
}

/* ================= SOFTWARE DELAY ================= */

void delay(volatile uint16_t count)
{
    volatile uint16_t i;
    volatile uint16_t j;

    for (i = 0; i < count; i++)
    {
        for (j = 0; j < 60000U; j++)
        {
            /* Wait */
        }
    }
}
