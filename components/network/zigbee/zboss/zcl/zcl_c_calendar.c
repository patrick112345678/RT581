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
/*  PURPOSE: CLIENT: Calendar Cluster
*/

#define ZB_TRACE_FILE_ID 3996

#include "zb_common.h"

#if defined (ZB_ZCL_SUPPORT_CLUSTER_CALENDAR) || defined DOXYGEN

#include "zcl/zb_zcl_calendar.h"

/******************************************************************************/
/* Common definitions */

zb_uint8_t gs_calendar_client_received_commands[] =
{
    ZB_ZCL_CLUSTER_ID_CALENDAR_CLIENT_ROLE_RECEIVED_CMD_LIST
};

zb_uint8_t gs_calendar_client_generated_commands[] =
{
    ZB_ZCL_CLUSTER_ID_CALENDAR_CLIENT_ROLE_GENERATED_CMD_LIST
};

zb_discover_cmd_list_t gs_calendar_client_cmd_list =
{
    sizeof(gs_calendar_client_received_commands), gs_calendar_client_received_commands,
    sizeof(gs_calendar_client_generated_commands), gs_calendar_client_generated_commands
};

zb_bool_t zb_zcl_process_c_calendar_specific_commands(zb_uint8_t param);

static zb_ret_t check_value_calendar(zb_uint16_t attr_id, zb_uint8_t endpoint, zb_uint8_t *value);

void zb_zcl_calendar_init_client()
{
    zb_zcl_add_cluster_handlers(ZB_ZCL_CLUSTER_ID_CALENDAR,
                                ZB_ZCL_CLUSTER_CLIENT_ROLE,
                                (zb_zcl_cluster_check_value_t)check_value_calendar,
                                (zb_zcl_cluster_write_attr_hook_t)NULL,
                                zb_zcl_process_c_calendar_specific_commands);
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

PUT_PL_START(zb_zcl_calendar_get_calendar_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->earliest_start_time);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->min_issuer_event_id);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->number_of_calendars);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->calendar_type);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
PUT_PL_END()

PUT_PL_START(zb_zcl_calendar_get_day_profiles_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->start_day_id);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->number_of_days);
PUT_PL_END()

PUT_PL_START(zb_zcl_calendar_get_week_profiles_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->start_week_id);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->number_of_weeks);
PUT_PL_END()

PUT_PL_START(zb_zcl_calendar_get_seasons_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
PUT_PL_END()

PUT_PL_START(zb_zcl_calendar_get_special_days_payload_t, pl)
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->start_time);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->number_of_events);
ZB_ZCL_PACKET_PUT_DATA8 (data, pl->calendar_type);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->provider_id);
ZB_ZCL_PACKET_PUT_DATA32(data, &pl->issuer_calendar_id);
PUT_PL_END()

#undef PUT_PL_START
#undef PUT_PL_END

void zb_zcl_calendar_send_cmd_get_calendar(zb_uint8_t param,
        const zb_addr_u *dst_addr, zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_get_calendar_payload_t *pl,
        zb_callback_t cb
                                          )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_get_calendar_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_CLI_CMD_GET_CALENDAR,
                    cb);
}


void zb_zcl_calendar_send_cmd_get_day_profiles(zb_uint8_t param,
        const zb_addr_u *dst_addr, zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_get_day_profiles_payload_t *pl,
        zb_callback_t cb
                                              )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_get_day_profiles_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_CLI_CMD_GET_DAY_PROFILES,
                    cb);
}

void zb_zcl_calendar_send_cmd_get_week_profiles(zb_uint8_t param,
        const zb_addr_u *dst_addr, zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_get_week_profiles_payload_t *pl,
        zb_callback_t cb
                                               )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_get_week_profiles_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_CLI_CMD_GET_WEEK_PROFILES,
                    cb);
}

void zb_zcl_calendar_send_cmd_get_seasons(zb_uint8_t param,
        const zb_addr_u *dst_addr, zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_get_seasons_payload_t *pl,
        zb_callback_t cb
                                         )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_get_seasons_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_CLI_CMD_GET_SEASONS,
                    cb);
}


void zb_zcl_calendar_send_cmd_get_special_days(zb_uint8_t param,
        const zb_addr_u *dst_addr, zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        const zb_zcl_calendar_get_special_days_payload_t *pl,
        zb_callback_t cb
                                              )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                    src_ep, pl, sizeof(*pl),
                    PUT_PL_FUN(zb_zcl_calendar_get_special_days_payload_t),
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_CLI_CMD_GET_SPECIAL_DAYS,
                    cb);
}


void zb_zcl_calendar_send_cmd_get_calendar_cancellation(zb_uint8_t param,
        const zb_addr_u *dst_addr, zb_aps_addr_mode_t dst_addr_mode,
        zb_uint8_t dst_ep, zb_uint8_t src_ep,
        zb_callback_t cb
                                                       )
{
    zb_zcl_send_cmd(param, dst_addr, dst_addr_mode, dst_ep,
                    ZB_ZCL_FRAME_DIRECTION_TO_SRV,
                    src_ep, NULL, 0, NULL,
                    ZB_ZCL_CLUSTER_ID_CALENDAR,
                    ZB_ZCL_ENABLE_DEFAULT_RESPONSE,
                    ZB_ZCL_CALENDAR_CLI_CMD_GET_CALENDAR_CANCELLATION,
                    cb);
}


/******************************************************************************/
/* Client definitions */

static const zb_uint8_t *zb_zcl_calendar_publish_calendar_parse_payload(
    zb_zcl_calendar_publish_calendar_payload_t *pl, zb_uint8_t param)
{
    zb_uint8_t                       *data = zb_buf_begin(param);
    zb_uint8_t                        data_size = zb_buf_len(param);

    if (!ZB_ZCL_CALENDAR_PUBLISH_CALENDAR_PL_SIZE_IS_VALID(((zb_zcl_calendar_publish_calendar_payload_t *)data), data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd, expected: %hd", (FMT__H_H,
                  data_size, ZB_ZCL_CALENDAR_PUBLISH_CALENDAR_PL_EXPECTED_SIZE(pl)));
        return NULL;
    }

    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_event_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->start_time, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->calendar_type, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->calendar_time_reference, data);

    ZB_ZCL_PACKET_GET_STATIC_STRING(pl->calendar_name, data);

    if (!data)
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid string len = %hd",
                  (FMT__H, ZB_ZCL_GET_STRING_LENGTH(pl->calendar_name)));
        return NULL;
    }

    ZB_ZCL_PACKET_GET_DATA8 (&pl->number_of_seasons, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->number_of_week_profiles, data);
    ZB_ZCL_PACKET_GET_DATA8 (&pl->number_of_day_profiles, data);

    return data;
}


static const zb_uint8_t *zb_zcl_calendar_publish_day_profile_parse_payload(
    zb_zcl_calendar_publish_day_profile_payload_t *pl,
    zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);
    zb_uint8_t data_len = 0;

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_calendar_publish_day_profile_parse_payload", (FMT__0));

    if (!ZB_ZCL_CALENDAR_PUBLISH_DAY_PROFILE_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    data_len = (data_size - sizeof(zb_zcl_calendar_publish_day_profile_payload_t) + sizeof(void *) + sizeof(zb_uint8_t)) /
               sizeof(zb_zcl_calendar_day_schedule_entries_t);

    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_event_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->day_id, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->total_number_of_schedule_entries, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->command_index, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->total_number_of_commands, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->calendar_type, data);
    pl->day_schedule_entries = (zb_zcl_calendar_day_schedule_entries_t *)data;
    pl->number_of_entries_in_this_command = data_len;

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_calendar_publish_day_profile_parse_payload", (FMT__0));

    return data;
}

static const zb_uint8_t *zb_zcl_calendar_publish_week_profile_parse_payload(
    zb_zcl_calendar_publish_week_profile_payload_t *pl,
    zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_calendar_publish_week_profile_parse_payload", (FMT__0));

    if (!ZB_ZCL_CALENDAR_PUBLISH_WEEK_PROFILE_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_event_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->week_id, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->day_id_ref_monday, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->day_id_ref_tuesday, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->day_id_ref_wednesday, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->day_id_ref_thursday, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->day_id_ref_friday, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->day_id_ref_saturday, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->day_id_ref_sunday, data);

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_calendar_publish_week_profile_parse_payload", (FMT__0));

    return data;
}

static const zb_uint8_t *zb_zcl_calendar_publish_seasons_parse_payload(
    zb_zcl_calendar_publish_seasons_payload_t *pl,
    zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);
    zb_uint8_t data_len = 0;

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_calendar_publish_seasons_parse_payload", (FMT__0));

    if (!ZB_ZCL_CALENDAR_PUBLISH_SEASONS_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    data_len = (data_size - sizeof(zb_zcl_calendar_publish_seasons_payload_t) + sizeof(void *) + sizeof(zb_uint8_t)) /
               sizeof(zb_zcl_calendar_season_entry_payload_t);

    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_event_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->command_index, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->total_number_of_commands, data);
    /* Note: zb_zcl_calendar_season_entry_payload_t is packed struct, so
     * any pointer cast is safe. Shut gcc warning. */
    pl->season_entry = (zb_zcl_calendar_season_entry_payload_t *)(void *)data;
    pl->number_of_entries_in_this_command = data_len;

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_calendar_publish_seasons_parse_payload", (FMT__0));

    return data;
}


static const zb_uint8_t *zb_zcl_calendar_publish_special_days_parse_payload(
    zb_zcl_calendar_publish_special_days_payload_t *pl,
    zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);
    zb_uint8_t data_len = 0;

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_calendar_publish_special_days_parse_payload", (FMT__0));

    if (!ZB_ZCL_CALENDAR_PUBLISH_SPECIAL_DAYS_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    data_len = (data_size - sizeof(zb_zcl_calendar_publish_special_days_payload_t) + sizeof(void *) + sizeof(zb_uint8_t)) /
               sizeof(zb_zcl_calendar_special_day_entry_t);

    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_event_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->start_time, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->calendar_type, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->total_number_of_special_days, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->command_index, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->total_number_of_commands, data);
    pl->special_day_entry = (zb_zcl_calendar_special_day_entry_t *)data;
    pl->number_of_entries_in_this_command = data_len;

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_calendar_publish_special_days_parse_payload", (FMT__0));

    return data;
}


static const zb_uint8_t *zb_zcl_calendar_cancel_calendar_parse_payload(
    zb_zcl_calendar_cancel_calendar_payload_t *pl,
    zb_uint8_t param)
{
    zb_uint8_t *data = zb_buf_begin(param);
    zb_uint8_t data_size = zb_buf_len(param);

    TRACE_MSG(TRACE_ZCL1, ">>zb_zcl_calendar_cancel_calendar_parse_payload", (FMT__0));

    if (!ZB_ZCL_CALENDAR_CANCEL_CALENDAR_PL_SIZE_IS_VALID(data_size))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload size %hd", (FMT__H, data_size));
        return NULL;
    }

    ZB_ZCL_PACKET_GET_DATA32(&pl->provider_id, data);
    ZB_ZCL_PACKET_GET_DATA32(&pl->issuer_calendar_id, data);
    ZB_ZCL_PACKET_GET_DATA8(&pl->calendar_type, data);

    TRACE_MSG(TRACE_ZCL1, "<<zb_zcl_calendar_cancel_calendar_parse_payload", (FMT__0));

    return data;
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


static void zb_zcl_calendar_callback_in_only( zb_uint8_t param, const zb_zcl_parsed_hdr_t *cmd_info )
{
    zb_bool_t processed = ZB_FALSE;
    zb_uint8_t status;

    zb_zcl_calendar_callback(param, &processed);

    switch (ZB_ZCL_DEVICE_CMD_PARAM_STATUS(param))
    {
    case RET_OK:
        status = ZB_ZCL_STATUS_SUCCESS;
        break;
    case RET_NOT_FOUND:
        status = ZB_ZCL_STATUS_NOT_FOUND;
        break;
    case RET_NO_MEMORY:
        status = ZB_ZCL_STATUS_INSUFF_SPACE;
        break;
    case RET_ERROR:
    /* FALLTHRU */
    default:
        status = ZB_ZCL_STATUS_FAIL;
        break;
    }

    zb_zcl_send_default_handler(param, cmd_info, (zb_zcl_status_t)status);
}


static zb_bool_t zb_zcl_calendar_process_publish_calendar(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_publish_calendar_payload_t  pl_in = ZB_ZCL_CALENDAR_PUBLISH_CALENDAR_PL_INIT;

    TRACE_MSG(TRACE_ZCL3, ">> zb_zcl_calendar_process_publish_calendar", (FMT__0));

    if (!zb_zcl_calendar_publish_calendar_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param,
                                      ZB_ZCL_CALENDAR_PUBLISH_CALENDAR_CB_ID, RET_ERROR, cmd_info, &pl_in, NULL);

    zb_zcl_calendar_callback_in_only(param, cmd_info);

    TRACE_MSG(TRACE_ZCL3, "<< zb_zcl_calendar_process_publish_calendar", (FMT__0));

    return ZB_TRUE;
}

static zb_bool_t zb_zcl_calendar_process_publish_day_profile(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_publish_day_profile_payload_t pl_in = ZB_ZCL_CALENDAR_PUBLISH_DAY_PROFILE_PL_INIT;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_publish_day_profile", (FMT__0));

    if (!zb_zcl_calendar_publish_day_profile_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid publish day profile payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param, ZB_ZCL_CALENDAR_PUBLISH_DAY_PROFILE_CB_ID,
                                      RET_ERROR, cmd_info, &pl_in, NULL);

    zb_zcl_calendar_callback_in_only(param, cmd_info);

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_publish_day_profile", (FMT__0));
    return ZB_TRUE;
}

static zb_bool_t zb_zcl_calendar_process_publish_week_profile(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_publish_week_profile_payload_t pl_in = ZB_ZCL_CALENDAR_PUBLISH_WEEK_PROFILE_PL_INIT;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_publish_week_profile", (FMT__0));

    if (!zb_zcl_calendar_publish_week_profile_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid publish week profile payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param, ZB_ZCL_CALENDAR_PUBLISH_WEEK_PROFILE_CB_ID,
                                      RET_ERROR, cmd_info, &pl_in, NULL);

    zb_zcl_calendar_callback_in_only(param, cmd_info);

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_publish_week_profile", (FMT__0));
    return ZB_TRUE;
}

static zb_bool_t zb_zcl_calendar_process_publish_seasons(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_publish_seasons_payload_t pl_in = ZB_ZCL_CALENDAR_PUBLISH_SEASONS_PL_INIT;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_publish_seasons", (FMT__0));

    if (!zb_zcl_calendar_publish_seasons_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid publish seasons payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param, ZB_ZCL_CALENDAR_PUBLISH_SEASONS_CB_ID,
                                      RET_ERROR, cmd_info, &pl_in, NULL);

    zb_zcl_calendar_callback_in_only(param, cmd_info);

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_publish_seasons", (FMT__0));
    return ZB_TRUE;
}

static zb_bool_t zb_zcl_calendar_process_publish_special_days(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_zcl_calendar_publish_special_days_payload_t pl_in = ZB_ZCL_CALENDAR_PUBLISH_SPECIAL_DAYS_PL_INIT;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_publish_special_days", (FMT__0));

    if (!zb_zcl_calendar_publish_special_days_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid publish special days payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param, ZB_ZCL_CALENDAR_PUBLISH_SPECIAL_DAYS_CB_ID,
                                      RET_ERROR, cmd_info, &pl_in, NULL);

    zb_zcl_calendar_callback_in_only(param, cmd_info);

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_publish_special_days", (FMT__0));
    return ZB_TRUE;
}


static zb_bool_t zb_zcl_calendar_process_cancel_calendar(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{

    zb_zcl_calendar_cancel_calendar_payload_t pl_in = ZB_ZCL_CALENDAR_CANCEL_CALENDAR_PL_INIT;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_calendar_process_cancel_calendar", (FMT__0));

    if (!zb_zcl_calendar_cancel_calendar_parse_payload(&pl_in, param))
    {
        TRACE_MSG(TRACE_ZCL1, "Invalid cancel calendar payload", (FMT__0));
        zb_zcl_send_default_handler(param, cmd_info, ZB_ZCL_STATUS_INVALID_FIELD);
        return ZB_TRUE;
    }

    ZB_ZCL_DEVICE_CMD_PARAM_INIT_WITH(param, ZB_ZCL_CALENDAR_CANCEL_CALENDAR_CB_ID,
                                      RET_ERROR, cmd_info, &pl_in, NULL);

    zb_zcl_calendar_callback_in_only(param, cmd_info);

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_calendar_process_publish_week_profile", (FMT__0));

    return ZB_TRUE;
}


static zb_bool_t zb_zcl_process_cal_cli_cmd(zb_uint8_t param,
        const zb_zcl_parsed_hdr_t *cmd_info)
{
    zb_bool_t processed = ZB_FALSE;

    switch ( (zb_zcl_calendar_srv_cmd_t) cmd_info->cmd_id)
    {
    case ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_CALENDAR:
        processed = zb_zcl_calendar_process_publish_calendar(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_DAY_PROFILE:
        processed = zb_zcl_calendar_process_publish_day_profile(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_WEEK_PROFILE:
        processed = zb_zcl_calendar_process_publish_week_profile(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_SEASONS:
        processed = zb_zcl_calendar_process_publish_seasons(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_SRV_CMD_PUBLISH_SPECIAL_DAYS:
        processed = zb_zcl_calendar_process_publish_special_days(param, cmd_info);
        break;
    case ZB_ZCL_CALENDAR_SRV_CMD_CANCEL_CALENDAR:
        processed = zb_zcl_calendar_process_cancel_calendar(param, cmd_info);
        break;
        /* FIXME: add default case */
#ifdef WIP
    default:
        break;
#endif
    }

    return processed;
}


/******************************************************************************/
/* Handler definition */
zb_bool_t zb_zcl_process_c_calendar_specific_commands(zb_uint8_t param)
{
    zb_zcl_parsed_hdr_t cmd_info;
    zb_bool_t           processed = ZB_FALSE;

    TRACE_MSG(TRACE_ZCL1, ">> zb_zcl_process_c_calendar_specific_commands, "
              "param=%hd", (FMT__H, param ));

    if ( ZB_ZCL_GENERAL_GET_CMD_LISTS_PARAM == param )
    {
        ZCL_CTX().zb_zcl_cluster_cmd_list = &gs_calendar_client_cmd_list;
        return ZB_TRUE;
    }

    ZB_ZCL_COPY_PARSED_HEADER(param, &cmd_info);
    /* ZB_ASSERT(cmd_info.profile_id == ZB_AF_SE_PROFILE_ID); */
    ZB_ASSERT(cmd_info.cluster_id == ZB_ZCL_CLUSTER_ID_CALENDAR);

    if (ZB_ZCL_FRAME_DIRECTION_TO_CLI == cmd_info.cmd_direction)
    {
        processed = zb_zcl_process_cal_cli_cmd(param, &cmd_info);
    }

    TRACE_MSG(TRACE_ZCL1, "<< zb_zcl_process_c_calendar_specific_commands (cmd_id=%hd, ret=%hd)",
              (FMT__H_H, cmd_info.cmd_id, processed));

    return processed;
}

#endif /* ZB_ZCL_SUPPORT_CLUSTER_CALENDAR || defined DOXYGEN */
