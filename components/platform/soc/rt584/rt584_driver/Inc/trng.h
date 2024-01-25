
#ifndef __RT584_TRNG_H__
#define __RT584_TRNG_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Please NOTICE: TRNG MUST be secure hardware.*/

extern void random_number_reset(void);

extern uint32_t get_random_number(void);

#ifdef __cplusplus
}
#endif

#endif

