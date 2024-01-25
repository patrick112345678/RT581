/**
 * @file log_cfg.h
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-01
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef __LOG_CFG_H__
#define __LOG_CFG_H__

#define LOG_POWERON_SOFTLEVEL_COMPONENT    (LOG_LEVEL_INFO)
#define LOG_POWERON_SOFTLEVEL_FILE         (LOG_LEVEL_INFO)
#define LOG_POWERON_SOFTLEVEL_PRI          (LOG_LEVEL_INFO)
#define LOG_NAMELEN_MAX                    (128)
#define LOG_DUMP_DEDUPLICATE               (1)
#define LOG_TEST                           (0)

#define LOG_LINE_BUF_SIZE                  (256)

#define __log_printf                       rf_printk

#endif
