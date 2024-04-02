// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
#ifndef __MEM_MGMT_H__
#define __MEM_MGMT_H__

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
//                Include (Better to prevent)
//=============================================================================
#include <stdio.h>
#include <stdint.h>
#include "project_config.h"

//=============================================================================
//                Public Definitions of const value
//=============================================================================

//=============================================================================
//                Public ENUM
//=============================================================================

//=============================================================================
//                Public Struct
//=============================================================================

//=============================================================================
//                Public Function Declaration
//=============================================================================

//==================================
//     Memory functions
//==================================
void mem_mgmt_show_info(void);

/** Memory allocation
 * @param u32_size memory size in bytes
 */
void *mem_malloc_fn(uint32_t u32_size, const char *pc_func_ptr, uint32_t u32_line);
#define mem_malloc(len) mem_malloc_fn(len, __FUNCTION__, __LINE__)

/** Memory free
 * @param p_pointer
 */
void mem_free_fn(void *p_pointer, const char *pc_func_ptr, uint32_t u32_line);
#define mem_free(ptr) mem_free_fn(ptr, __FUNCTION__, __LINE__)

/** Memory copy
 * @param p_pointer_dest, p_pointer_src, lens
 */
void mem_copy_fn(void *p_pointer_dest, const void *p_pointer_src, uint32_t lens, const char *pc_func_ptr, uint32_t u32_line);
#define mem_memcpy(dest,src,len) mem_copy_fn(dest,src,len, __FUNCTION__, __LINE__)

#ifdef __cplusplus
};
#endif
#endif /* __MEM_MGMT_H__ */
