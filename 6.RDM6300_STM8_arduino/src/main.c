#include <stdint.h>

/* =========================================================
   STM8S003F3P6  -  RDM6300 RFID  ->  RS485 (MAX485) transmitter

   The STM8S003F3 has only ONE UART (UART1), so it does both jobs:
     UART1 RX (PD6) receives frames from the RDM6300
     UART1 TX (PD5) drives the MAX485 DI pin

   WIRING (TSSOP20 pin numbers)
     STM8 PD6 (pin 3)  <- RDM6300 TX
     STM8 PD5 (pin 2)  -> MAX485 DI
     STM8 PD4 (pin 1)  -> MAX485 DE + RE (tied together)
                          HIGH = transmit, LOW = idle/receive
     MAX485 RO         -> leave unconnected (do NOT wire to PD6,
                          the RDM6300 already uses that line)
     MAX485 A / B      -> A / B of the Nano's MAX485 (project 8)
     GND               -> common ground for STM8, RDM6300, both MAX485
     STM8 PD1 (pin 18) =  SWIM, keep free for ST-Link

   RS485 settings: 9600 baud, 8N1 (matches 8.NANO_RS485)
   ========================================================= */

/* Clock */
#define CLK_CKDIVR       (*(volatile uint8_t *)0x50C6)

/* GPIO Port D */
#define PD_ODR           (*(volatile uint8_t *)0x500F)
#define PD_DDR           (*(volatile uint8_t *)0x5011)
#define PD_CR1           (*(volatile uint8_t *)0x5012)
#define PD_CR2           (*(volatile uint8_t *)0x5013)

/* UART1 Registers */
#define UART1_SR         (*(volatile uint8_t *)0x5230)
#define UART1_DR         (*(volatile uint8_t *)0x5231)
#define UART1_BRR1       (*(volatile uint8_t *)0x5232)
#define UART1_BRR2       (*(volatile uint8_t *)0x5233)
#define UART1_CR1        (*(volatile uint8_t *)0x5234)
#define UART1_CR2        (*(volatile uint8_t *)0x5235)
#define UART1_CR3        (*(volatile uint8_t *)0x5236)

/* UART Status bits */
#define UART_SR_TXE      (1 << 7)
#define UART_SR_TC       (1 << 6)
#define UART_SR_RXNE     (1 << 5)
#define UART_SR_OR       (1 << 3)

/* UART Control bits */
#define UART_CR2_REN     (1 << 2)
#define UART_CR2_TEN     (1 << 3)

/* RS485 direction pin: PD4 -> MAX485 DE + RE */
#define RS485_DIR_PIN    4
#define RS485_TX_MODE()  (PD_ODR |=  (1 << RS485_DIR_PIN))
#define RS485_RX_MODE()  (PD_ODR &= ~(1 << RS485_DIR_PIN))

/* RFID configuration */
#define RFID_BUFFER_SIZE 15
#define RFID_FRAME_SIZE  14

/* =========================================================
   GLOBAL VARIABLES
   ========================================================= */

volatile char rfidData[RFID_BUFFER_SIZE];
volatile uint8_t rfidIndex = 0;
volatile uint16_t cardCounter = 0;

/* =========================================================
   SHORT DELAY (approximate, at 16 MHz)
   ========================================================= */

static void delay_us_approx(uint16_t n)
{
    while (n--)
    {
        __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
    }
}

/* =========================================================
   RS485 DIRECTION PIN INIT
   ========================================================= */

void RS485_Init(void)
{
    /* PD4 output push-pull, start LOW (driver off) */
    PD_ODR &= ~(1 << RS485_DIR_PIN);
    PD_DDR |=  (1 << RS485_DIR_PIN);
    PD_CR1 |=  (1 << RS485_DIR_PIN);
    PD_CR2 &= ~(1 << RS485_DIR_PIN);
}

/* =========================================================
   UART1 INITIALIZATION
   9600 baud, 8 data bits, no parity, 1 stop bit
   ========================================================= */

void UART1_Init(void)
{
    /* 16 MHz HSI, no prescaler */
    CLK_CKDIVR = 0x00;

    /* PD6 = UART1_RX, input floating */
    PD_DDR &= ~(1 << 6);
    PD_CR1 &= ~(1 << 6);
    PD_CR2 &= ~(1 << 6);

    /* PD5 = UART1_TX, output push-pull */
    PD_DDR |= (1 << 5);
    PD_CR1 |= (1 << 5);
    PD_CR2 |= (1 << 5);

    /* Disable UART before configuration */
    UART1_CR2 = 0x00;

    /* 16 MHz / 9600 = 1667 = 0x0683 (BRR2 must be written first) */
    UART1_BRR2 = 0x03;
    UART1_BRR1 = 0x68;

    /* 8-bit word length, no parity */
    UART1_CR1 = 0x00;

    /* One stop bit */
    UART1_CR3 = 0x00;

    /* Enable transmitter and receiver */
    UART1_CR2 = UART_CR2_REN | UART_CR2_TEN;
}

/* =========================================================
   UART1 RAW TRANSMIT (no direction control)
   ========================================================= */

void UART1_SendChar(char data)
{
    while (!(UART1_SR & UART_SR_TXE));
    UART1_DR = data;
}

void UART1_SendString(const char *str)
{
    while (*str)
    {
        UART1_SendChar(*str++);
    }
}

void UART1_SendNumber(uint16_t number)
{
    char buffer[6];
    uint8_t i = 0;

    if (number == 0)
    {
        UART1_SendChar('0');
        return;
    }

    while (number > 0)
    {
        buffer[i++] = (number % 10) + '0';
        number /= 10;
    }

    while (i > 0)
    {
        UART1_SendChar(buffer[--i]);
    }
}

/* =========================================================
   RS485 FRAME CONTROL
   Call RS485_Begin() before sending a message and
   RS485_End() after it, so the whole message goes out
   as one burst with the driver enabled.
   ========================================================= */

void RS485_Begin(void)
{
    RS485_TX_MODE();
    delay_us_approx(10);            /* let the MAX485 driver settle */
}

void RS485_End(void)
{
    /* Wait until the last byte, including its stop bit,
       has fully left the shift register (TC), not just TXE */
    while (!(UART1_SR & UART_SR_TC));
    RS485_RX_MODE();
}

/* =========================================================
   UART1 RECEIVE
   ========================================================= */

uint8_t UART1_Available(void)
{
    return (UART1_SR & UART_SR_RXNE);
}

char UART1_Read(void)
{
    return UART1_DR;
}

/* Clear an overrun left over from bytes that arrived
   while we were busy transmitting (read SR then DR) */
void UART1_ClearOverrun(void)
{
    if (UART1_SR & UART_SR_OR)
    {
        (void)UART1_DR;
        rfidIndex = 0;              /* drop the broken frame */
    }
}

/* =========================================================
   RFID FRAME PROCESSING
   ========================================================= */

void RFID_Process(void)
{
    UART1_ClearOverrun();

    while (UART1_Available())
    {
        uint8_t data = UART1_Read();

        /* Start of RFID frame: STX = 0x02 */
        if (data == 0x02)
        {
            rfidIndex = 0;
            rfidData[rfidIndex++] = data;
        }
        /* Collect RFID frame */
        else if (rfidIndex > 0)
        {
            if (rfidIndex < RFID_BUFFER_SIZE)
            {
                rfidData[rfidIndex++] = data;
            }
            else
            {
                rfidIndex = 0;
            }
        }

        /* End of RFID frame: ETX = 0x03 */
        if (data == 0x03 && rfidIndex > 0)
        {
            if (rfidIndex == RFID_FRAME_SIZE)
            {
                cardCounter++;

                /* One message = one RS485 burst (< 64 bytes for the Nano) */
                RS485_Begin();

                UART1_SendString("Counter = ");
                UART1_SendNumber(cardCounter);
                UART1_SendString(" > Card ID: ");

                /* 8-character RFID ID */
                for (uint8_t i = 1; i < 9; i++)
                {
                    UART1_SendChar(rfidData[i]);
                }

                UART1_SendString("\r\n");

                RS485_End();
            }

            /* Reset for next RFID frame */
            rfidIndex = 0;
        }
    }
}

/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    RS485_Init();
    UART1_Init();

    RS485_Begin();
    UART1_SendString("STM8S003F3 RFID Reader\r\n");
    UART1_SendString("RDM6300 Ready, RS485 TX\r\n");
    RS485_End();

    while (1)
    {
        RFID_Process();
    }
}
