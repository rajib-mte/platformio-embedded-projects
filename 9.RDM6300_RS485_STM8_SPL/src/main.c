/* =========================================================
   Project 9 : STM8S003F3P6  RDM6300 RFID -> RS485 transmitter
   Framework : STM8 Standard Peripheral Library (SPL)
   Clock     : 16 MHz HSI

   UART MAPPING
     UART0 (software UART, TX only) -> RS485 data transmit
            The STM8S003F3 has only one hardware UART (UART1),
            so UART0 is a bit-banged 9600 8N1 TX timed by TIM4.
     UART1 RX -> RFID (RDM6300), 9600 8N1
     UART1 TX -> serial monitor (USB-TTL adapter), 9600 8N1

   WIRING (TSSOP20 pin numbers)
     PD6 (pin 3)  <- RDM6300 TX
     PD5 (pin 2)  -> USB-TTL RX  (serial monitor)
     PD3 (pin 20) -> MAX485 DI   (UART0 / RS485 data)
     PD4 (pin 1)  -> MAX485 DE + RE tied together (HIGH = transmit)
     MAX485 RO    -> not connected
     MAX485 A/B   -> A/B of the Nano's MAX485 (project 8)
     GND          -> common to all boards
     PD1 (pin 18) =  SWIM, keep free for ST-Link

   RS485 receiver: 8.NANO_RS485 (9600, 50 ms idle gap per frame)
   RFID parsing taken from 6.RDM6300_STM8_arduino
   ========================================================= */

#include "stm8s.h"

/* ---------------- Pins ---------------- */
#define RS485_PORT       GPIOD
#define RS485_TX_PIN     GPIO_PIN_3      /* UART0 TX -> MAX485 DI      */
#define RS485_DIR_PIN    GPIO_PIN_4      /* MAX485 DE+RE               */

/* ---------------- Settings ------------- */
#define RFID_BAUD        9600
#define RS485_BIT_TICKS  104             /* 1 MHz ticks per bit = 9615 baud */

#define RFID_BUFFER_SIZE 15
#define RFID_FRAME_SIZE  14

/* ---------------- Globals -------------- */
static char     rfidData[RFID_BUFFER_SIZE];
static uint8_t  rfidIndex   = 0;
static uint16_t cardCounter = 0;

/* =========================================================
   CLOCK
   ========================================================= */
static void Clock_Init(void)
{
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);   /* 16 MHz */
}

/* =========================================================
   UART1 : RFID in (RX) + serial monitor out (TX)
   ========================================================= */
static void UART1_Setup(void)
{
    UART1_DeInit();
    UART1_Init((uint32_t)RFID_BAUD,
               UART1_WORDLENGTH_8D,
               UART1_STOPBITS_1,
               UART1_PARITY_NO,
               UART1_SYNCMODE_CLOCK_DISABLE,
               UART1_MODE_TXRX_ENABLE);
    UART1_Cmd(ENABLE);
}

static void Monitor_SendChar(char c)
{
    while (UART1_GetFlagStatus(UART1_FLAG_TXE) == RESET);
    UART1_SendData8((uint8_t)c);
}

/* =========================================================
   UART0 : software TX for RS485 (TIM4 bit timing)
   ========================================================= */
static void UART0_Setup(void)
{
    /* TX idle HIGH, DE/RE LOW (driver off) */
    GPIO_Init(RS485_PORT, RS485_TX_PIN,  GPIO_MODE_OUT_PP_HIGH_FAST);
    GPIO_Init(RS485_PORT, RS485_DIR_PIN, GPIO_MODE_OUT_PP_LOW_FAST);

    /* TIM4: 16 MHz / 16 = 1 MHz, overflow every RS485_BIT_TICKS us */
    TIM4_TimeBaseInit(TIM4_PRESCALER_16, (uint8_t)(RS485_BIT_TICKS - 1));
    TIM4_ClearFlag(TIM4_FLAG_UPDATE);
    TIM4_Cmd(ENABLE);
}

static void UART0_WaitBit(void)
{
    while (TIM4_GetFlagStatus(TIM4_FLAG_UPDATE) == RESET);
    TIM4_ClearFlag(TIM4_FLAG_UPDATE);
}

static void UART0_SendChar(char c)
{
    uint8_t data = (uint8_t)c;
    uint8_t i;

    /* line up with the timer so every bit is a full period */
    TIM4_SetCounter(0);
    TIM4_ClearFlag(TIM4_FLAG_UPDATE);

    GPIO_WriteLow(RS485_PORT, RS485_TX_PIN);          /* start bit */
    UART0_WaitBit();

    for (i = 0; i < 8; i++)                           /* LSB first */
    {
        if (data & 0x01) GPIO_WriteHigh(RS485_PORT, RS485_TX_PIN);
        else             GPIO_WriteLow(RS485_PORT, RS485_TX_PIN);
        data >>= 1;
        UART0_WaitBit();
    }

    GPIO_WriteHigh(RS485_PORT, RS485_TX_PIN);         /* stop bit */
    UART0_WaitBit();
}

static void RS485_Begin(void)
{
    GPIO_WriteHigh(RS485_PORT, RS485_DIR_PIN);        /* driver on */
    UART0_WaitBit();                                  /* settle */
}

static void RS485_End(void)
{
    /* software TX is blocking: the stop bit is already finished */
    GPIO_WriteLow(RS485_PORT, RS485_DIR_PIN);         /* driver off */
}

/* =========================================================
   OUTPUT HELPERS : every char goes to RS485 AND the monitor
   ========================================================= */
static void Out_Char(char c)
{
    UART0_SendChar(c);
    Monitor_SendChar(c);
}

static void Out_String(const char *s)
{
    while (*s) Out_Char(*s++);
}

static void Out_Number(uint16_t n)
{
    char buf[6];
    uint8_t i = 0;

    if (n == 0) { Out_Char('0'); return; }
    while (n > 0) { buf[i++] = (char)((n % 10) + '0'); n /= 10; }
    while (i > 0) Out_Char(buf[--i]);
}

/* =========================================================
   RFID (from project 6)
   ========================================================= */
static void RFID_ClearOverrun(void)
{
    /* bytes that arrived while we were transmitting */
    if (UART1_GetFlagStatus(UART1_FLAG_OR) == SET)
    {
        (void)UART1_ReceiveData8();
        rfidIndex = 0;
    }
}

static void RFID_SendCard(void)
{
    uint8_t i;

    cardCounter++;

    RS485_Begin();
    Out_String("Counter = ");
    Out_Number(cardCounter);
    Out_String(" > Card ID: ");
    for (i = 1; i < 9; i++) Out_Char(rfidData[i]);    /* 8-char ID */
    Out_String("\r\n");
    RS485_End();
}

static void RFID_Process(void)
{
    RFID_ClearOverrun();

    while (UART1_GetFlagStatus(UART1_FLAG_RXNE) == SET)
    {
        uint8_t data = UART1_ReceiveData8();

        if (data == 0x02)                             /* STX */
        {
            rfidIndex = 0;
            rfidData[rfidIndex++] = (char)data;
        }
        else if (rfidIndex > 0)
        {
            if (rfidIndex < RFID_BUFFER_SIZE) rfidData[rfidIndex++] = (char)data;
            else                              rfidIndex = 0;
        }

        if (data == 0x03 && rfidIndex > 0)            /* ETX */
        {
            if (rfidIndex == RFID_FRAME_SIZE) RFID_SendCard();
            rfidIndex = 0;
        }
    }
}

/* =========================================================
   MAIN
   ========================================================= */
void main(void)
{
    Clock_Init();
    UART0_Setup();
    UART1_Setup();

    RS485_Begin();
    Out_String("STM8S003F3 RFID -> RS485\r\n");
    RS485_End();

    while (1)
    {
        RFID_Process();
    }
}
