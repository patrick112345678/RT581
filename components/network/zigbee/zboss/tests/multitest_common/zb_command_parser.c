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
/*  PURPOSE: Functions for multitest commands parsing
*/

#define ZB_TRACE_FILE_ID 40363
#include <ctype.h>
#include "zb_common.h"
#include "zb_bufpool.h"
#include "zb_osif.h"
#include "zb_command_parser.h"

/* Updates ptr to skip leading whitespaces */
static void skip_ws(char **ptr)
{
  while (**ptr != '\0' && isspace(**ptr))
  {
    (*ptr)++;
  }
}


zb_bool_t parse_command_token(char **ptr, char *dst, int max_len)
{
  zb_bool_t res = ZB_TRUE;

  max_len--; /* reserve one byte for '\0' */
  skip_ws(ptr);

  while (**ptr != '\0'
         && (isalnum(**ptr) || **ptr == '_' || **ptr == '-'))
  {
    if (max_len == 0)
    {
      /* no space in dst buffer */
      res = ZB_FALSE;
      break;
    }

    *dst = **ptr;
    dst++, (*ptr)++, max_len--;
  }

  *dst = '\0';
  return res;
}

/* Parses value for key-value pairs. */
static void read_param_value(char **ptr, char *dst, int max_len)
{
  /* currently the same function */
  parse_command_token(ptr, dst, max_len);
}

/* Handles strings like:
   key\s*=\s*value */
static zb_bool_t handle_parameter(char **ptr, param_handler_function_t func)
{
  char argument_key  [20];
  char argument_value[20];
  zb_bool_t res;

  res = parse_command_token(ptr, argument_key, sizeof(argument_key));
  skip_ws(ptr);

  if (!res)
  {
    // failed to parse key
  }
  else if (**ptr != '=')
  {
    // error, no = sign
    res = ZB_FALSE;
  }
  else if (*argument_key == '\0')
  {
    // error, empty key
    res = ZB_FALSE;
  }
  else
  {
    (*ptr)++; /* skip '=' symbol */

    read_param_value(ptr, argument_value, sizeof(argument_value));
    res = (*argument_value != '\0');
  }

  if (res)
  {
    func(argument_key, argument_value);
  }

  return res;
}

/**
 * Handles parameter list of the key = value
 * They may or may not be separeted by commas.
 * @returns true if everything is parsed and successfully handled
 */
zb_bool_t handle_command_parameter_list(char *ptr, param_handler_function_t func)
{
  zb_bool_t res = ZB_TRUE;

  while (*ptr != '\0' && res == ZB_TRUE)
  {
    skip_ws(&ptr);

    if (*ptr == ',')
    {
      /* skipping comma */
      ptr++;
    }

    res = handle_parameter(&ptr, func);

    skip_ws(&ptr);
  }

  return res;
}
