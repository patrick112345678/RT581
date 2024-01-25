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
/* PURPOSE: Light Coordinator
*/

#define ZB_TRACE_FILE_ID 40010
#include "light_zc_HA_1_2_bulb.h"
#ifdef ZB_USE_BUTTONS
#include "light_zc_HA_1_2_bulb_hal.h"
#endif

zb_ieee_addr_t g_zc_addr = LIGHT_ZC_ADDRESS;

#ifdef ZC_AUTO_SEARCH_AND_BIND
light_zc_ctx_t zc_ctx;
#endif

#ifndef ZB_COORDINATOR_ROLE
#error Coordinator role is not compiled!
#endif

#ifdef ZC_AUTO_SEARCH_AND_BIND
static void cleanup_dev(simple_device_t *dev)
{
    ZB_BZERO(dev, sizeof(simple_device_t));
    dev->last_zdo_tsn = 0xFF;
    dev->assign_idx = 0xFF;

    {
        zb_uint8_t j;

        for (j = 0; j < (LIGHT_ZC_MAX_DEVICES - 1); ++j)
        {
            dev->assign_table[j] = 0xFF;
        }
    }
}

static void cleanup_assign_table(simple_device_t *dev)
{
    zb_uint8_t i;
    zb_uint8_t dev_idx = ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev);

    TRACE_MSG(TRACE_APP1, "cleanup assign table dev at idx: %hd", (FMT__H, dev_idx));

    for (i = 0; i < LIGHT_ZC_MAX_DEVICES; ++i)
    {
        zb_uint8_t j;

        for (j = 0; j < (LIGHT_ZC_MAX_DEVICES - 1); ++j)
        {
            if (zc_ctx.devices[i].assign_table[j] == dev_idx)
            {
                //TODO: send unbind request
                zc_ctx.devices[i].assign_table[j] = 0xFF;
            }
        }
    }
}

static void init_ctx()
{
    zb_uint8_t i;

    ZB_BZERO(&zc_ctx, sizeof(zc_ctx));

    for (i = 0; i < LIGHT_ZC_MAX_DEVICES; ++i)
    {
        cleanup_dev(&zc_ctx.devices[i]);
    }
}

#endif  /* ZC_AUTO_SEARCH_AND_BIND */

MAIN()
{
    ARGV_UNUSED;

    ZB_SET_TRACE_ON();
    ZB_SET_TRAF_DUMP_ON();

#ifdef ZC_AUTO_SEARCH_AND_BIND
    init_ctx();
#endif

    ZB_INIT("zdo_zc");

#ifdef ZB_USE_BUTTONS
    light_zc_hal_init();
#endif

    zb_set_long_address(g_zc_addr);
    zb_set_network_coordinator_role(LIGHT_ZC_CHANNEL_MASK);
    zb_set_max_children(2);
#ifdef ZB_USE_BUTTONS
    /* Erase NVRAM if BUTTON2 is pressed on start */
    zb_set_nvram_erase_at_start(light_zc_hal_is_button_pressed(LIGHT_ZC_BUTTON_2_IDX));
#else
    /*
    Do not erase NVRAM to save the network parameters after device reboot or power-off
    NOTE: If this option is set to ZB_FALSE then do full device erase for all network
    devices before running other samples.
    */
    zb_set_nvram_erase_at_start(ZB_FALSE);
#endif

    if (zboss_start() != RET_OK)
    {
        TRACE_MSG(TRACE_ERROR, "zdo_dev_start failed", (FMT__0));
    }
    else
    {
        zboss_main_loop();
    }

    TRACE_DEINIT();

    MAIN_RETURN(0);
}

#ifdef ZC_AUTO_SEARCH_AND_BIND

static simple_device_t *device_find_by_zdo_tsn(zb_uint8_t zdo_tsn)
{
    zb_uint8_t i;

    for (i = 0; i < LIGHT_ZC_MAX_DEVICES; ++i)
    {
        if (zc_ctx.devices[i].last_zdo_tsn == zdo_tsn)
        {
            TRACE_MSG(TRACE_APP1, "found device by tsn: %hd", (FMT__H, zdo_tsn));
            return &zc_ctx.devices[i];
        }
    }

    TRACE_MSG(TRACE_ERROR, "find device by tsn: %hd failed", (FMT__H, zdo_tsn));
    return NULL;
}

static zb_ret_t device_bind(zb_uint8_t       buf_ref,
                            simple_device_t *dev,
                            simple_device_t *light,
                            simple_device_t *light_control);
static void device_assign(zb_uint8_t buf_ref, zb_uint16_t dev_idx);

static void device_bind_cb(zb_uint8_t buf_ref)
{
    zb_bufid_t buf = buf_ref;
    zb_zdo_bind_resp_t *resp = (zb_zdo_bind_resp_t *)zb_buf_begin(buf);
    simple_device_t    *dev;

    TRACE_MSG(TRACE_APP1, "> device_bind_cb buf_ref %hd", (FMT__H, buf_ref));

    dev = device_find_by_zdo_tsn(resp->tsn);

    if (dev != NULL)
    {
        dev->last_zdo_tsn = 0xFF;

#ifdef ZC_AUTO_SEARCH_AND_BIND_LVL_CTRL_CLST
        if (dev->bind_step == BIND_STEP_ON_OFF_CLST)
        {
            dev->bind_step = BIND_STEP_LVL_CTRL_CLST;

            {
                zb_ret_t ret;

                if (dev->dev_type == SIMPLE_DEV_TYPE_LIGHT)
                {
                    ret = device_bind(buf_ref, dev, dev, &zc_ctx.devices[dev->assign_table[dev->assign_idx]]);
                }
                else
                {
                    ret = device_bind(buf_ref, dev, &zc_ctx.devices[dev->assign_table[dev->assign_idx]], dev);
                }

                if (ret == RET_IGNORE)
                {
                    dev->assign_idx = 0;
                    device_assign(buf_ref, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev));
                    buf_ref = 0;
                }
                else if (ret == RET_ERROR)
                {

                }
                else if (ret == RET_OK)
                {
                    buf_ref = 0;
                }
                else
                {
                    ZB_ASSERT(0);
                }
            }
        }
        else
#endif
        {
            dev->assign_idx = 0;
            device_assign(buf_ref, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev));
            buf_ref = 0;
        }
    }

    if (buf_ref)
    {
        zb_buf_free(buf);
    }

    TRACE_MSG(TRACE_APP1, "< device_bind_cb", (FMT__0));
}

static zb_ret_t device_bind(zb_uint8_t       buf_ref,
                            simple_device_t *dev,
                            simple_device_t *light,
                            simple_device_t *light_control)
{
    zb_bufid_t buf = buf_ref;
    zb_zdo_bind_req_param_t *bind_param = ZB_BUF_GET_PARAM(buf, zb_zdo_bind_req_param_t);

    TRACE_MSG(TRACE_APP1, "dev_idx: %hd light idx %hd light_control idx %hd",
              (FMT__H_H_H, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev),
               ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, light),
               ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, light_control)));

    switch (dev->bind_step)
    {
    case BIND_STEP_ON_OFF_CLST:
        bind_param->cluster_id = ZB_ZCL_CLUSTER_ID_ON_OFF;
        break;
#ifdef ZC_AUTO_SEARCH_AND_BIND_LVL_CTRL_CLST
    case BIND_STEP_LVL_CTRL_CLST:
        bind_param->cluster_id = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL;
        break;
#endif
    default:
        ZB_ASSERT(0);
        break;
    }

    /* light as destination */
    ZB_IEEE_ADDR_COPY(bind_param->dst_address.addr_long, light->ieee_addr);
    bind_param->dst_endp = light->match_ep;
    bind_param->dst_addr_mode = ZB_APS_ADDR_MODE_64_ENDP_PRESENT;
    TRACE_MSG(TRACE_APP1, "bind dst: ep %hd ieee_addr " TRACE_FORMAT_64, (FMT__H_A, bind_param->dst_endp, TRACE_ARG_64(bind_param->dst_address.addr_long)));

    /* light control as source */
    ZB_IEEE_ADDR_COPY(bind_param->src_address, light_control->ieee_addr);
    bind_param->src_endp = light_control->match_ep;
    bind_param->req_dst_addr = zb_address_short_by_ieee(light_control->ieee_addr);
    TRACE_MSG(TRACE_APP1, "bind src: ep %hd ieee_addr " TRACE_FORMAT_64, (FMT__H_A, bind_param->src_endp, TRACE_ARG_64(bind_param->src_address)));

    if (bind_param->req_dst_addr == ZB_UNKNOWN_SHORT_ADDR)
    {
        TRACE_MSG(TRACE_ERROR, "get short address by ieee failed", (FMT__0));
        dev->assign_table[dev->assign_idx] = 0xFF;
        dev->assign_idx = 0xFF;
        cleanup_assign_table(light_control);
        cleanup_dev(light_control);
        return RET_IGNORE;
    }

    dev->last_zdo_tsn = zb_zdo_bind_req(buf_ref, device_bind_cb);

    if (dev->last_zdo_tsn == 0xFF)
    {
        TRACE_MSG(TRACE_ERROR, "bind request allocation failed", (FMT__0));
        cleanup_dev(dev);
        return RET_ERROR;
    }

    return RET_OK;
}

static void device_assign(zb_uint8_t buf_ref, zb_uint16_t dev_idx)
{
    zb_uint8_t i;
    simple_device_t *dev = &zc_ctx.devices[(zb_uint8_t)dev_idx];

    TRACE_MSG(TRACE_APP1, "device assign idx: %hd ieee: " TRACE_FORMAT_64, (FMT__H_A, dev_idx, TRACE_ARG_64(dev->ieee_addr)));

    for (i = 0; i < LIGHT_ZC_MAX_DEVICES; ++i)
    {
        //skip self entry
        if (i == (zb_uint8_t)dev_idx)
        {
            continue;
        }

        if (zc_ctx.devices[i].dev_type == SIMPLE_DEV_TYPE_UNUSED ||
                zc_ctx.devices[i].dev_type == SIMPLE_DEV_TYPE_UNDEFINED)
        {
            continue;
        }

        //skip assign for the same type of device
        if (dev->dev_type == zc_ctx.devices[i].dev_type)
        {
            continue;
        }

        {
            zb_uint8_t j;
            zb_uint8_t free_slot = 0xFF;

            for (j = 0; j < (LIGHT_ZC_MAX_DEVICES - 1); ++j)
            {
                if (dev->assign_table[j] != 0xFF && dev->assign_table[j] == i)
                {
                    //already assigned
                    break;
                }

                if (free_slot == 0xFF && dev->assign_table[j] == 0xFF)
                {
                    free_slot = j;
                }
            }

            if (j < (LIGHT_ZC_MAX_DEVICES - 1))
            {
                //already assigned for this device
                continue;
            }

            if (free_slot == 0xFF)
            {
                ZB_ASSERT(0);
            }

            dev->assign_table[free_slot] = i;

            {
                zb_ret_t ret;

                dev->assign_idx = free_slot;

                if (dev->dev_type == SIMPLE_DEV_TYPE_LIGHT)
                {
                    ret = device_bind(buf_ref, dev, dev, &zc_ctx.devices[dev->assign_table[dev->assign_idx]]);
                }
                else
                {
                    ret = device_bind(buf_ref, dev, &zc_ctx.devices[dev->assign_table[dev->assign_idx]], dev);
                }

                if (ret == RET_IGNORE)
                {
                    continue;
                }
                else if (ret == RET_ERROR)
                {
                    break;
                }
                else if (ret == RET_OK)
                {
                    buf_ref = 0;
                }
                else
                {
                    ZB_ASSERT(0);
                }
            }
        }
    }

    if (buf_ref)
    {
        zb_buf_free(buf_ref);
    }
}

//Match device callback
static void device_match_cb(zb_uint8_t buf_ref);

//Start to match device
static void device_match(zb_uint8_t buf_ref, zb_uint16_t dev_idx)
{
    zb_bufid_t buf = buf_ref;
    zb_zdo_match_desc_param_t *req;
    simple_device_t *dev = &zc_ctx.devices[(zb_uint8_t)dev_idx];

    TRACE_MSG(TRACE_APP1, ">> device match %hd step %hd", (FMT__H_H, buf_ref, dev->match_step));

    /* match_desc_param already has allocated memory for one cluster identfier */
    req = zb_buf_initial_alloc(buf, sizeof(zb_zdo_match_desc_param_t) + (1) * sizeof(zb_uint16_t));

    req->nwk_addr = zb_address_short_by_ieee(dev->ieee_addr);

    if (req->nwk_addr != ZB_UNKNOWN_SHORT_ADDR)
    {
        req->addr_of_interest = req->nwk_addr;
        req->profile_id = ZB_AF_ZLL_PROFILE_ID;

        switch (dev->match_step)
        {
        case MATCH_STEP_ON_OFF_LVL_CTRL_SERVER:
#ifdef ZC_AUTO_SEARCH_AND_BIND_LVL_CTRL_CLST
            /* We are searching for On/Off and Level Control Server */
            req->num_in_clusters = 2;
#else
            /* We are searching for On/Off Server */
            req->num_in_clusters = 1;
#endif
            req->num_out_clusters = 0;
            break;
        case MATCH_STEP_ON_OFF_LVL_CTRL_CLIENT:
            req->num_in_clusters = 0;
#ifdef ZC_AUTO_SEARCH_AND_BIND_LVL_CTRL_CLST
            /* We are searching for On/Off and Level Control Server */
            req->num_out_clusters = 2;
#else
            /* We are searching for On/Off Server */
            req->num_out_clusters = 1;
#endif
            break;

        default:
            ZB_ASSERT(0);
            break;
        };

        req->cluster_list[0] = ZB_ZCL_CLUSTER_ID_ON_OFF;
#ifdef ZC_AUTO_SEARCH_AND_BIND_LVL_CTRL_CLST
        req->cluster_list[1] = ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL;
#endif

        dev->last_zdo_tsn = zb_zdo_match_desc_req(buf_ref, device_match_cb);

        if (dev->last_zdo_tsn == 0xFF)
        {
            TRACE_MSG(TRACE_ERROR, "zb_zdo_match_desc_req failed", (FMT__0));
            zb_buf_free(buf_ref);
        }
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "get short address by ieee failed", (FMT__0));
        zb_buf_free(buf_ref);
    }

    TRACE_MSG(TRACE_APP1, "<< device match", (FMT__0));
}

//Match device callback
static void device_match_cb(zb_uint8_t buf_ref)
{
    zb_bufid_t buf = buf_ref;
    zb_zdo_match_desc_resp_t *resp = (zb_zdo_match_desc_resp_t *)zb_buf_begin(buf);
    simple_device_t *dev;

    TRACE_MSG(TRACE_APP1, ">> device_match_cb buf_ref %hd, status %hd", (FMT__H_H, buf_ref, resp->status));

    dev = device_find_by_zdo_tsn(resp->tsn);

    if (dev == NULL)
    {
        zb_buf_free(buf);
        return;
    }

    dev->last_zdo_tsn = 0xFF;

    if (resp->status == ZB_ZDP_STATUS_SUCCESS)
    {
        switch (dev->match_step)
        {
        case MATCH_STEP_ON_OFF_LVL_CTRL_SERVER:
            if (resp->match_len > 0)
            {
                /* Match EP list follows right after response header */
                dev->match_ep = *(zb_uint8_t *)(resp + 1);
                dev->dev_type = SIMPLE_DEV_TYPE_LIGHT;

                TRACE_MSG(TRACE_APP1, "found light, dev_idx %hd, endpoint %hd", (FMT__H_H, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev), dev->match_ep));
                ZB_SCHEDULE_APP_CALLBACK2(device_assign, buf_ref, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev));
            }
            else
            {
                dev->match_step = MATCH_STEP_ON_OFF_LVL_CTRL_CLIENT;
                ZB_SCHEDULE_APP_CALLBACK2(device_match, buf_ref, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev));
            }
            break;
        case MATCH_STEP_ON_OFF_LVL_CTRL_CLIENT:
            if (resp->match_len > 0)
            {
                /* Match EP list follows right after response header */
                dev->match_ep = *(zb_uint8_t *)(resp + 1);
                dev->dev_type = SIMPLE_DEV_TYPE_LIGHT_CONTROL;

                TRACE_MSG(TRACE_APP1, "found light_control, dev_idx %hd, endpoint %hd", (FMT__H_H, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev), dev->match_ep));
                ZB_SCHEDULE_APP_CALLBACK2(device_assign, buf_ref, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev));
            }
            else
            {
                TRACE_MSG(TRACE_APP1, "no match found for dev_idx: %hd", (FMT__H, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev)));
                cleanup_dev(dev);
                zb_buf_free(buf);
            }
            break;
        default:
            ZB_ASSERT(0);
        };
    }
    else
    {
        cleanup_dev(dev);
        zb_buf_free(buf);
    }
}

static void device_annce_handler(zb_uint8_t buf_ref)
{
    zb_zdo_app_signal_hdr_t *sg_p = NULL;
    zb_zdo_signal_device_annce_params_t *dev_annce_params;
    simple_device_t *dev = NULL;

    zb_get_app_signal(buf_ref, &sg_p);
    dev_annce_params = ZB_ZDO_SIGNAL_GET_PARAMS(sg_p, zb_zdo_signal_device_annce_params_t);

    TRACE_MSG(TRACE_APP1, "dev_annce short_addr 0x%x", (FMT__D, dev_annce_params->device_short_addr));

    /* try to found slot for device */
    {
        /* [address_ieee_by_short] */
        zb_uint8_t i;
        zb_ieee_addr_t ieee_addr;

        if (zb_address_ieee_by_short(dev_annce_params->device_short_addr, ieee_addr) != RET_OK)
        {
            TRACE_MSG(TRACE_ERROR, "get ieee address by short failed", (FMT__0));
            zb_buf_free(buf_ref);
            return;
        }
        /* [address_ieee_by_short] */

        for (i = 0; i < LIGHT_ZC_MAX_DEVICES; ++i)
        {
            if (zc_ctx.devices[i].dev_type == SIMPLE_DEV_TYPE_UNUSED ||
                    (zc_ctx.devices[i].dev_type != SIMPLE_DEV_TYPE_UNUSED &&
                     ZB_IEEE_ADDR_CMP(ieee_addr, zc_ctx.devices[i].ieee_addr)))
            {
                dev = &zc_ctx.devices[i];

                if (zc_ctx.devices[i].dev_type == SIMPLE_DEV_TYPE_UNUSED)
                {
                    ZB_IEEE_ADDR_COPY(dev->ieee_addr, ieee_addr);

                    TRACE_MSG(TRACE_APP1, "device ieee: " TRACE_FORMAT_64, (FMT__A, TRACE_ARG_64(dev->ieee_addr)));
                    zc_ctx.devices[i].dev_type = SIMPLE_DEV_TYPE_UNDEFINED;
                }
                break;
            }
        }
    }

    if (dev == NULL)
    {
        TRACE_MSG(TRACE_ERROR, "no free slot for new device", (FMT__0));
        zb_buf_free(buf_ref);
        return;
    }

    TRACE_MSG(TRACE_APP1, "found slot %hd for device, device type %hd", (FMT__H_H, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev), dev->dev_type));

    ZB_SCHEDULE_APP_CALLBACK2(device_match, buf_ref, ZB_ARRAY_IDX_BY_ELEM(zc_ctx.devices, dev));
}

#endif  /* ZC_AUTO_SEARCH_AND_BIND */

void zboss_signal_handler(zb_uint8_t param)
{
    zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, NULL);

    if (ZB_GET_APP_SIGNAL_STATUS(param) == 0)
    {
        switch (sig)
        {
        case ZB_ZDO_SIGNAL_DEFAULT_START:
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            TRACE_MSG(TRACE_APP1, "Device STARTED OK", (FMT__0));
            /* Turn off link key exchange if legacy device support (<ZB3.0) is neeeded */
            zb_bdb_set_legacy_device_support(1);
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;
#ifdef ZC_AUTO_SEARCH_AND_BIND
        case ZB_ZDO_SIGNAL_DEVICE_ANNCE:
            TRACE_MSG(TRACE_APP1, "Device announcement signal", (FMT__0));
            ZB_SCHEDULE_APP_ALARM(device_annce_handler, param, ZB_TIME_ONE_SECOND * 5);
            param = 0;
            break;
#endif  /* ZC_AUTO_SEARCH_AND_BIND */
        default:
            TRACE_MSG(TRACE_APP1, "Unknown signal 0x%hx", (FMT__H, sig));
        }
    }
    else if (sig == ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY)
    {
        TRACE_MSG(TRACE_APP1, "Production config is not present or invalid", (FMT__0));
    }
    else
    {
        TRACE_MSG(TRACE_ERROR, "Device started FAILED status %d", (FMT__D, ZB_GET_APP_SIGNAL_STATUS(param)));
    }

    if (param)
    {
        zb_buf_free(param);
    }
}
