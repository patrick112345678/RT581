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
/* PURPOSE: trace and traffic dump for platform with filesystem and printf() routine
*/


#define ZB_TRACE_FILE_ID 2145
#include "zb_common.h"

#if (defined ZB_TRACE_TO_FILE || defined ZB_TRACE_TO_SYSLOG)

#include "zb_trace_common.h"

#if defined (ZB_TRACE_TO_SYSLOG)

#ifdef __ANDROID__
#include <android/log.h>
#elif defined UNIX
#include <syslog.h>
#endif

#endif

#ifdef ZB_PLATFORM_LINUX
#include <time.h>
#endif

/** @cond DOXYGEN_DEBUG_SECTION */
/** @addtogroup ZB_TRACE */
/** @{ */

#define MAX_SUBSYSTEM_NAME 32

static zb_bool_t is_error_msg(zb_uint_t mask, zb_uint_t level)
{
  return mask == TRACE_SUBSYSTEM_INFO && level == 0;
}

static const zb_char_t *subsystem_name_get(zb_uint_t mask, zb_uint_t level)
{
  zb_uindex_t i;

  const zb_char_t *components[] =
  {
   "COMMON",              /* 0x0001 */
   "MEM",                 /* 0x0002 */
   "MAC",                 /* 0x0004 */
   "NWK",
   "APS",
   "ZSE/CLOUD",
   "ZDO",
   "SECUR",
   "ZCL",
   "ZLL/JSON",
   "MAC_API/SSL",
   "APP",
   "LWIP/UART/TPORT",
   "MACLL/ALIEN/SPECIAL",
   "ZGP",
   "USB/SPI/HTTP",        /* 0x8000 */
  };

  for (i = 0; i < 16; i++)
  {
    if ((1u << i) == mask)
    {
      return components[i];
    }
  }

  /* No one of default names. Then it's ERROR of INFO */
  if (is_error_msg(mask, level))
  {
    return "ERROR";
  }
  else
  {
    return "INFO";
  }
}

static void fill_subsystem_name(zb_char_t *subsystem_name, zb_uint_t mask, zb_uint_t level)
{
  const zb_char_t *name = subsystem_name_get(mask, level);

  /* Print ERROR messages without level */
  if (is_error_msg(mask, level))
  {
    snprintf(subsystem_name, MAX_SUBSYSTEM_NAME, "%s", name);
  }
  else
  {
    snprintf(subsystem_name, MAX_SUBSYSTEM_NAME, "%s%d", name, level);
  }
}

/**
 * Output trace message.
 *
 * @param format - printf-like format string
 * @param file_name - source file name
 * @param line_number - source file line
 * @param args_size - number of added parameters
 */
void zb_trace_msg_txt_file(
  zb_uint_t mask,
  zb_uint_t level,
  const zb_char_t *format,
  const zb_char_t *file_name,
#if defined ZB_BINARY_AND_TEXT_TRACE_MODE
  zb_uint16_t file_id,
#endif
  zb_int_t line_number,
  zb_int_t args_size, ...)
{
#if defined ZB_BINARY_AND_TEXT_TRACE_MODE
  ZVUNUSED(file_id);
#endif
  if (!zb_trace_check(level, mask))
  {
    return;
  }
  /* If ZB_TRACE_LEVEL not defined, output nothing */
#ifdef ZB_TRACE_LEVEL
  {
    va_list   arglist;
    zb_uint_t sec, msec;
#ifdef ZB_TRACE_TO_SYSLOG
    char buf[4096];
    int printed;
#endif

#ifndef ZB_TRACE_TO_SYSLOG
    if (!s_trace_file)
    {
      return;
    }
#endif

    (void)args_size;
    zb_osif_trace_lock();
    zb_osif_trace_get_time(&sec, &msec);

#ifdef ZB_TRACE_TO_SYSLOG

    printed = snprintf(buf, sizeof(buf), "%d %x %d/%d/%03d.%03d %s:%d\t",
                           zb_trace_get_counter(), osif_get_thread_id(), ZB_TIMER_GET(),
                           ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_TIMER_GET()),
                           sec, msec,
                           file_name, line_number);
    zb_trace_inc_counter();

    if(printed > 0)
    {
      va_start(arglist, args_size);
      vsnprintf(&buf[printed], sizeof(buf) - printed, format, arglist);
      va_end(arglist);
#ifdef __ANDROID__
      __android_log_print(level == 0 ? ANDROID_LOG_ERROR : ANDROID_LOG_DEBUG,
                           g_tag, "%s", buf);
#else
      vsyslog(LOG_LOCAL7, "%s", buf);
#endif
    }

#else /* ZB_TRACE_TO_SYSLOG */

    {
      zb_char_t subsystem_name[MAX_SUBSYSTEM_NAME];
      zb_char_t ltime[200];

      fill_subsystem_name(subsystem_name, mask, level);
      ZB_BZERO(ltime, sizeof(ltime));
#ifdef ZB_PLATFORM_LINUX
      {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        snprintf(ltime, sizeof(ltime), "%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
      }
#endif
#ifndef ZB_THREADS
      zb_osif_trace_printf(s_trace_file, "%d %s %d/%d/%03d.%03d %s:%d\t[%s] ",
                          zb_trace_get_counter(), ltime, ZB_TIMER_GET(),
                          ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_TIMER_GET()),
                          sec, msec,
                          file_name, line_number, subsystem_name);
#else
      zb_osif_trace_printf(s_trace_file, "%d %x %s %d/%d/%03d.%03d %s:%d\t[%s] ",
                          zb_trace_get_counter(), osif_get_thread_id(), ltime, ZB_TIMER_GET(),
                          ZB_TIME_BEACON_INTERVAL_TO_MSEC(ZB_TIMER_GET()),
                          sec, msec,
                          file_name, line_number, subsystem_name);
#endif
    }
    zb_trace_inc_counter();
    va_start(arglist, args_size);
    zb_osif_trace_vprintf(s_trace_file, format, arglist);
    va_end(arglist);
    if (format[strlen(format) - 1] != '\n')
    {
      zb_osif_trace_printf(s_trace_file, "\n");
    }
    zb_osif_file_flush(s_trace_file);
#ifdef ZB_USE_LOGFILE_ROTATE
    zb_osif_log_file_rotate();
#endif

#endif /* ZB_TRACE_TO_SYSLOG */
    zb_osif_trace_unlock();
  }
#else
  (void)file_name;
  (void)line_number;
  (void)format;
  (void)args_size;
#endif
}

void zb_file_trace_vprintf(const char *format, va_list arglist)
{
  zb_osif_trace_vprintf(s_trace_file, (char*)format, arglist);
}

/** @} */
/** @endcond */ /* DOXYGEN_DEBUG_SECTION */

#endif  /* (defined ZB_TRACE_TO_FILE || defined ZB_TRACE_TO_SYSLOG) */
