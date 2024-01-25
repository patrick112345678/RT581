/**************************************************************************//**
 * @file     sysfun.H
 * @version
 * @brief    SYSTEM HELP FUNCTION API DEFINITION
 *
 * @copyright
 ******************************************************************************/

#ifndef ___SYSFUN_H__
#define ___SYSFUN_H__


extern void enter_critical_section(void);
extern void leave_critical_section(void);

extern uint32_t version_check(void);

extern int console_init(uint32_t baudrate);

typedef enum {
    IRQ_PRIORITY_HIGHEST = 0,
    IRQ_PRIORITY_HIGH = 1,
    IRQ_PRIORITY_NORMAL =3,
    IRQ_PRIORITY_LOW =5,
    IRQ_PRIORITY_LOWEST = 7,
} irq_priority_t;



#endif
