#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "uart.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    200
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

#define RX_BUFFER_SIZE 256
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static unsigned index_buff;

#define SIGNAL_START        0
#define SIGNAL_UART_RX      1
#define SIGNAL_UART_RX_TOUT 2
#define SIGNAL_UART_ERR     3
#define SIGNAL_UART_EOT     4
#define SIGNAL_PROMPT       5

#define PROMPT "\r\nHello. What is your name?>"

static ETSTimer prompt_timer;

//-------------------------------------------------------------------------------------------------
static void  prompt_timer_handler(void *dummy)
{
    system_os_post(user_procTaskPrio, SIGNAL_PROMPT, 0 );
}
//-------------------------------------------------------------------------------------------------

//Main code function
static void  loop(os_event_t *events)
{
    switch(events->sig)
    {
        case SIGNAL_START:
        case SIGNAL_PROMPT:
            os_printf(PROMPT);
        break;
        case SIGNAL_UART_RX_TOUT:
            os_printf(".");
        case SIGNAL_UART_RX:
            os_timer_disarm(&prompt_timer); // Start read each 5 seconds
            rx_buffer[index_buff++]=events->par;
            if(index_buff >= RX_BUFFER_SIZE)
            {
                os_printf("\r\nYour name is too long. Try again?>");
                index_buff = 0;
            }
            else if(events->par == '\r')
            {
                rx_buffer[index_buff-1]='\0';
                os_printf("Good morning %s. How are you?.\r\n",rx_buffer);
                os_printf(PROMPT);
                index_buff = 0;
                os_timer_arm(&prompt_timer,5000,1); // Start read each 5 seconds
            }
            else
            {
                os_printf("%c",events->par);
            }
        break;
        case SIGNAL_UART_ERR:
            switch(events->par)
            {
                case UART_FRM_ERR_INT_ST:
                    os_printf("<FRM ERR>\r\n");
                break;
                case UART_BRK_DET_INT_ST:
                    os_printf("<BRK ERR>\r\n");
                break;
                case UART_CTS_CHG_INT_ST:
                    os_printf("<CTS ERR>\r\n");
                break;
                case UART_DSR_CHG_INT_ST:
                    os_printf("<DSR ERR>\r\n");
                break;
                case UART_RXFIFO_OVF_INT_ST:
                    os_printf("<FRM ERR>\r\n");
                break;
                case UART_PARITY_ERR_INT_ST:
                    os_printf("<PAR ERR>\r\n");
                break;
            }
        break;
        case SIGNAL_UART_EOT:
            os_printf("<EOT>\r\n");
        break;
    }
}

//------------------------------------------------------------
void ICACHE_FLASH_ATTR user_init()
{
     //Set ap settings
    uart_init(user_procTaskPrio,
              SIGNAL_UART_RX,
              SIGNAL_UART_RX_TOUT,
              SIGNAL_UART_ERR,
              SIGNAL_UART_EOT);

    // Initialize read prompt timer
    os_timer_disarm(&prompt_timer);
    os_timer_setfn(&prompt_timer,prompt_timer_handler,NULL);

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    system_os_post(user_procTaskPrio, SIGNAL_START, 0 );

    os_timer_arm(&prompt_timer,5000,1); // Start read each 5 seconds
}
