/**
 * @file app_uart.c
 * @author Jiemin Cao (jiemin.cao@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-08-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdlib.h>
#include <stdint.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "queue.h"
#include <hosal_uart.h>
#include "log.h"
#include "main.h"

//=============================================================================
//                  Constant Definition
//=============================================================================
#define UART1_OPERATION_PORT 1
HOSAL_UART_DEV_DECL(uart1_dev, UART1_OPERATION_PORT, 4, 5, UART_BAUDRATE_115200)

#define UART2_OPERATION_PORT 2
HOSAL_UART_DEV_DECL(uart2_dev, UART2_OPERATION_PORT, 6, 7, UART_BAUDRATE_19200)
//=============================================================================
//                  Macro Definition
//=============================================================================
#define RX_BUFF_SIZE                    1200
#define TX_BUFF_SIZE                    RX_BUFF_SIZE
#define TX_BUFF_MASK                    (TX_BUFF_SIZE -1)
//=============================================================================
//                  Structure Definition
//=============================================================================
typedef struct uart_io
{
    uint16_t start;
    uint16_t end;

    uint32_t recvLen;
    uint8_t uart_cache[RX_BUFF_SIZE];
} uart_io_t;

typedef struct
{
    uint8_t port;
    uint16_t dlen;
    uint8_t *pdata;
} _app_uart_data_t ;

//=============================================================================
//                  Global Data Definition
//=============================================================================
static QueueHandle_t app_uart_handle;
static uart_io_t g_uart1_rx_io = { .start = 0, .end = 0, };
static uart_io_t g_uart2_rx_io = { .start = 0, .end = 0, };
static uint8_t g_tx_buf[TX_BUFF_SIZE];
static hosal_uart_dma_cfg_t txdam_cfg =
{
    .dma_buf = g_tx_buf,
    .dma_buf_size = sizeof(g_tx_buf),
};

//=============================================================================
//                  Private Function Definition
//=============================================================================
static int __uart_tx_callback(void *p_arg)
{
    APP_EVENT_NOTIFY_ISR(EVENT_UART_UART_OUT_DONE);
    return 0;
}

static void __uart_queue_send(app_task_event_t evt)
{
    _app_uart_data_t uart_data;
    static uint32_t __tx_done = 1;

    if ((EVENT_UART_UART_OUT_DONE & evt))
    {
        __tx_done = 1;
        return;
    }

    if (__tx_done == 1)
    {
        if (xQueueReceive(app_uart_handle, (void *)&uart_data, 0) == pdPASS)
        {
            memcpy(txdam_cfg.dma_buf, uart_data.pdata, uart_data.dlen);
            txdam_cfg.dma_buf_size = uart_data.dlen;
            if (UART1_OPERATION_PORT == uart_data.port)
            {
                hosal_uart_ioctl(&uart1_dev, HOSAL_UART_DMA_TX_START, &txdam_cfg);
                log_debug_hexdump("uart1 Tx", uart_data.pdata, uart_data.dlen);
            }
            else if (UART2_OPERATION_PORT == uart_data.port)
            {
                hosal_uart_ioctl(&uart2_dev, HOSAL_UART_DMA_TX_START, &txdam_cfg);
                log_debug_hexdump("Send Meter (uart2 Tx)", uart_data.pdata, uart_data.dlen);
            }
            __tx_done = 0;
            if (uart_data.pdata)
            {
                mem_free(uart_data.pdata);
            }
        }
    }
}

/*uart 1 use*/
static int __uart1_read(uint8_t *p_data)
{
    uint32_t byte_cnt = 0;
    hosal_uart_ioctl(&uart1_dev, HOSAL_UART_DISABLE_INTERRUPT, (void *)NULL);

    if (g_uart1_rx_io.start != g_uart1_rx_io.end)
    {
        if (g_uart1_rx_io.start > g_uart1_rx_io.end)
        {
            memcpy(p_data, g_uart1_rx_io.uart_cache + g_uart1_rx_io.end, g_uart1_rx_io.start - g_uart1_rx_io.end);
            byte_cnt = g_uart1_rx_io.start - g_uart1_rx_io.end;
            g_uart1_rx_io.end = g_uart1_rx_io.start;
        }
        else
        {
            memcpy(p_data, g_uart1_rx_io.uart_cache + g_uart1_rx_io.end, RX_BUFF_SIZE - g_uart1_rx_io.end);
            byte_cnt = RX_BUFF_SIZE - g_uart1_rx_io.end;
            g_uart1_rx_io.end = RX_BUFF_SIZE - 1;

            if (g_uart1_rx_io.start)
            {
                memcpy(&p_data[byte_cnt], g_uart1_rx_io.uart_cache, g_uart1_rx_io.start);
                byte_cnt += g_uart1_rx_io.start;
                g_uart1_rx_io.end = (RX_BUFF_SIZE + g_uart1_rx_io.start - 1) % RX_BUFF_SIZE;
            }
        }
    }

    g_uart1_rx_io.start = g_uart1_rx_io.end = 0;
    g_uart1_rx_io.recvLen = 0;
    hosal_uart_ioctl(&uart1_dev, HOSAL_UART_ENABLE_INTERRUPT, (void *)NULL);
    return byte_cnt;
}

static int __uart1_rx_callback(void *p_arg)
{
    uint32_t len = 0;
    if (g_uart1_rx_io.start >= g_uart1_rx_io.end)
    {
        g_uart1_rx_io.start += hosal_uart_receive(p_arg, g_uart1_rx_io.uart_cache + g_uart1_rx_io.start,
                               RX_BUFF_SIZE - g_uart1_rx_io.start - 1);
        if (g_uart1_rx_io.start == (RX_BUFF_SIZE - 1))
        {
            g_uart1_rx_io.start = hosal_uart_receive(p_arg, g_uart1_rx_io.uart_cache,
                                  (RX_BUFF_SIZE + g_uart1_rx_io.end - 1) % RX_BUFF_SIZE);
        }
    }
    else if (((g_uart1_rx_io.start + 1) % RX_BUFF_SIZE) != g_uart1_rx_io.end)
    {
        g_uart1_rx_io.start += hosal_uart_receive(p_arg, g_uart1_rx_io.uart_cache,
                               g_uart1_rx_io.end - g_uart1_rx_io.start - 1);
    }

    if (g_uart1_rx_io.start != g_uart1_rx_io.end)
    {

        len = (g_uart1_rx_io.start + RX_BUFF_SIZE - g_uart1_rx_io.end) % RX_BUFF_SIZE;
        if (g_uart1_rx_io.recvLen != len)
        {
            g_uart1_rx_io.recvLen = len;
            APP_EVENT_NOTIFY_ISR(EVENT_UART1_UART_IN);
        }
    }

    return 0;
}

void __uart1_data_parse()
{
    uint16_t len = 0;
    uint8_t *tmp_buff = mem_malloc(RX_BUFF_SIZE);
    if (tmp_buff)
    {
        len = __uart1_read(tmp_buff);
        log_info_hexdump("uart 1 parse", tmp_buff, len);
        mem_free(tmp_buff);
    }
}


/*uart 2 use*/
static int __uart2_read(uint8_t *p_data)
{
    uint32_t byte_cnt = 0;
    hosal_uart_ioctl(&uart2_dev, HOSAL_UART_DISABLE_INTERRUPT, (void *)NULL);

    if (g_uart2_rx_io.start != g_uart2_rx_io.end)
    {
        if (g_uart2_rx_io.start > g_uart2_rx_io.end)
        {
            memcpy(p_data, g_uart2_rx_io.uart_cache + g_uart2_rx_io.end, g_uart2_rx_io.start - g_uart2_rx_io.end);
            byte_cnt = g_uart2_rx_io.start - g_uart2_rx_io.end;
            g_uart2_rx_io.end = g_uart2_rx_io.start;
        }
        else
        {
            memcpy(p_data, g_uart2_rx_io.uart_cache + g_uart2_rx_io.end, RX_BUFF_SIZE - g_uart2_rx_io.end);
            byte_cnt = RX_BUFF_SIZE - g_uart2_rx_io.end;
            g_uart2_rx_io.end = RX_BUFF_SIZE - 1;

            if (g_uart2_rx_io.start)
            {
                memcpy(&p_data[byte_cnt], g_uart2_rx_io.uart_cache, g_uart2_rx_io.start);
                byte_cnt += g_uart2_rx_io.start;
                g_uart2_rx_io.end = (RX_BUFF_SIZE + g_uart2_rx_io.start - 1) % RX_BUFF_SIZE;
            }
        }
    }

    g_uart2_rx_io.start = g_uart2_rx_io.end = 0;
    g_uart2_rx_io.recvLen = 0;
    hosal_uart_ioctl(&uart2_dev, HOSAL_UART_ENABLE_INTERRUPT, (void *)NULL);
    return byte_cnt;
}

static int __uart2_rx_callback(void *p_arg)
{
    uint32_t len = 0;
    if (g_uart2_rx_io.start >= g_uart2_rx_io.end)
    {
        g_uart2_rx_io.start += hosal_uart_receive(p_arg, g_uart2_rx_io.uart_cache + g_uart2_rx_io.start,
                               RX_BUFF_SIZE - g_uart2_rx_io.start - 1);
        if (g_uart2_rx_io.start == (RX_BUFF_SIZE - 1))
        {
            g_uart2_rx_io.start = hosal_uart_receive(p_arg, g_uart2_rx_io.uart_cache,
                                  (RX_BUFF_SIZE + g_uart2_rx_io.end - 1) % RX_BUFF_SIZE);
        }
    }
    else if (((g_uart2_rx_io.start + 1) % RX_BUFF_SIZE) != g_uart2_rx_io.end)
    {
        g_uart2_rx_io.start += hosal_uart_receive(p_arg, g_uart2_rx_io.uart_cache,
                               g_uart2_rx_io.end - g_uart2_rx_io.start - 1);
    }

    if (g_uart2_rx_io.start != g_uart2_rx_io.end)
    {

        len = (g_uart2_rx_io.start + RX_BUFF_SIZE - g_uart2_rx_io.end) % RX_BUFF_SIZE;
        if (g_uart2_rx_io.recvLen != len)
        {
            g_uart2_rx_io.recvLen = len;
            APP_EVENT_NOTIFY_ISR(EVENT_UART2_UART_IN);
        }
    }

    return 0;
}

void __uart2_data_parse()
{
    uint16_t len = 0;
    uint8_t *tmp_buff = mem_malloc(RX_BUFF_SIZE);
    if (tmp_buff)
    {
        len = __uart2_read(tmp_buff);
        //log_info_hexdump("uart 2 parse", g_tmp_buff, len);
        udf_Meter_received_task(tmp_buff, len);
        mem_free(tmp_buff);
    }
}

void __uart_task(app_task_event_t sevent)
{
    if ((EVENT_UART_UART_OUT & sevent) || (EVENT_UART_UART_OUT_DONE & sevent))
    {
        __uart_queue_send(sevent);
    }

    if (EVENT_UART1_UART_IN & sevent)
    {
        __uart1_data_parse();
    }

    if (EVENT_UART2_UART_IN & sevent)
    {
        __uart2_data_parse();
    }
}

int app_uart_data_send(uint8_t u_port, uint8_t *p_data, uint16_t data_len)
{
    _app_uart_data_t uart_data;

    uart_data.pdata =  mem_malloc(data_len);
    if (uart_data.pdata)
    {
        memcpy(uart_data.pdata, p_data, data_len);
        uart_data.port = u_port;
        uart_data.dlen = data_len;
        //log_info_hexdump("uart sent",p_data,data_len);
        //        while (xQueueSend(app_uart_handle, (void *)&uart_data, portMAX_DELAY) != pdPASS);

        //        APP_EVENT_NOTIFY(EVENT_UART_UART_OUT);
        if (xQueueSend(app_uart_handle, (void *)&uart_data, portMAX_DELAY) != pdPASS)
        {
            printf("queue full \r\n");
        }
        else
        {
            APP_EVENT_NOTIFY(EVENT_UART_UART_OUT);
        }
    }

    return 0;
}

void app_uart_init(void)
{
    /*Init UART In the first place*/
    hosal_uart_init(&uart1_dev);
    hosal_uart_init(&uart2_dev);

    app_uart_handle = xQueueCreate(16, sizeof(_app_uart_data_t));

    /* Configure UART Rx interrupt callback function */
    hosal_uart_callback_set(&uart1_dev, HOSAL_UART_RX_CALLBACK, __uart1_rx_callback, &uart1_dev);
    hosal_uart_callback_set(&uart1_dev, HOSAL_UART_TX_DMA_CALLBACK, __uart_tx_callback, &uart1_dev);

    hosal_uart_callback_set(&uart2_dev, HOSAL_UART_RX_CALLBACK, __uart2_rx_callback, &uart2_dev);
    hosal_uart_callback_set(&uart2_dev, HOSAL_UART_TX_DMA_CALLBACK, __uart_tx_callback, &uart2_dev);

    /* Configure UART to interrupt mode */
    hosal_uart_ioctl(&uart1_dev, HOSAL_UART_MODE_SET, (void *)HOSAL_UART_MODE_INT_RX);
    hosal_uart_ioctl(&uart2_dev, HOSAL_UART_MODE_SET, (void *)HOSAL_UART_MODE_INT_RX);

    __NVIC_SetPriority(Uart1_IRQn, 2);
    __NVIC_SetPriority(Uart2_IRQn, 2);
}