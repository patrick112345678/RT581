
#ifndef ___SOFT_INT_REG_H__
#define ___SOFT_INT_REG_H__

typedef struct {
    __OM  uint32_t  ENABLE_IRQ;     /*0x00*/
    __OM  uint32_t  CLEAR_IRQ;      /*0x04*/
    __IM  uint32_t  IRQ_STATE;      /*0x08*/
    __IO  uint32_t  DATA;           /*0x0C*/
} SOFT_IRQ_T;

#define ENABLE_SOFT_IRQ            (1<<0)
#define CLEAR_SOFT_IRQ             (1<<0)

#endif

