
#ifndef ___SOFT_INT_H__
#define ___SOFT_INT_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*soft_int_cb_t)(uint32_t data);

extern uint32_t Register_Soft_Intr(uint32_t id, soft_int_cb_t soft_cb_fun);

extern uint32_t Enable_Soft_Intr(uint32_t id);

extern uint32_t Get_Soft_Intr_Data(uint32_t id, uint32_t *data);

extern uint32_t Set_Soft_Intr_Data(uint32_t id, uint32_t data);

#ifdef __cplusplus
extern "C"
}
#endif

#endif
