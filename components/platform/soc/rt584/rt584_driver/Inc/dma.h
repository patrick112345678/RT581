/**************************************************************************//**
 * @file     dma.h
 * @version
 * @brief    DMA driver header file
 *
 * @copyright
 ******************************************************************************/
#ifndef __DMA_API_H__
#define __DMA_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_NUMBER_OF_DMA      2     

#define DMA_STATUS_BUSY        1
#define DMA_STATUS_FREE        0

/**
 * @brief DMA finish routine callback for user application.
 * @param chennel_id:   DMA channel_id
 * @param statue:       DMA transfer status
 *
 */
typedef void (*dma_proc_cb_t)(uint32_t channel_id);

/**@brief DMA config structure for DMA setting
 **
 */
typedef struct {
    uint32_t   dma_src_addr;
    uint32_t   dma_dest_addr;
    uint32_t   dma_length;
} dma_config_t;                    /*@TODO: change name*/


/**
 * @brief DMA interrupt ISR function register. Use to notify channel DMA finished.
 *
 * @param dma_channel_id   Specifies the dma channel number.
 * @param app_dma_callback
 */
extern uint32_t dma_register_isr(
    uint32_t  dma_channel_id,
    dma_proc_cb_t  app_dma_callback
);

/**
 * @brief DMA config. Use to config DMA mode and start the DMA.
 *
 * @param dma_channel_id   Specifies the dma channel number.
 * @param *dma_config_t    Specifies the dma setting.
 *
 */
 
extern uint32_t dma_config_and_enable(
     uint32_t  dma_channel_id, 
     const dma_config_t *mode
);

/**
 * @brief Get DMA transfer status.
 *
 * @param dma_channel_id  Specifies the dma channel number.
 * @param status    DMA tranfer status.
 *
 */
extern uint32_t dma_get_status(uint32_t  dma_channel_id, uint32_t *status);


#ifdef __cplusplus
}
#endif

#endif
