/* ZBOSS Zigbee software protocol stack
 *
 * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
 * www.dsr-zboss.com
 * www.dsr-corporation.com
 * All rights reserved.
 *
 * This is unpublished proprietary source code of DSR Corporation
 * The copyright notice does not evidence any actual or intended
 * publication of such source code.
 *
 * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
 * Corporation
 *
 * Commercial Usage
 * Licensees holding valid DSR Commercial licenses may use
 * this file in accordance with the DSR Commercial License
 * Agreement provided with the Software or, alternatively, in accordance
 * with the terms contained in a written agreement between you and
 * DSR.
 */
/*     PURPOSE: NCP manufacture mode routines.
*/

#define ZB_TRACE_FILE_ID 14054
#include "zb_common.h"
#include "zb_mac_globals.h"
#include "zb_sniffer.h"
#include "mac_internal.h"
#include "zb_ti13xx_config.h"
#include "ncp_hl_proto.h"
#include "ncp_manufacture_mode.h"
/*cstat -MISRAC2012-* */
#include <ti/drivers/dpl/SwiP.h>
/*cstat +MISRAC2012-* */

/* EVS: TODO: move it to platform */

#ifndef NCP_MODE
#error This app is to be compiled as NCP application FW
#endif

/* CMD_FS
   Frequency Synthesizer Programming Command */
static rfc_CMD_FS_t RF_cmdFs =
{
  .commandNo = CMD_FS,
  .status = 0x0000,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = 0x0,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  /* default frequency 2405 = 0x0965 is 11 channel */
  .frequency = 0x0965,
  .fractFreq = 0x0000,
  /* 0-RX */
  .synthConf.bTxMode = 0x0,
  .synthConf.refFreq = 0x0,
  .__dummy0 = 0x00,
  .__dummy1 = 0x00,
  .__dummy2 = 0x00,
  .__dummy3 = 0x0000
};

/* CMD_TX_TEST
   Transmitter Test Command */
static rfc_CMD_TX_TEST_t RF_cmdTxTest =
{
  .commandNo = CMD_TX_TEST,
  .status = 0x0000,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = 0x0,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .config.bUseCw = 0x1,
  .config.bFsOff = 0x0,
  .config.whitenMode = 0x2,
  .__dummy0 = 0x00,
  .txWord = 0xFFFF,
  .__dummy1 = 0x00,
  .endTrigger.triggerType = 0x1,
  .endTrigger.bEnaCmd = 0x0,
  .endTrigger.triggerNo = 0x0,
  .endTrigger.pastTrig = 0x0,
  .syncWord = 0x71764129,
  .endTime = 0x00000000
};

/* CMD_IEEE_TX
   IEEE 802.15.4 Transmit Command */
static rfc_CMD_IEEE_TX_t RF_ieeeTX =
{
  .commandNo = CMD_IEEE_TX,
  .status = 0,
  .pNextOp = NULL,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_STOP_ON_FALSE,
  .condition.nSkip = 0x0,
  .txOpt.bIncludePhyHdr = 0, /* generate PHY header automatically */
  .txOpt.bIncludeCrc = 0, /* automatically generate CRC */
  .txOpt.payloadLenMsb = 0, /* must be nonzero only for test purposes */
  .payloadLen = 0,
  .pPayload = NULL,
  .timeStamp = 0 /* read-only field */
};

#ifdef ZB_SUBGHZ_BAND_ENABLED
/* Proprietary Mode Advanced Transmit Command */
static rfc_CMD_PROP_TX_ADV_t RF_propTX =
{
  .commandNo = CMD_PROP_TX_ADV,
  .status = 0x0000,
  .pNextOp = NULL,
  .startTime = 0,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x1,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .pktConf.bFsOff = 0x0,         /* 0: Keep frequency synth on after command. 1: Turn frequency synth off after command */
  .pktConf.bUseCrc = 0x1,        /* 0: Do not append CRC. 1: Append CRC */
  .pktConf.bCrcIncSw = 0x0,      /* 0: Do not include sync word in CRC calculation. 1: Include sync word in CRC calculation */
  .pktConf.bCrcIncHdr = 0x0,     /* 0: Do not include header in CRC calculation. 1: Include header in CRC calculation */
  .numHdrBits = 0x10,
  .pktLen = 0,
  .startConf.bExtTxTrig = 0x0,
  .startConf.inputMode = 0x0,
  .startConf.source = 0x0,
  .preTrigger.triggerType = 0x4,
  .preTrigger.bEnaCmd = 0x0,
  .preTrigger.triggerNo = 0x0,
  .preTrigger.pastTrig = 0x1,
  .preTime = 0x00000000,
  .syncWord = 0x0000904E,
  .pPkt = 0
};
#endif

typedef enum manuf_state_e
{
  MANUF_STATE_IDLE,
  MANUF_STATE_TONE,
  MANUF_STATE_STREAM,
  MANUF_STATE_PACKET,
  MANUF_STATE_RX,
  MANUF_STATE_TERMINATE
}
manuf_state_t;

typedef enum manuf_init_state_e
{
  MANUF_INIT_STATE_RX_ON_WHEN_IDLE,
  MANUF_INIT_STATE_CHANGE_PAGE,
  MANUF_INIT_STATE_CHANGE_CHANNEL,
  MANUF_INIT_STATE_APPLY_POWER,
  MANUF_INIT_STATE_SET_FS,
  MANUF_INIT_STATE_TERMINATE
}
manuf_init_state_t;

typedef struct manuf_obj_s
{
  zb_int8_t current_power_dbm; /* can be negative */
  zb_uint8_t current_page;
  zb_uint8_t current_channel;
  manuf_state_t manuf_state;
  manuf_init_state_t init_state;
  zb_callback_t tx_cb;
  zb_bufid_t tx_cb_arg;
  zb_callback_t rx_cb;
  RF_Handle rf_handle;
}
manuf_obj_t;

/* Check alignment for rfc_CMD_*_t and RF_Op.
 * All of them should be aligned by 4 because rfc_CMD_*_t is cast to RF_Op in functions like
 * RF_runCmd(), RF_postCmd() and RF_scheduleCmd(). */
ZB_ASSERT_IF_NOT_ALIGNED_TO_4(rfc_CMD_FS_t);
ZB_ASSERT_IF_NOT_ALIGNED_TO_4(rfc_CMD_TX_TEST_t);
ZB_ASSERT_IF_NOT_ALIGNED_TO_4(rfc_CMD_IEEE_TX_t);
ZB_ASSERT_IF_NOT_ALIGNED_TO_4(rfc_CMD_PROP_TX_ADV_t);
ZB_ASSERT_IF_NOT_ALIGNED_TO_4(RF_Op);

static manuf_obj_t m_obj;

static zb_ret_t rf_abort(void);
static zb_ret_t zb_ncp_manuf_apply_power(void);
static void ti13xx_rf_callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e);
static void zb_ncp_manuf_direct_pib_set(zb_uint8_t attr, void *value, zb_ushort_t value_size);
static void zb_ncp_manuf_direct_pib_set_cb(zb_uint8_t param);
static zb_ret_t zb_ncp_manuf_fill_cmdfs(void);
static zb_ret_t zb_ncp_manuf_run_cmdfs(void);
static void zb_ncp_manuf_send_packet_timeout(zb_bufid_t param);

zb_bool_t zb_ncp_manuf_check_request(zb_uint16_t call_id)
{
  zb_bool_t ret = ZB_FALSE;

  switch(m_obj.manuf_state)
  {
    case MANUF_STATE_TONE:
      switch(call_id)
      {
        case NCP_HL_MANUF_MODE_END:
        case NCP_HL_MANUF_GET_PAGE_AND_CHANNEL:
        case NCP_HL_MANUF_GET_POWER:
        case NCP_HL_MANUF_STOP_TONE:
          ret = ZB_TRUE;
          break;
        default:
          /* MISRA rule 16.4 - Mandatory default label */
          break;
      }
      break;
    case MANUF_STATE_STREAM:
      switch(call_id)
      {
        case NCP_HL_MANUF_MODE_END:
        case NCP_HL_MANUF_GET_PAGE_AND_CHANNEL:
        case NCP_HL_MANUF_GET_POWER:
        case NCP_HL_MANUF_STOP_STREAM_RANDOM:
          ret = ZB_TRUE;
          break;
        default:
          /* MISRA rule 16.4 - Mandatory default label */
          break;
      }
      break;
    case MANUF_STATE_PACKET:
      switch(call_id)
      {
        case NCP_HL_MANUF_MODE_END:
        case NCP_HL_MANUF_GET_PAGE_AND_CHANNEL:
        case NCP_HL_MANUF_GET_POWER:
          ret = ZB_TRUE;
          break;
        default:
          /* MISRA rule 16.4 - Mandatory default label */
          break;
      }
      break;
    case MANUF_STATE_RX:
      switch(call_id)
      {
        case NCP_HL_MANUF_MODE_END:
        case NCP_HL_MANUF_GET_PAGE_AND_CHANNEL:
        case NCP_HL_MANUF_GET_POWER:
        case NCP_HL_MANUF_STOP_TEST_RX:
          ret = ZB_TRUE;
          break;
        default:
          /* MISRA rule 16.4 - Mandatory default label */
          break;
      }
      break;
    default:
      ret = ZB_TRUE;
      break;
  }

  return ret;
}


/**
 * Init RF in sniffer mode with default channel and power
 */
void zb_ncp_manuf_init(zb_uint8_t page, zb_uint8_t channel)
{
  /* Unrealistically high value for obtaining the maximum allowed */
  m_obj.current_power_dbm = 125;
  m_obj.current_page = page;
  m_obj.current_channel = channel;
  m_obj.manuf_state = MANUF_STATE_IDLE;
  m_obj.init_state = MANUF_INIT_STATE_RX_ON_WHEN_IDLE;
  /* zboss_start_in_sniffer_mode don't fail */
  (void)zboss_start_in_sniffer_mode();
}

void zb_ncp_manuf_init_cont(zb_uint8_t param)
{
  zb_ret_t ret_power;

  TRACE_MSG(TRACE_MACLL2, "zb_ncp_manuf_init_cont m_obj.init_state %d", (FMT__D, m_obj.init_state));
  switch (m_obj.init_state)
  {
    case MANUF_INIT_STATE_RX_ON_WHEN_IDLE:
    {
      zb_uint8_t rx_on;
      m_obj.init_state = MANUF_INIT_STATE_CHANGE_PAGE;
      zb_set_rx_on_when_idle(ZB_FALSE);
#ifdef ZB_ED_RX_OFF_WHEN_IDLE
      rx_on = ZB_FALSE;
#else
      rx_on = ZB_PIBCACHE_RX_ON_WHEN_IDLE();
#endif
      zb_nwk_pib_set(param, ZB_PIB_ATTRIBUTE_RX_ON_WHEN_IDLE, &rx_on, 1, zb_ncp_manuf_init_cont);
      param = 0;
      break;
    }
    case MANUF_INIT_STATE_CHANGE_PAGE:
      m_obj.init_state = MANUF_INIT_STATE_CHANGE_CHANNEL;
      zb_ncp_manuf_direct_pib_set(ZB_PHY_PIB_CURRENT_PAGE, (void *)&m_obj.current_page, 1);
      break;
    case MANUF_INIT_STATE_CHANGE_CHANNEL:
      m_obj.init_state = MANUF_INIT_STATE_APPLY_POWER;
      zb_ncp_manuf_direct_pib_set(ZB_PHY_PIB_CURRENT_CHANNEL, (void *)&m_obj.current_channel, 1);
      break;
    case MANUF_INIT_STATE_APPLY_POWER:
      m_obj.init_state = MANUF_INIT_STATE_SET_FS;
#ifdef ZB_SUBGHZ_BAND_ENABLED
      m_obj.rf_handle = m_obj.current_page > 0U ? RF_SUBGHZ_CTX().rf_handle : RF_CTX().rf_handle;
      TRACE_MSG(TRACE_MACLL2, "m_obj.rf_handle %p RF_SUBGHZ_CTX().rf_handle %p RF_CTX().rf_handle %p",
                (FMT__P_P_P, m_obj.rf_handle, RF_SUBGHZ_CTX().rf_handle, RF_CTX().rf_handle));
#else
      m_obj.rf_handle = RF_CTX().rf_handle;
#endif
      ZB_ASSERT(m_obj.rf_handle != NULL);
      ret_power = zb_ncp_manuf_apply_power();
      ZB_ASSERT(ret_power == RET_OK);
      ZB_SCHEDULE_CALLBACK(zb_ncp_manuf_init_cont, 0);
      break;
    case MANUF_INIT_STATE_SET_FS:
      if (zb_ncp_manuf_fill_cmdfs() == RET_OK)
      {
        if (zb_ncp_manuf_run_cmdfs() == RET_OK)
        {
          m_obj.init_state = MANUF_INIT_STATE_RX_ON_WHEN_IDLE;
        }
      }
      break;
    default:
      break;
  }

  if (param != 0U)
  {
    zb_buf_free(param);
  }
}

static void zb_ncp_manuf_direct_pib_set_cb(zb_uint8_t param)
{
  zb_uint8_t value;
  zb_mlme_set_confirm_t *cfm = (zb_mlme_set_confirm_t *)zb_buf_begin(param);

  switch(cfm->pib_attr)
  {
    case ZB_PHY_PIB_CURRENT_PAGE:
      zb_ncp_manuf_init_cont(0);
      break;
    case ZB_PHY_PIB_CURRENT_CHANNEL:
      zb_ncp_manuf_init_cont(0);
      break;
    case ZB_PIB_ATTRIBUTE_PROMISCUOUS_MODE_CB:
      value = 1;
      zb_ncp_manuf_direct_pib_set(ZB_PIB_ATTRIBUTE_PROMISCUOUS_RX_ON, (void *)&value, sizeof(value));
      break;
    case ZB_PIB_ATTRIBUTE_PROMISCUOUS_RX_ON:
      break;
    default:
      break;
  }
}

static zb_ret_t zb_ncp_manuf_fill_cmdfs(void)
{
  zb_ret_t ret = RET_OK;

  if (m_obj.current_page == 0U)
  {
    RF_cmdFs.frequency = (zb_uint16_t)(2405UL + 5UL * ((zb_uint32_t)m_obj.current_channel - 11U));
    RF_cmdFs.fractFreq = 0U;
  }
#ifdef ZB_SUBGHZ_BAND_ENABLED
  else
  {
    zb_uint32_t freq;
    switch (m_obj.current_page)
    {
      case ZB_CHANNEL_PAGE28_SUB_GHZ:
      case ZB_CHANNEL_PAGE29_SUB_GHZ:
      case ZB_CHANNEL_PAGE30_SUB_GHZ:
        ZB_ASSERT(m_obj.current_channel <= 62U);
        freq = ZB_CHANNEL_PAGE28_SUB_GHZ_START_HZ + m_obj.current_channel * ZB_SUB_GHZ_GB_REGION_CHANNEL_SPACING;
        break;

      case ZB_CHANNEL_PAGE31_SUB_GHZ:
        ZB_ASSERT(m_obj.current_channel <= 26U);
        freq = ZB_CHANNEL_PAGE31_SUB_GHZ_START_HZ + m_obj.current_channel * ZB_SUB_GHZ_GB_REGION_CHANNEL_SPACING;
        break;

      default:
        TRACE_MSG(TRACE_ERROR, "zb_ncp_manuf_set_page_and_channel unknown page %hu", (FMT__H, m_obj.current_page));
        ret = RET_ERROR;
        ZB_ASSERT(ZB_FALSE);
        break;
    }
    if (ret == RET_OK)
    {
      RF_cmdFs.frequency = (zb_uint16_t)(freq / 1000000U);
      RF_cmdFs.fractFreq = (zb_uint16_t)(65536U * ((freq / 1000U) % 1000U) / 1000U);
    }
  }
#endif

  return ret;
}

static zb_ret_t zb_ncp_manuf_run_cmdfs(void)
{
  RF_EventMask rf_event;
  zb_ret_t ret = RET_OK;

  /*cstat !MISRAC2012-Rule-11.3 */
  rf_event = RF_runCmd(m_obj.rf_handle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);
  if (!ZB_BIT_IS_SET(rf_event, (RF_EventMask)(RF_EventCmdDone | RF_EventLastCmdDone)))
  {
    TRACE_MSG(TRACE_ERROR, "RF_runCmd failed [0x%x 0x%x]",
              (FMT__L_L, (zb_uint32_t)(rf_event >> 32U), (zb_uint32_t)(rf_event & 0xFFFFFFFFU)));
    ret = RET_ERROR;
    ZB_ASSERT(ZB_FALSE);
  }

  return ret;
}

zb_ret_t zb_ncp_manuf_set_page_and_channel(zb_uint8_t page, zb_uint8_t channel)
{
  zb_ret_t ret = RET_OK;

  if ((m_obj.current_page > 0U && page == 0U) ||
      (m_obj.current_page == 0U && page > 0U))
  {
    ret = RET_OPERATION_FAILED;
  }
  if (ret == RET_OK)
  {
    /* store a channel but won't set until get RX or TX command */
    m_obj.current_page = page;
    m_obj.current_channel = channel;

    ret = zb_ncp_manuf_fill_cmdfs();
  }

  return ret;
}

zb_uint8_t zb_ncp_manuf_get_page(void)
{
  return m_obj.current_page;
}

zb_uint8_t zb_ncp_manuf_get_channel(void)
{
  return m_obj.current_channel;
}

/* store the power value for applying on a next transmission */
void zb_ncp_manuf_set_power(zb_int8_t power_dbm)
{
  zb_ret_t ret;

  m_obj.current_power_dbm = power_dbm;
  ret = zb_ncp_manuf_apply_power();
  ZB_ASSERT(ret == RET_OK);
}

zb_int8_t zb_ncp_manuf_get_power(void)
{
  return m_obj.current_power_dbm;
}

static zb_ret_t zb_ncp_manuf_apply_power(void)
{
  zb_ret_t ret;

  ret = ZB_TRANSCEIVER_SET_TX_POWER(m_obj.current_power_dbm);
  /* Get power value back */
  ZB_TRANSCEIVER_GET_TX_POWER(&m_obj.current_power_dbm);

  return ret;
}

zb_ret_t zb_ncp_manuf_start_tone(void)
{
  zb_ret_t ret;
  RF_CmdHandle ref_handle;

  /* 1-TX */
  RF_cmdFs.synthConf.bTxMode = 1;

  ret = zb_ncp_manuf_run_cmdfs();

  if (ret == RET_OK)
  {
    m_obj.manuf_state = MANUF_STATE_TONE;

    /* set power when FS is enabled */
    zb_ret_t ret_power = zb_ncp_manuf_apply_power();
    ZB_ASSERT(ret_power == RET_OK);
    RF_cmdTxTest.config.bUseCw = 0x1;

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00003,2} */
    ref_handle = RF_postCmd(m_obj.rf_handle, (RF_Op*)(&RF_cmdTxTest), RF_PriorityNormal, NULL, 0);
    if (ref_handle < 0)
    {
      TRACE_MSG(TRACE_ERROR, "RF_postCmd failed [%d]", (FMT__D, ref_handle));
      ret = RET_ERROR;
    }
  }

  return ret;
}

void zb_ncp_manuf_stop_tone_or_stream(void)
{
  zb_ret_t ret;

  ret = rf_abort();
  if (ret == RET_OK)
  {
    m_obj.manuf_state = MANUF_STATE_IDLE;
  }
  else
  {
    TRACE_MSG(TRACE_ERROR, "rf_abort failed [%d]", (FMT__D, ret));
    ZB_ASSERT(ZB_FALSE);
  }
}

zb_ret_t zb_ncp_manuf_start_stream(zb_uint16_t tx_word)
{
  zb_ret_t ret;

  /* 1-TX */
  RF_cmdFs.synthConf.bTxMode = 1;

  ret = zb_ncp_manuf_run_cmdfs();

  if (ret == RET_OK)
  {
    RF_CmdHandle ref_handle;

    m_obj.manuf_state = MANUF_STATE_STREAM;

    /* set power when FS is enabled */
    zb_ret_t ret_power = zb_ncp_manuf_apply_power();
    ZB_ASSERT(ret_power == RET_OK);
    RF_cmdTxTest.config.bUseCw = 0x0;
    RF_cmdTxTest.config.whitenMode = 0x3;
    RF_cmdTxTest.txWord = tx_word;

    /*cstat !MISRAC2012-Rule-11.3 */
    /** @mdr{00003,4} */
    ref_handle = RF_postCmd(m_obj.rf_handle, (RF_Op*)(&RF_cmdTxTest), RF_PriorityNormal, NULL, 0);
    if (ref_handle < 0)
    {
      TRACE_MSG(TRACE_ERROR, "RF_postCmd failed [%d]", (FMT__D, ref_handle));
      ret = RET_ERROR;
    }
  }

  return ret;
}

static zb_ret_t rf_abort(void)
{
  RF_Stat rf_stat;
  uint32_t cmd;

  cmd = CMD_ABORT;
  rf_stat = RF_runImmediateCmd(m_obj.rf_handle, &cmd);

  return (rf_stat == RF_StatSuccess || rf_stat == RF_StatCmdDoneSuccess) ? RET_OK : RET_ERROR;
}

zb_ret_t zb_ncp_manuf_send_packet(zb_bufid_t param, zb_callback_t cb)
{
  zb_ret_t ret;
  zb_uint8_t *data;
  zb_uint8_t len;

  /* 1-TX */
  RF_cmdFs.synthConf.bTxMode = 1;

  ret = zb_ncp_manuf_run_cmdfs();

  if (ret == RET_OK)
  {
    RF_Op *cmd;
    RF_CmdHandle ref_handle;
    zb_uint32_t frame_duration;
    zb_uint32_t tx_symbols;

    m_obj.manuf_state = MANUF_STATE_PACKET;
    m_obj.tx_cb = cb;
    m_obj.tx_cb_arg = param;
    len = (zb_uint8_t)zb_buf_len(param);
    data = zb_buf_begin(param);
    param = 0;

    if (m_obj.current_page == 0U)
    {
      /* Data placed from third byte - SubGHz feature */
      RF_ieeeTX.pPayload = &data[2];
      RF_ieeeTX.payloadLen = len - 2U;
      RF_ieeeTX.pNextOp = NULL;
      RF_ieeeTX.status = 0;
      RF_ieeeTX.startTrigger.triggerType = TRIG_NOW;
      /*cstat !MISRAC2012-Rule-11.3 */
      /** @mdr{00003,6} */
      cmd = (RF_Op*)&RF_ieeeTX;
      tx_symbols = (zb_uint32_t)len * ZB_PHY_SYMBOLS_PER_OCTET;
      frame_duration = ZB_USECS_TO_MILLISECONDS(tx_symbols * ZB_SYMBOL_DURATION_USEC);
    }
#ifdef ZB_SUBGHZ_BAND_ENABLED
    else
    {
      /* MAC_TI_CC13XX_PHY_HDR_LEN already in len - need to subtract */
      data[0] = len - MAC_TI_CC13XX_PHY_HDR_LEN + MAC_TI_CC13XX_CRC_LEN;
      data[1] = MAC_TI_CC13XX_EXPECTED_FIRST_PHY_HDR_BYTE;
      zb_mac_flip_bits(&data[2], len - MAC_TI_CC13XX_PHY_HDR_LEN);
      RF_propTX.pPkt = data;
      RF_propTX.pktLen = len; /* +MAC_TI_CC13XX_PHY_HDR_LEN already in len */
      RF_propTX.startTrigger.triggerType = TRIG_NOW;
      RF_propTX.startTime = 0;
      RF_propTX.status = IDLE;
      /*cstat !MISRAC2012-Rule-11.3 */
      /** @mdr{00003,6} */
      cmd = (RF_Op*)&RF_propTX;
      tx_symbols = ZB_MAC_EU_FSK_SHR_LEN_SYMBOLS + ZB_MAC_EU_FSK_PHR_LEN_SYMBOLS + (zb_uint32_t)len * ZB_SUB_GHZ_PHY_SYMBOLS_PER_OCTET;
      frame_duration = ZB_USECS_TO_MILLISECONDS(tx_symbols * ZB_SUB_GHZ_SYMBOL_DURATION_USEC);
    }
#endif

    ref_handle = RF_postCmd(
      m_obj.rf_handle,
      cmd,
      RF_PriorityNormal,
      ti13xx_rf_callback,
      (RF_EventMask) RF_EventLastFGCmdDone | (RF_EventMask) RF_EventLastCmdDone
      );
    if (ref_handle < 0)
    {
      TRACE_MSG(TRACE_ERROR, "RF_postCmd failed [%d]", (FMT__D, ref_handle));
      ret = RET_ERROR;
    }
    ZB_SCHEDULE_ALARM_CANCEL(zb_ncp_manuf_send_packet_timeout, ZB_ALARM_ANY_PARAM);
    TRACE_MSG(TRACE_MACLL3, "scheduling zb_ncp_manuf_send_packet_timeout %u bi", (FMT__D, ZB_MILLISECONDS_TO_BEACON_INTERVAL(frame_duration) + 1U));
    ZB_SCHEDULE_ALARM(zb_ncp_manuf_send_packet_timeout, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(frame_duration) + 1U);
  }

  if (param != 0U)
  {
    zb_buf_free(param);
  }

  TRACE_MSG(TRACE_MACLL3, "zb_ncp_manuf_send_packet ret %d", (FMT__D, ret));

  return ret;
}

static void zb_ncp_manuf_send_packet_timeout(zb_bufid_t param)
{
  ZVUNUSED(param);
  (void)rf_abort();
  TRACE_MSG(TRACE_MACLL3, "zb_ncp_manuf_send_packet_timeout m_obj.tx_cb %p", (FMT__P, m_obj.tx_cb));
  if (m_obj.tx_cb != NULL)
  {
    zb_buf_set_status(m_obj.tx_cb_arg, RET_OPERATION_FAILED);
    (void)ZB_SCHEDULE_APP_CALLBACK(m_obj.tx_cb, m_obj.tx_cb_arg);
    m_obj.tx_cb = NULL;
    m_obj.tx_cb_arg = 0U;
  }
  if (m_obj.tx_cb_arg != 0U)
  {
    zb_buf_free(m_obj.tx_cb_arg);
    m_obj.tx_cb_arg = 0U;
  }
  m_obj.manuf_state = MANUF_STATE_IDLE;
}

static void ti13xx_rf_callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
  zb_ret_t status;
  uintptr_t swip_key;

  swip_key = SwiP_disable();

  TRACE_ENTER_INT();

  TRACE_MSG(TRACE_MACLL3, "ti13xx_rf_callback h %p ch %p e (hi 0x%lx lo 0x%lx)",
            (FMT__P_P_L_L, h, ch, (zb_uint32_t)((e>>32) & 0xFFFFFFFF), (zb_uint32_t)(e & 0xFFFFFFFF)));

  if (m_obj.tx_cb != NULL)
  {
    if (ZB_BIT_IS_SET(e, (RF_EventMask)(RF_EventLastFGCmdDone | RF_EventLastCmdDone)))
    {
      status = RET_OK;
    }
    else
    {
      status = RET_OPERATION_FAILED;
    }
    zb_buf_set_status(m_obj.tx_cb_arg, status);
    ZB_SCHEDULE_ALARM_CANCEL(zb_ncp_manuf_send_packet_timeout, ZB_ALARM_ANY_PARAM);
    (void)ZB_SCHEDULE_APP_CALLBACK(m_obj.tx_cb, m_obj.tx_cb_arg);

    m_obj.tx_cb = NULL;
    m_obj.tx_cb_arg = 0U;
  }

  m_obj.manuf_state = MANUF_STATE_IDLE;

  TRACE_LEAVE_INT();
  SwiP_restore(swip_key);
}

zb_ret_t zb_ncp_manuf_start_rx(zb_callback_t cb)
{
  zb_ret_t ret;

  ret = zb_ncp_manuf_run_cmdfs();

  if (ret == RET_OK)
  {
    m_obj.manuf_state = MANUF_STATE_RX;
    m_obj.rx_cb = cb;
    if (m_obj.current_page == 0U)
    {
      /* receive packets with CRC error */
      RF_cmdIeeeRx_ieee154.rxConfig.bAutoFlushCrc = 0,
      /* receive packets that are ignored */
      RF_cmdIeeeRx_ieee154.rxConfig.bAutoFlushIgn = 0;
      /* don't ignore packets */
      RF_cmdIeeeRx_ieee154.frameTypes.bAcceptFt0Beacon = 1;
      RF_cmdIeeeRx_ieee154.frameTypes.bAcceptFt1Data = 1;
      RF_cmdIeeeRx_ieee154.frameTypes.bAcceptFt2Ack = 1;
      RF_cmdIeeeRx_ieee154.frameTypes.bAcceptFt3MacCmd = 1;
      RF_cmdIeeeRx_ieee154.frameTypes.bAcceptFt4Reserved = 1;
      RF_cmdIeeeRx_ieee154.frameTypes.bAcceptFt5Reserved = 1;
      RF_cmdIeeeRx_ieee154.frameTypes.bAcceptFt6Reserved = 1;
      RF_cmdIeeeRx_ieee154.frameTypes.bAcceptFt7Reserved = 1;
    }
#ifdef ZB_SUBGHZ_BAND_ENABLED
    else
    {
      /* Nothing to do for SubGHz */
    }
#endif
    zb_ncp_manuf_direct_pib_set(ZB_PIB_ATTRIBUTE_PROMISCUOUS_MODE_CB, (void *)&m_obj.rx_cb, sizeof(m_obj.rx_cb));
  }

  return ret;
}

void zb_ncp_manuf_stop_rx(void)
{
  zb_uint8_t value = 0;

  m_obj.manuf_state = MANUF_STATE_IDLE;
  zb_ncp_manuf_direct_pib_set(ZB_PIB_ATTRIBUTE_PROMISCUOUS_RX_ON, (void *)&value, sizeof(value));
}

static void zb_ncp_manuf_direct_pib_set(zb_uint8_t attr, void *value, zb_ushort_t value_size)
{
  zb_bufid_t buf = zb_buf_get_any();

  if (buf != 0U)
  {
    zb_mlme_set_request_t *req;

    TRACE_MSG(TRACE_MAC2, "zb_ncp_manuf_direct_pib_set: attr = %d", (FMT__D, attr));
    req = zb_buf_initial_alloc(buf, sizeof(zb_mlme_set_request_t) + value_size);
    req->pib_attr = attr;
    req->pib_index = 0;
    req->pib_length = (zb_uint8_t)value_size;
    ZB_MEMCPY((req+1), value, value_size);
    req->confirm_cb_u.cb = zb_ncp_manuf_direct_pib_set_cb;
    zb_mlme_set_request(buf);
  }
}
