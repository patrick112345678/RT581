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
/*  PURPOSE: SERVER: Calendar Cluster
*/

#define ZB_TRACE_FILE_ID 3997

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_CALENDAR) || defined DOXYGEN

#include "zcl/zb_zcl_calendar.h"

/******************************************************************************/
/* Common definitions */

zb_uint8_t gs_calendar_server_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_CALENDAR_SERVER_ROLE_RECEIVED_CMD_LIST
};

zb_uint8_t gs_calendar_server_generated_commands[] =
{
    ZB_ZCL_CLUSTER_ID_CALENDAR_SERVER_ROLE_GENERATED_CMD_LIST
};

zb_discover_cmd_list_t gs_calendar_server_cmd_list =
{
    sizeof(gs_calendar_server_received_commands), gs_calendar_server_received_commands,
    sizeof(gs_calendar_server_generated_commands), gs_calendar_server_generated_commands
};

/* FIXME: Some commands are sent via follow macros. But we usually use ZB_APS_ADDR_MODE_64_ENDP_PRESENT address mode*/
/** Helper for sending cluster commands
 * @param _fn - sending cluster command function
 * @param _param - user parameter
 * @param _cmd_info - command info
 * @param _payload - sending payload. Corresoponds to sending command.
 * @param _cb - Callback which should be called when the ZCL stack receives
 * APS ack.
 */
#define ZB_ZCL_CALENDAR_SEND_CMD_HELPER(_fn, _param, _cmd_info, _payload, _cb)  \
  _fn((_param), \
     (zb_addr_u *) &ZB_ZCL_PARSED_HDR_SHORT_DATA(_cmd_info).source.u.short_addr, \
     ZB_APS_ADDR_MODE_16_ENDP_PRESENT, \
     ZB_ZCL_PARSED_HDR_SHORT_DATA(_cmd_info).src_endpoint, \
     ZB_ZCL_PARSED_HDR_SHORT_DATA(_cmd_info).dst_endpoint, \
     (_payload), (_cb) \
  )

zb_bool_t zb_zcl_process_s_calendar_specific_commands(zb_uint8_t param);

static zb_ret_t check_value_calendar(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value);


void zb_zcl_calendar_init_server()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_CALENDAR,
                                ZB_ZCL_CLUSTER_SERVER_ROLE,
                                (zb_zcl_cluster_check_value_t)check_value_calendar,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                zb_zcl_process_s_calendar_specific_commands);
}


static zb_ret_t check_value_calendar(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value)
{
    ZVUNUSED(attr_id);
    ZVUNUSED(value);
    ZVUNUSED(endpoint);

    /* All values for mandatory attributes are allowed, extra check for
     * optional attributes is needed */

    return RET_OK;
}


/* TODO: may be add "_" to the end of zb_zcl_calendar_ and start of _put_payload */
/* #define PUT_PL_FUN(type) zb_zcl_cal ## type ## put_payload */
#define PUT_PL_FUN(type) type ## _put_payload
#define PUT_PL_START(type, pl_var) \
static zb_uint8_t *PUT_PL_FUN(type)(zb_uint8_t *data, const void *pl_arg) \
{ \
  const type *pl_var = pl_arg; \
  ZB_ASSERT(pl_arg); \
  ZB_ASSERT(data);

#define PUT_PL_END() return data; }

PUT_PL_START(zb_zcl_calendar_publish_calendar_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_event_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->start_time);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->calendar_type);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->calendar_time_reference);
TRACE_MSG(TRACE_ZCL1, "calendar_name_size %hd", (FMT__H, pl->calendar_name[0]));
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->calendar_name[0]);
ZB_ZCL_PACKET_PUT_DATA_N(data, pl->calendar_name + 1, pl->calendar_name[0]);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->number_of_seasons);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->number_of_week_profiles);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->number_of_day_profiles);
PUT_PL_END()

PUT_PL_START(zb_zcl_calendar_publish_week_profile_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_event_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->week_id);

ZB_ZCL_PACKET_PUT_DATA8 (data, pl->day_id_ref_monday);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->day_id_ref_tuesday);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->day_id_ref_wednesday);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->day_id_ref_thursday);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->day_id_ref_friday);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->day_id_ref_saturday);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->day_id_ref_sunday);
PUT_PL_END()

PUT_PL_START(zb_zcl_calendar_publish_special_days_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_event_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->start_time);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->calendar_type);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->total_number_of_special_days);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->command_index);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->total_number_of_commands);
ZB_ZCL_PACKET_PUT_DATA_N(data, pl->special_day_entry, sizeof(zb_zcl_calendar_special_day_entry_t)*pl->number_of_entries_in_this_command);
PUT_PL_END()

PUT_PL_START(zb_zcl_calendar_cancel_calendar_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
ZB_ZCL_PACKET_PUT_DATA8(data, pl->calendar_type);
PUT_PL_END()

PUT_PL_START(zb_zcl_calendar_publish_day_profile_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_event_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->day_id);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->total_number_of_schedule_entries);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->command_index);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->total_number_of_commands);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->calendar_type);
ZB_ZCL_PACKET_PUT_DATA_N(data, pl->day_schedule_entries, sizeof(zb_zcl_calendar_day_schedule_entries_t)*pl->number_of_entries_in_this_command);
PUT_PL_END()

PUT_PL_START(zb_zcl_calendar_publish_seasons_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_event_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->command_index);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->total_number_of_commands);
ZB_ZCL_PACKET_PUT_DATA_N(data, pl->season_entry, sizeof(zb_zcl_calendar_season_entry_payload_t)*pl->number_of_entries_in_this_command);
PUT_PL_END()

#undef PUT_PL_START
#undef PUT_PL_END

void zb_zcl_calendar_send_cmd_publish_calendar(zb_uint8_t param,
        const zb_addr_u *dst_addr, zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_publish_calendar_payload_t *pl,
        zb_callback_t cb
                                              )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_publish_calendar_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_CALENDAR,
                    cb);

}


void zb_zcl_calendar_send_cmd_publish_day_profile(zb_uint8_t param, const zb_addr_u *dst_addr,
        zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_publish_day_profile_payload_t *pl,
        zb_callback_t cb
                                                 )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_publish_day_profile_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_DAY_PROFILE,
                    cb);
}


void zb_zcl_calendar_send_cmd_publish_week_profile(zb_uint8_t param, const zb_addr_u *dst_addr,
        zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_publish_week_profile_payload_t *pl,
        zb_callback_t cb
                                                  )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_publish_week_profile_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_WEEK_PROFILE,
                    cb);
}


void zb_zcl_calendar_send_cmd_publish_seasons(zb_uint8_t param, const zb_addr_u *dst_addr,
        zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_publish_seasons_payload_t *pl,
        zb_callback_t cb
                                             )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_publish_seasons_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_SEASONS,
                    cb);
}

void zb_zcl_calendar_send_cmd_publish_special_days(zb_uint8_t param,
        const zb_addr_u *dst_addr, zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_publish_special_days_payload_t *pl,
        zb_callback_t cb
                                                  )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_publish_special_days_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_SPECIAL_DAYS,
                    cb);
}

void zb_zcl_calendar_send_cmd_cancel_calendar(zb_uint8_t param,
        const zb_addr_u *dst_addr, zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_cancel_calendar_payload_t *pl,
        zb_callback_t cb
                                             )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_CLI,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_cancel_calendar_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_SRV_CMD_CANCEL_CALENDAR,
                    cb);
}

static void zb_zcl_calendar_callback( zb_uint8_t param, zb_bool_t *processed )
{
    zb_ret_t pre_status = ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param);

    if (ZCL_CTX().device_cb)
    {
        (ZCL_CTX().device_cb)(param);

        *processed = (zb_bool_t) (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) != pre_status);

        if (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param) != RET_OK)
        {
            TRACE_MSG(TRACE_ZCL1, "ERROR during command processing: "
                      "User callback failed with err=%d.",
                      (FMT__D, ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param)));
        }
    }
    else
    {
        TRACE_MSG(TRACE_ZCL1, "User app callback isn't defined.", (FMT__0));
    }
}
/******************************************************************************/
/* Server definitions */

static ZB_INLINE const zb_uint8_t *zb_zcl_calendar_get_calendar_parse_payload(
    zb_zcl_calendar_get_calendar_payload_t *pl, zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);

    ZB_ASSERT(data);

    if (!ZB_ZCL_CALENDAR_GET_CALENDAR_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    ZB_ZCL_PACKET_GET_DATA32(&pl->earliest_start_time, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->min_issuer_event_id, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->number_of_calendars, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->calendar_type, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);

    if (!ZB_ZCL_CALENDAR_CALENDAR_TYPE_IS_VALID(pl->calendar_type))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid calendar type %hd", (FMT__H, pl->calendar_type));
        return NULL;
    }

    return data;
}

static const zb_uint8_t *zb_zcl_calendar_get_day_profiles_parse_payload(
    zb_zcl_calendar_get_day_profiles_payload_t *pl, zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_calendar_get_day_profiles_parse_payload", (FMT__0));

    if (!ZB_ZCL_CALENDAR_GET_DAY_PROFILES_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->start_day_id, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->number_of_days, data);

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_calendar_get_day_profiles_parse_payload", (FMT__0));

    return data;
}

static const zb_uint8_t *zb_zcl_calendar_get_week_profiles_parse_payload(
    zb_zcl_calendar_get_week_profiles_payload_t *pl, zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_calendar_get_week_profiles_parse_payload", (FMT__0));

    if (!ZB_ZCL_CALENDAR_GET_WEEK_PROFILES_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->start_week_id, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->number_of_weeks, data);

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_calendar_get_week_profiles_parse_payload", (FMT__0));

    return data;
}

static const zb_uint8_t *zb_zcl_calendar_get_seasons_parse_payload(
    zb_zcl_calendar_get_seasons_payload_t *pl, zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_calendar_get_seasons_parse_payload", (FMT__0));

    if (!ZB_ZCL_CALENDAR_GET_SEASONS_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_calendar_get_seasons_parse_payload", (FMT__0));

    return data;
}


static const zb_uint8_t *zb_zcl_calendar_get_special_days_parse_payload(
    zb_zcl_calendar_get_special_days_payload_t *pl, zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_calendar_get_special_days_parse_payload", (FMT__0));

    if (!ZB_ZCL_CALENDAR_GET_SPECIAL_DAYS_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    ZB_ZCL_PACKET_GET_DATA32(&pl->start_time, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->number_of_events, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->calendar_type, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_calendar_get_special_days_parse_payload", (FMT__0));

    return data;
}


static zb_bool_t zb_zcl_calendar_process_get_calendar(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_get_calendar_payload_t      pl_in = ZB_ZCL_CALENDAR_GET_CALENDAR_PL_INIT;
    zb_zcl_calendar_publish_calendar_payload_t  pl_out = ZB_ZCL_CALENDAR_PUBLISH_CALENDAR_PL_INIT;
    zb_bool_t                         processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_get_calendar", (FMT__0));

    if (!zb_zcl_calendar_get_calendar_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                      ZB_ZCL_CALENDAR_GET_CALENDAR_CB_ID, RET_NOT_FOUND, cmd_info, &pl_in, &pl_out);

    zb_zcl_calendar_callback(param, &processed);

    if (processed)
    {
        ZB_ZCL_CALENDAR_SEND_CMD_HELPER(zb_zcl_calendar_send_cmd_publish_calendar, param, cmd_info, &pl_out, NULL);
    }
    else
    {
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_NOT_FOUND);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_get_calendar", (FMT__0));
    return ZB_TRUE;
}


static zb_bool_t zb_zcl_calendar_process_get_day_profiles(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_get_day_profiles_payload_t pl_in = ZB_ZCL_CALENDAR_GET_DAY_PROFILES_PL_INIT;
    zb_zcl_calendar_publish_day_profile_payload_t pl_out = ZB_ZCL_CALENDAR_PUBLISH_DAY_PROFILE_PL_INIT;
    zb_bool_t processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_get_day_profiles", (FMT__0));

    if (!zb_zcl_calendar_get_day_profiles_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                      ZB_ZCL_CALENDAR_GET_DAY_PROFILES_CB_ID,
                                      RET_NOT_FOUND, cmd_info, &pl_in, &pl_out);

    zb_zcl_calendar_callback(param, &processed);

    if (processed)
    {
        ZB_ZCL_CALENDAR_SEND_CMD_HELPER(zb_zcl_calendar_send_cmd_publish_day_profile, param, cmd_info, &pl_out, NULL);
    }
    else
    {
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_NOT_FOUND);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_get_day_profiles", (FMT__0));
    return ZB_TRUE;
}

static zb_bool_t zb_zcl_calendar_process_get_week_profiles(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_get_week_profiles_payload_t pl_in = ZB_ZCL_CALENDAR_GET_WEEK_PROFILES_PL_INIT;
    zb_zcl_calendar_publish_week_profile_payload_t pl_out = ZB_ZCL_CALENDAR_PUBLISH_WEEK_PROFILE_PL_INIT;
    zb_bool_t processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_get_week_profiles", (FMT__0));

    if (!zb_zcl_calendar_get_week_profiles_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                      ZB_ZCL_CALENDAR_GET_WEEK_PROFILES_CB_ID,
                                      RET_NOT_FOUND, cmd_info, &pl_in, &pl_out);

    zb_zcl_calendar_callback(param, &processed);

    if (processed)
    {
        ZB_ZCL_CALENDAR_SEND_CMD_HELPER(zb_zcl_calendar_send_cmd_publish_week_profile, param, cmd_info, &pl_out, NULL);
    }
    else
    {
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_NOT_FOUND);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_get_week_profiles", (FMT__0));
    return ZB_TRUE;
}


static zb_bool_t zb_zcl_calendar_process_get_seasons(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_get_seasons_payload_t pl_in = ZB_ZCL_CALENDAR_GET_SEASONS_PL_INIT;
    zb_zcl_calendar_publish_seasons_payload_t pl_out = ZB_ZCL_CALENDAR_PUBLISH_SEASONS_PL_INIT;
    zb_bool_t processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_get_seasons", (FMT__0));

    if (!zb_zcl_calendar_get_seasons_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                      ZB_ZCL_CALENDAR_GET_SEASONS_CB_ID,
                                      RET_NOT_FOUND, cmd_info, &pl_in, &pl_out);

    zb_zcl_calendar_callback(param, &processed);

    if (processed)
    {
        ZB_ZCL_CALENDAR_SEND_CMD_HELPER(zb_zcl_calendar_send_cmd_publish_seasons, param, cmd_info, &pl_out, NULL);
    }
    else
    {
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_NOT_FOUND);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_get_seasons", (FMT__0));
    return ZB_TRUE;
}

static zb_bool_t zb_zcl_calendar_process_get_special_days(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_get_special_days_payload_t pl_in = ZB_ZCL_CALENDAR_GET_SPECIAL_DAYS_PL_INIT;
    zb_zcl_calendar_publish_special_days_payload_t pl_out = ZB_ZCL_CALENDAR_PUBLISH_SPECIAL_DAYS_PL_INIT;
    zb_bool_t processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_get_special_days", (FMT__0));

    if (!zb_zcl_calendar_get_special_days_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                      ZB_ZCL_CALENDAR_GET_SPECIAL_DAYS_CB_ID,
                                      RET_NOT_FOUND, cmd_info, &pl_in, &pl_out);

    zb_zcl_calendar_callback(param, &processed);

    if (processed)
    {
        ZB_ZCL_CALENDAR_SEND_CMD_HELPER(zb_zcl_calendar_send_cmd_publish_special_days, param, cmd_info, &pl_out, NULL);
    }
    else
    {
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_NOT_FOUND);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_get_special_days", (FMT__0));
    return ZB_TRUE;
}

static zb_bool_t zb_zcl_calendar_process_get_calendar_cancellation(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_cancel_calendar_payload_t pl_out = ZB_ZCL_CALENDAR_CANCEL_CALENDAR_PL_INIT;
    zb_bool_t processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_get_calendar_cancellation", (FMT__0));

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                      ZB_ZCL_CALENDAR_GET_CALENDAR_CANCELLATION_CB_ID,
                                      RET_NOT_FOUND, cmd_info, NULL, &pl_out);

    zb_zcl_calendar_callback(param, &processed);

    if (processed)
    {
        ZB_ZCL_CALENDAR_SEND_CMD_HELPER(zb_zcl_calendar_send_cmd_cancel_calendar, param, cmd_info, &pl_out, NULL);
    }
    else
    {
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_NOT_FOUND);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_get_calendar_cancellation", (FMT__0));
    return ZB_TRUE;
}


static zb_bool_t zb_zcl_process_cal_srv_cmd(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_bool_t processed = ZB_FALSE;

    switch ((zb_zcl_calendar_cli_cmd_t) cmd_info->cmd_id)
    {
    case ZB_ZCL_CALENDAR_CLI_CMD_GET_CALENDAR:
        processed = zb_zcl_calendar_process_get_calendar(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_CLI_CMD_GET_DAY_PROFILES:
        processed = zb_zcl_calendar_process_get_day_profiles(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_CLI_CMD_GET_WEEK_PROFILES:
        processed = zb_zcl_calendar_process_get_week_profiles(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_CLI_CMD_GET_SEASONS:
        processed = zb_zcl_calendar_process_get_seasons(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_CLI_CMD_GET_SPECIAL_DAYS:
        processed = zb_zcl_calendar_process_get_special_days(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_CLI_CMD_GET_CALENDAR_CANCELLATION:
        processed = zb_zcl_calendar_process_get_calendar_cancellation(param, cmd_info);
        break;
#ifdef WIP
    default:
        break;
#endif
        /* FIXME: add default case */
    }

    return processed;
}

/******************************************************************************/
/* Handler definition */
zb_bool_t zb_zcl_process_s_calendar_specific_commands(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t cmd_info;
    zb_bool_t           processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_process_s_calendar_specific_commands, "
              "param=%hd", (FMT__H, param));

    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_calendar_server_cmd_list;
        return ZB_TRUE;
    }

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
    /* ZB_ASSERT(cmd_info.profile_id == ZB_AF_SE_PROFILE_ID); */
    ZB_ASSERT(cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_CALENDAR);

    if (ZB_ZCL_FRAME_DIRECTION_TO_SRV == cmd_info.cmd_direction)
    {
        processed = zb_zcl_process_cal_srv_cmd(param, &cmd_info);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_process_s_calendar_specific_commands (cmd_id=%hd, ret=%hd)",
              (FMT__H_H, cmd_info.cmd_id, processed));

    return processed;
}

#endif /* ZB_ZCL_SUPPORT_CLUSTER_CALENDAR || defined DOXYGEN */
