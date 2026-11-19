#include <stdint.h>

/* =========================================================
   STM8S003F3P6 REGISTER DEFINITIONS
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
#define UART_SR_RXNE     (1 << 5)

/* UART Control bits */
#define UART_CR2_REN     (1 << 2)
#define UART_CR2_TEN     (1 << 3)

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
   UART1 INITIALIZATION
   9600 baud, 8 data bits, no parity, 1 stop bit
   ========================================================= */

void UART1_Init(void)
{
    /* Enable 16 MHz HSI clock (default HSI clock source) */
    CLK_CKDIVR = 0x00;

    /* PD6 = UART1_RX
       Input floating */
    PD_DDR &= ~(1 << 6);
    PD_CR1 &= ~(1 << 6);
    PD_CR2 &= ~(1 << 6);

    /* PD5 = UART1_TX
       Output push-pull */
    PD_DDR |= (1 << 5);
    PD_CR1 |= (1 << 5);
    PD_CR2 |= (1 << 5);

    /* Disable UART before configuration */
    UART1_CR2 = 0x00;

    /* 16 MHz / 9600 baud */

    UART1_BRR1 = 0x68;
    UART1_BRR2 = 0x03;

    /* 8-bit word length, no parity */
    UART1_CR1 = 0x00;

    /* One stop bit, no hardware flow control */
    UART1_CR3 = 0x00;

    /* Enable transmitter and receiver */
    UART1_CR2 = UART_CR2_REN | UART_CR2_TEN;
}

/* =========================================================
   UART1 TRANSMIT
   ========================================================= */

void UART1_SendChar(char data)
{
    while (!(UART1_SR & (1 << 7)));

    UART1_DR = data;
}

void UART1_SendString(const char *str)
{
    while (*str)
    {
        UART1_SendChar(*str++);
    }
}

/* =========================================================
   INTEGER TO ASCII
   ========================================================= */

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

/* =========================================================
   RFID FRAME PROCESSING
   ========================================================= */

void RFID_Process(void)
{
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

                UART1_SendString("\r\nCounter = ");
                UART1_SendNumber(cardCounter);

                UART1_SendString(" > Card ID: ");

                /* Extract 8-character RFID ID */
                for (uint8_t i = 1; i < 9; i++)
                {
                    UART1_SendChar(rfidData[i]);
                }

                UART1_SendString("\r\n");
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
    UART1_Init();

    UART1_SendString("\r\nSTM8S003F3 RFID Reader\r\n");
    UART1_SendString("RDM6300 Ready...\r\n");

    while (1)
    {
        RFID_Process();
    }
}
