/**************************************************************************//**
 * @file     CRYPTO.h
 * @version  
 * @brief    CRYPTO ACCELEATOR driver header file
 *
 * @copyright 
 ******************************************************************************/
#ifndef ___CRYPTO_H__
#define ___CRYPTO_H__



#include "RT584_CM33.h"



#ifdef __cplusplus
extern "C"
{
#endif



/*secp256r1 curve is 256 bytes, 8*uint32_t = 256 */
#define secp256r1_op_num      8
    
extern uint32_t gfp_ecc_curve_p256_init(void);


extern void gfp_point_p256_mult(
    uint32_t *p_result_x, 
    uint32_t *p_result_y, 
    uint32_t *target_x, 
    uint32_t *target_y, 
    uint32_t *target_k
);

extern void gfp_ecdsa_p256_verify_init(void);


extern uint32_t gfp_ecdsa_p256_verify(uint32_t *p_result_x, uint32_t *r, uint32_t *s, uint32_t *h, uint32_t *qx, uint32_t *qy);


#ifdef __cplusplus
}
#endif

#endif
