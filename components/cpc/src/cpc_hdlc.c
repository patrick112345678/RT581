/**
 * @file cpc_hdlc.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-08-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cpc_hdlc.h"
#include "cpc_crc.h"

/*******************************************************************************
 *********************************   DEFINES   *********************************
 ******************************************************************************/

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

/*******************************************************************************
 **************************   LOCAL FUNCTIONS   ********************************
 ******************************************************************************/

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

extern inline uint8_t cpc_hdlc_get_flag(const uint8_t *header_buf);
extern inline uint8_t cpc_hdlc_get_address(const uint8_t *header_buf);
extern inline uint16_t cpc_hdlc_get_length(const uint8_t *header_buf);
extern inline uint8_t cpc_hdlc_get_control(const uint8_t *header_buf);
extern inline uint16_t cpc_hdlc_get_hcs(const uint8_t *header_buf);
extern inline uint16_t cpc_hdlc_get_fcs(const uint8_t *payload_buf, uint16_t payload_length);
extern inline uint8_t cpc_hdlc_get_frame_type(uint8_t control);
extern inline uint8_t cpc_hdlc_get_seq(uint8_t control);
extern inline uint8_t cpc_hdlc_get_ack(uint8_t control);
extern inline uint8_t cpc_hdlc_get_supervisory_function(uint8_t control);
extern inline uint8_t cpc_hdlc_get_unumbered_type(uint8_t control);
extern inline bool cpc_hdlc_is_poll_final(uint8_t control);
extern inline uint8_t cpc_hdlc_create_control_data(uint8_t seq, uint8_t ack, bool poll_final);
extern inline uint8_t cpc_hdlc_create_control_supervisory(uint8_t ack, uint8_t supervisory_function);
extern inline uint8_t cpc_hdlc_create_control_unumbered(uint8_t type);
extern inline void cpc_hdlc_set_control_ack(uint8_t *header_buf, uint8_t ack);

/***************************************************************************/ /**
                                                                               * Initialize Power Manager module.
                                                                               ******************************************************************************/
void cpc_hdlc_create_header(uint8_t *header_buf,
                            uint8_t address,
                            uint16_t length,
                            uint8_t control,
                            bool compute_crc)
{
    header_buf[0] = CPC_HDLC_FLAG_VAL;
    header_buf[1] = address;
    header_buf[2] = (uint8_t)length;
    header_buf[3] = (uint8_t)(length >> 8);
    header_buf[4] = control;

    if (compute_crc)
    {
        uint16_t hcs;

        hcs = cpc_get_crc_sw(header_buf, CPC_HDLC_HEADER_SIZE);

        header_buf[5] = (uint8_t)hcs;
        header_buf[6] = (uint8_t)(hcs >> 8);
    }
}
