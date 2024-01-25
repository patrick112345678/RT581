/**
 * @file log_type.h
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __LOG_TYPE_H__
#define __LOG_TYPE_H__

typedef enum _log_leve {
    LOG_LEVEL_ALL = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_ASSERT,
    LOG_LEVEL_NEVER,
} log_level_t;

typedef struct _log_info {
    log_level_t *level; //default 0
    char *name;
} log_info_t;

#endif
