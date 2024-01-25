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
/*  PURPOSE: NCP High level adapters: NVRAM
*/
#define ZB_TRACE_FILE_ID 17515

#define ZB_USE_NVRAM
#include "ncp_host_hl_transport_internal_api.h"
#include "zb_nvram.h"
#include "ncp_host_hl_adapter.h"
#include "ncp_hl_proto.h"

/* This size refers to NCP transport buffer size */
#define NVRAM_WRITE_BUFFER_SIZE 1024

/* this buffer aggregates datasets data to be sent to SoC as a single transaction */
typedef struct nvram_write_buffer_s
{
    zb_uint8_t data[NVRAM_WRITE_BUFFER_SIZE];
    zb_uindex_t body_pos;
    zb_uindex_t pos;
} nvram_write_buffer_t;


typedef struct nvram_read_buffer_s
{
    zb_uint8_t *data;
    zb_size_t len;
} nvram_read_buffer_t;

typedef struct ncp_host_nvram_adapter_ctx_s
{
    nvram_write_buffer_t write_buf;
    nvram_read_buffer_t read_buf;
    zb_nvram_read_app_data_t read_app_data_cb[ZB_NVRAM_APP_DATASET_NUMBER];
    zb_nvram_write_app_data_t write_app_data_cb[ZB_NVRAM_APP_DATASET_NUMBER];
    zb_nvram_get_app_data_size_t get_app_data_size_cb[ZB_NVRAM_APP_DATASET_NUMBER];
    zb_uint8_t transaction_state; /** @see @ref nvram_transaction_state */
} ncp_host_nvram_adapter_ctx_t;

static ncp_host_nvram_adapter_ctx_t g_adapter_ctx;


static void nvram_send_write_request(void);


static void nvram_set_read_buf(zb_uint8_t *buf, zb_size_t len)
{
    g_adapter_ctx.read_buf.data = buf;
    g_adapter_ctx.read_buf.len = len;
}


static void nvram_reset_read_buf(void)
{
    g_adapter_ctx.read_buf.data = NULL;
    g_adapter_ctx.read_buf.len = 0;
}


void ncp_host_nvram_adapter_init_ctx(void)
{
    ZB_BZERO(&g_adapter_ctx, sizeof(g_adapter_ctx));
    g_adapter_ctx.transaction_state = ZB_NVRAM_TRANSACTION_AUTOCOMMIT;
}


void zb_nvram_register_app1_read_cb(zb_nvram_read_app_data_t cb)
{
    TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app1_read_cb, cb %p", (FMT__P, cb));
    g_adapter_ctx.read_app_data_cb[0] = cb;
}


void zb_nvram_register_app2_read_cb(zb_nvram_read_app_data_t cb)
{
    TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app2_read_cb, cb %p", (FMT__P, cb));
    g_adapter_ctx.read_app_data_cb[1] = cb;
}


void zb_nvram_register_app3_read_cb(zb_nvram_read_app_data_t cb)
{
    TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app3_read_cb, cb %p", (FMT__P, cb));
    g_adapter_ctx.read_app_data_cb[2] = cb;
}


void zb_nvram_register_app4_read_cb(zb_nvram_read_app_data_t cb)
{
    TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app4_read_cb, cb %p", (FMT__P, cb));
    g_adapter_ctx.read_app_data_cb[3] = cb;
}



void zb_nvram_register_app1_write_cb(zb_nvram_write_app_data_t wcb,
                                     zb_nvram_get_app_data_size_t gcb)
{
    TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app1_write_cb, wcb %p, gcp %p", (FMT__P_P, wcb, gcb));
    g_adapter_ctx.write_app_data_cb[0] = wcb;
    g_adapter_ctx.get_app_data_size_cb[0] = gcb;
}


void zb_nvram_register_app2_write_cb(zb_nvram_write_app_data_t wcb,
                                     zb_nvram_get_app_data_size_t gcb)
{
    TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app2_write_cb, wcb %p, gcp %p", (FMT__P_P, wcb, gcb));
    g_adapter_ctx.write_app_data_cb[1] = wcb;
    g_adapter_ctx.get_app_data_size_cb[1] = gcb;
}


void zb_nvram_register_app3_write_cb(zb_nvram_write_app_data_t wcb,
                                     zb_nvram_get_app_data_size_t gcb)
{
    TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app3_write_cb, wcb %p, gcp %p", (FMT__P_P, wcb, gcb));
    g_adapter_ctx.write_app_data_cb[2] = wcb;
    g_adapter_ctx.get_app_data_size_cb[2] = gcb;
}


void zb_nvram_register_app4_write_cb(zb_nvram_write_app_data_t wcb,
                                     zb_nvram_get_app_data_size_t gcb)
{
    TRACE_MSG(TRACE_COMMON1, "zb_nvram_register_app4_write_cb, wcb %p, gcp %p", (FMT__P_P, wcb, gcb));
    g_adapter_ctx.write_app_data_cb[3] = wcb;
    g_adapter_ctx.get_app_data_size_cb[3] = gcb;
}


static zb_size_t nvram_buf_get_avail_len(void)
{
    zb_size_t ret;

    ret = NVRAM_WRITE_BUFFER_SIZE - g_adapter_ctx.write_buf.pos;

    ret -= sizeof(ncp_hl_nvram_write_req_hdr_t);

    /* command header is not written yet, so we should subtract its size as well */
    if (g_adapter_ctx.write_buf.pos == 0)
    {
        ret -= sizeof(ncp_hl_nvram_write_req_ds_hdr_t);
    }

    return ret;
}


static zb_uindex_t nvram_write_dataset_header(zb_nvram_dataset_types_t t,
        zb_uint16_t dataset_version,
        zb_uint16_t data_len)
{
    nvram_write_buffer_t *buf = &g_adapter_ctx.write_buf;
    ncp_hl_nvram_write_req_hdr_t *common_header = (ncp_hl_nvram_write_req_hdr_t *)&buf->data[0];
    ncp_hl_nvram_write_req_ds_hdr_t *ds_hdr;
    zb_uindex_t ret;

    TRACE_MSG(TRACE_COMMON2, ">> nvram_write_dataset_header, type %d, version %d, len %d",
              (FMT__D_D_D, t, dataset_version, data_len));

    if (buf->pos == 0)
    {
        common_header->dataset_qnt = 0;
        buf->pos += sizeof(*common_header);
    }

    common_header->dataset_qnt++;

    ds_hdr = (ncp_hl_nvram_write_req_ds_hdr_t *)&buf->data[buf->pos];
    buf->pos += sizeof(*ds_hdr);
    ds_hdr->type = t;
    ds_hdr->version = dataset_version;
    ds_hdr->len = data_len;

    ret = buf->pos;
    buf->body_pos = buf->pos;
    buf->pos += data_len;

    TRACE_MSG(TRACE_COMMON2, "<< nvram_write_dataset_header", (FMT__0));

    return ret;
}


static zb_uint16_t nvram_get_dataset_body_length(zb_nvram_dataset_types_t t)
{
    zb_uint16_t ret = 0;

    switch (t)
    {
#ifdef ZB_ENABLE_HA
    case ZB_NVRAM_HA_DATA:
        ret = sizeof(zb_nvram_dataset_ha_t);
        break;
#endif /* ZB_ENABLE_HA */

#if defined ZB_ENABLE_ZCL && !defined ZB_ZCL_DISABLE_REPORTING
    case ZB_NVRAM_ZCL_REPORTING_DATA:
        ret = zb_nvram_zcl_reporting_dataset_length();
        break;
#endif /* (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)) */

#ifdef ZB_HA_ENABLE_POLL_CONTROL_SERVER
    case ZB_NVRAM_HA_POLL_CONTROL_DATA:
        ret = sizeof(zb_nvram_dataset_poll_control_t);
        break;
#endif /* defined ZB_HA_ENABLE_POLL_CONTROL_SERVER */

    case ZB_NVRAM_APP_DATA1:
        if (g_adapter_ctx.get_app_data_size_cb[0] != NULL)
        {
            ret = g_adapter_ctx.get_app_data_size_cb[0]();
        }
        break;
    case ZB_NVRAM_APP_DATA2:
        if (g_adapter_ctx.get_app_data_size_cb[1] != NULL)
        {
            ret = g_adapter_ctx.get_app_data_size_cb[1]();
        }
        break;
    case ZB_NVRAM_APP_DATA3:
        if (g_adapter_ctx.get_app_data_size_cb[2] != NULL)
        {
            ret = g_adapter_ctx.get_app_data_size_cb[2]();
        }
        break;
    case ZB_NVRAM_APP_DATA4:
        if (g_adapter_ctx.get_app_data_size_cb[3] != NULL)
        {
            ret = g_adapter_ctx.get_app_data_size_cb[3]();
        }
        break;

    default:
        TRACE_MSG(TRACE_ERROR, "invalid dataset type: %d", (FMT__D, t));
        ZB_ASSERT(0);
        break;
    }

    return ret;
}


static zb_uint16_t nvram_get_dataset_version(zb_nvram_dataset_types_t ds_type)
{
    zb_uint16_t ds_version = ZB_NVRAM_DATA_SET_VERSION_NOT_AVAILABLE;

    switch (ds_type)
    {
#if defined ZB_ENABLE_HA
    case ZB_NVRAM_HA_DATA:
        ds_version = ZB_NVRAM_HA_DATA_DS_VER;
        break;
#endif /*defined ZB_ENABLE_HA*/

#if (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING))
    case ZB_NVRAM_ZCL_REPORTING_DATA:
        ds_version = ZB_NVRAM_ZCL_REPORTING_DATA_DS_VER;
        break;
#endif /* (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)) */

    case ZB_NVRAM_APP_DATA1:
        /* Application controls the version of application dataset by its own */
        break;
    case ZB_NVRAM_APP_DATA2:
        /* Application controls the version of application dataset by its own */
        break;
    case ZB_NVRAM_APP_DATA3:
        /* Application controls the version of application dataset by its own */
        break;
    case ZB_NVRAM_APP_DATA4:
        /* Application controls the version of application dataset by its own */
        break;

    default:
        TRACE_MSG(TRACE_ERROR, "Unknown dataset type", (FMT__0));
        break;
    }

    return ds_version;
}


zb_bool_t ncp_host_nvram_dataset_is_supported(zb_nvram_dataset_types_t t)
{
    zb_ret_t ret;

    switch (t)
    {
    case ZB_NVRAM_HA_DATA:
    case ZB_NVRAM_ZCL_REPORTING_DATA:
    case ZB_NVRAM_HA_POLL_CONTROL_DATA:
    case ZB_NVRAM_APP_DATA1:
    case ZB_NVRAM_APP_DATA2:
    case ZB_NVRAM_APP_DATA3:
    case ZB_NVRAM_APP_DATA4:
        ret = ZB_TRUE;
        break;
    default:
        ret = ZB_FALSE;
    }

    return ret;
}


static zb_ret_t nvram_write_dataset_body(zb_nvram_dataset_types_t t, zb_uindex_t pos)
{
    zb_uint8_t page = 0;
    zb_ret_t ret;

    switch (t)
    {
#if defined ZB_ENABLE_HA
    case ZB_NVRAM_HA_DATA:
        ret = zb_nvram_write_ha_dataset(page, pos);
        break;
#endif /* defined ZB_ENABLE_HA */

#if defined ZB_ENABLE_ZCL && !defined ZB_ZCL_DISABLE_REPORTING
    case ZB_NVRAM_ZCL_REPORTING_DATA:
        ret = zb_nvram_write_zcl_reporting_dataset(page, pos);
        break;
#endif /* (defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)) */

#if defined ZB_HA_ENABLE_POLL_CONTROL_SERVER
    case ZB_NVRAM_HA_POLL_CONTROL_DATA:
        ret = zb_nvram_write_poll_control_dataset(page, pos);
        break;
#endif /* defined ZB_HA_ENABLE_POLL_CONTROL_SERVER */

    case ZB_NVRAM_APP_DATA1:
        if (g_adapter_ctx.write_app_data_cb[0] != NULL)
        {
            ret = g_adapter_ctx.write_app_data_cb[0](page, pos);
        }
        else
        {
            ret = RET_ERROR;
        }
        break;
    case ZB_NVRAM_APP_DATA2:
        if (g_adapter_ctx.write_app_data_cb[1] != NULL)
        {
            ret = g_adapter_ctx.write_app_data_cb[1](page, pos);
        }
        else
        {
            ret = RET_ERROR;
        }
        break;
    case ZB_NVRAM_APP_DATA3:
        if (g_adapter_ctx.write_app_data_cb[2] != NULL)
        {
            ret = g_adapter_ctx.write_app_data_cb[2](page, pos);
        }
        else
        {
            ret = RET_ERROR;
        }
        break;
    case ZB_NVRAM_APP_DATA4:
        if (g_adapter_ctx.write_app_data_cb[3] != NULL)
        {
            ret = g_adapter_ctx.write_app_data_cb[3](page, pos);
        }
        else
        {
            ret = RET_ERROR;
        }
        break;

    default:
        TRACE_MSG(TRACE_ERROR, "unsupported dataset: %d", (FMT__D, t));
        ZB_ASSERT(0);
        ret = RET_ERROR;
        break;
    }

    return ret;
}


zb_ret_t zb_nvram_write_dataset(zb_nvram_dataset_types_t t)
{
    zb_ret_t ret;
    zb_size_t dataset_len;
    zb_uint16_t dataset_version;
    zb_size_t avail_len;
    zb_uindex_t dataset_body_pos;

    TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_write_dataset, type %d", (FMT__D, t));

    do
    {
        if (!ncp_host_nvram_dataset_is_supported(t))
        {
            TRACE_MSG(TRACE_ERROR, "invalid dataset type: %d", (FMT__D, t));
            ret = RET_INVALID_PARAMETER;
            break;
        }

        dataset_len = nvram_get_dataset_body_length(t);
        dataset_version = nvram_get_dataset_version(t);
        avail_len = nvram_buf_get_avail_len();
        if (dataset_len > avail_len)
        {
            TRACE_MSG(TRACE_ERROR, "can't write a dataset body, need %d bytes, available %d",
                      (FMT__D_D, dataset_len, avail_len));
            ret = RET_ERROR;
            break;
        }
        if (dataset_len == 0)
        {
            ret = RET_OK;
            TRACE_MSG(TRACE_ERROR, "dataset len is zero, didn't write anything", (FMT__0));
            break;
        }

        dataset_body_pos = nvram_write_dataset_header(t, dataset_version, dataset_len);

        ret = nvram_write_dataset_body(t, dataset_body_pos);

        if (ret != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "failed to write a dataset body, ret %d", (FMT__D, ret));
            break;
        }

        if (g_adapter_ctx.transaction_state == ZB_NVRAM_TRANSACTION_AUTOCOMMIT)
        {
            nvram_send_write_request();
        }

    } while (0);

    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_dataset, ret %d", (FMT__D, ret));

    /* TODO: handle the error code in the place where this function is called
             or in some other error handler. See ZOI-616. */
    ZB_ASSERT(ret == RET_OK);

    return ret;
}


void zb_nvram_transaction_start(void)
{
    TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_transaction_start", (FMT__0));

    g_adapter_ctx.transaction_state = ZB_NVRAM_TRANSACTION_ONGOING;

    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_transaction_start", (FMT__0));
}


void zb_nvram_transaction_commit(void)
{
    TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_transaction_start", (FMT__0));

    g_adapter_ctx.transaction_state = ZB_NVRAM_TRANSACTION_AUTOCOMMIT;

    nvram_send_write_request();

    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_transaction_start", (FMT__0));
}


zb_ret_t zb_nvram_write_data(zb_uint8_t page, zb_uint32_t pos, zb_uint8_t *buf, zb_uint16_t len)
{
    zb_ret_t ret = RET_OK;

    TRACE_MSG(TRACE_COMMON1, ">> zb_nvram_write_data, page %d, pos %d, buf %p, len %d",
              (FMT__D_D_P_D, page, pos, buf, len));

    do
    {
        if (page != 0)
        {
            TRACE_MSG(TRACE_ERROR, "invalid page %d", (FMT__D, page));
            ret = RET_INVALID_PARAMETER_1;
            break;
        }

        if (pos < g_adapter_ctx.write_buf.body_pos || pos >= g_adapter_ctx.write_buf.pos)
        {
            TRACE_MSG(TRACE_ERROR, "invalid pos %d, should be between %d and %d",
                      (FMT__D_D_D, pos, g_adapter_ctx.write_buf.body_pos, g_adapter_ctx.write_buf.pos));
            ret = RET_INVALID_PARAMETER_2;
            break;
        }

        if (len > g_adapter_ctx.write_buf.pos - g_adapter_ctx.write_buf.body_pos)
        {
            TRACE_MSG(TRACE_ERROR, "invalid len, %d, should be no more than %d",
                      (FMT__D_D, len, g_adapter_ctx.write_buf.pos - g_adapter_ctx.write_buf.body_pos));
            ret = RET_INVALID_PARAMETER_4;
            break;
        }

        ZB_MEMCPY(&g_adapter_ctx.write_buf.data[g_adapter_ctx.write_buf.body_pos], buf, len);
        g_adapter_ctx.write_buf.body_pos += len;

    } while (0);

    TRACE_MSG(TRACE_COMMON1, "<< zb_nvram_write_data, ret %d", (FMT__D, ret));

    ZB_ASSERT(ret == RET_OK);
    return ret;
}


zb_ret_t zb_nvram_read_data(zb_uint8_t page, zb_uint32_t pos, zb_uint8_t *buf, zb_uint16_t len)
{
    zb_ret_t ret = RET_OK;
    TRACE_MSG(TRACE_COMMON2, ">> zb_nvram_read_data, page %d, pos %d, buf %p, len %d",
              (FMT__D_D_P_D, page, pos, buf, len));

    do
    {
        if (page != 0)
        {
            TRACE_MSG(TRACE_ERROR, "incorrect page %d, expected %d", (FMT__D, page, 0));
            ret = RET_INVALID_PARAMETER_1;
            break;
        }

        if (pos > g_adapter_ctx.read_buf.len)
        {
            TRACE_MSG(TRACE_ERROR, "position %d is out of range [0; %d)",
                      (FMT__D, pos, g_adapter_ctx.read_buf.len));
            ret = RET_INVALID_PARAMETER_2;
            break;
        }

        if (pos + len > g_adapter_ctx.read_buf.len)
        {
            TRACE_MSG(TRACE_ERROR, "incorrect_length %d, max allowed len %d",
                      (FMT__D_D, len, g_adapter_ctx.read_buf.len - pos));
            ret = RET_INVALID_PARAMETER_4;
            break;
        }

        ZB_MEMCPY(buf, &g_adapter_ctx.read_buf.data[pos], len);
    } while (0);

    TRACE_MSG(TRACE_COMMON2, "<< zb_nvram_read_data, ret %d", (FMT__D, ret));
    return ret;
}


static void nvram_send_write_request(void)
{
    zb_ret_t ret;

    ret = ncp_host_nvram_write_request(g_adapter_ctx.write_buf.data, g_adapter_ctx.write_buf.pos);
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "failed to send an nvram write request, ret %d", (FMT__D, ret));
        ZB_ASSERT(0);
    }

    g_adapter_ctx.write_buf.pos = 0;
    g_adapter_ctx.write_buf.body_pos = 0;
}


void zb_nvram_clear(void)
{
    zb_ret_t ret;

    ret = ncp_host_nvram_clear_request();
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "failed to send an nvram clear request, ret %d", (FMT__D, ret));
        ZB_ASSERT(0);
    }
}


void zb_nvram_erase(void)
{
    zb_ret_t ret;

    ret = ncp_host_nvram_erase_request();
    if (ret != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "failed to send an nvram erase request, ret %d", (FMT__D, ret));
        ZB_ASSERT(0);
    }
}


static void nvram_read_app_data(zb_uint8_t i,
                                zb_uint8_t page,
                                zb_uint32_t pos,
                                zb_uint16_t payload_length)
{
    TRACE_MSG(TRACE_COMMON1, "nvram_read_app_data1 %hd pos %ld len %d", (FMT__H_L_D, page, pos, payload_length));
    if (g_adapter_ctx.read_app_data_cb[i] != NULL)
    {
        g_adapter_ctx.read_app_data_cb[i](page, pos, payload_length);
    }
}


zb_ret_t ncp_host_nvram_read_dataset(zb_nvram_dataset_types_t type,
                                     zb_uint8_t *buf, zb_uint16_t ds_len,
                                     zb_uint16_t ds_ver, zb_nvram_ver_t nvram_ver)
{
    zb_ret_t ret;
    TRACE_MSG(TRACE_COMMON1, ">> ncp_host_nvram_read_dataset, type %d, buf %p, len %d, ver %d",
              (FMT__D_P_D_D, type, buf, ds_len, ds_ver));

    if (!ncp_host_nvram_dataset_is_supported(type))
    {
        TRACE_MSG(TRACE_ERROR, "unsupported dataset, type %d", (FMT__D, type));
        ret = RET_ERROR;
        ZB_ASSERT(0);
    }
    else
    {
        zb_uint8_t page = 0;
        zb_uint16_t pos = 0;

        nvram_set_read_buf(buf, ds_len);

        switch (type)
        {
#if defined ZB_ENABLE_HA
        case ZB_NVRAM_HA_DATA:
            TRACE_MSG(TRACE_COMMON1, "nvram load: read ha dataset", (FMT__0));
            zb_nvram_read_ha_dataset(page, pos, ds_len, nvram_ver, ds_ver);
            break;
#endif /* defined ZB_ENABLE_HA*/

#if defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING)
        case ZB_NVRAM_ZCL_REPORTING_DATA:
            TRACE_MSG(TRACE_COMMON1, "nvram load: read zcl reporting dataset", (FMT__0));
            zb_nvram_read_zcl_reporting_dataset(page, pos, ds_len, nvram_ver, ds_ver);
            break;
#endif /* defined(ZB_ENABLE_ZCL) && !(defined ZB_ZCL_DISABLE_REPORTING) */

#if defined ZB_HA_ENABLE_POLL_CONTROL_SERVER
        case ZB_NVRAM_HA_POLL_CONTROL_DATA:
            TRACE_MSG(TRACE_COMMON1, "nvram load: read ha poll control dataset", (FMT__0));
            zb_nvram_read_poll_control_dataset(page, pos, ds_len, nvram_ver, ds_ver);
            break;
#endif /* defined ZB_HA_ENABLE_POLL_CONTROL_SERVER */

        case ZB_NVRAM_APP_DATA1:
            TRACE_MSG(TRACE_COMMON1, "nvram load: read app data1", (FMT__0));
            nvram_read_app_data(0, page, pos, ds_len);
            break;
        case ZB_NVRAM_APP_DATA2:
            TRACE_MSG(TRACE_COMMON1, "nvram load: read app data2", (FMT__0));
            nvram_read_app_data(1, page, pos, ds_len);
            break;
        case ZB_NVRAM_APP_DATA3:
            TRACE_MSG(TRACE_COMMON1, "nvram load: read app data3", (FMT__0));
            nvram_read_app_data(2, page, pos, ds_len);
            break;
        case ZB_NVRAM_APP_DATA4:
            TRACE_MSG(TRACE_COMMON1, "nvram load: read app data4", (FMT__0));
            nvram_read_app_data(3, page, pos, ds_len);
            break;

        default:
            break;
        }
        nvram_reset_read_buf();
        ret = RET_OK;
    }

    TRACE_MSG(TRACE_COMMON1, "<< ncp_host_nvram_read_dataset, ret %d", (FMT__D, ret));
    return ret;
}


void ncp_host_handle_nvram_clear_response(zb_ret_t status)
{
    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_handle_nvram_clear_response, status 0x%x",
              (FMT__D, status));

    ZB_ASSERT(status == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_handle_nvram_clear_response", (FMT__0));
}


void ncp_host_handle_nvram_erase_response(zb_ret_t status)
{
    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_handle_nvram_erase_response, status 0x%x",
              (FMT__D, status));

    ZB_ASSERT(status == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_handle_nvram_erase_response", (FMT__0));
}


void ncp_host_handle_nvram_write_response(zb_ret_t status)
{
    TRACE_MSG(TRACE_TRANSPORT2, ">> ncp_host_handle_nvram_write_response, status 0x%x",
              (FMT__D, status));

    ZB_ASSERT(status == RET_OK);

    TRACE_MSG(TRACE_TRANSPORT2, "<< ncp_host_handle_nvram_write_response", (FMT__0));
}
