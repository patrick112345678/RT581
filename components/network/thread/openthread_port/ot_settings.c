#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <openthread/platform/settings.h>
#include "EnhancedFlashDataset.h"

#include "log.h"

#define OT_MAX_KEY_LEN              20
#define OT_MAX_ENTRY_NUM            512
#define OT_MAX_ENTRY_BITMAP         ((OT_MAX_ENTRY_NUM + 31) / 32)

void otPlatSettingsInit(otInstance *aInstance, const uint16_t *aSensitiveKeys, uint16_t aSensitiveKeysLength)
{
}

void otPlatSettingsDeinit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

void otPlatSettingsSetCriticalKeys(otInstance *aInstance, const uint16_t *aKeys, uint16_t aKeysLength) {}

otError otPlatSettingsGet(otInstance *aInstance, uint16_t aKey, int aIndex, uint8_t *aValue, uint16_t *aValueLength)
{
    char            key[OT_MAX_KEY_LEN];
    uint32_t        bitmapArray[OT_MAX_ENTRY_BITMAP], i, j;
    size_t          valuLength = 0;
    otError         ret = OT_ERROR_NONE;
    struct env_node_obj obj;

    efd_port_env_lock();

    do
    {
        if (aIndex < 0)
        {
            ret = OT_ERROR_NOT_FOUND;
            break;
        }

        sprintf(key, "ot-bm-%x", aKey);
        efd_get_env_blob(key, bitmapArray, OT_MAX_ENTRY_BITMAP * 4, (size_t *)&valuLength);
        if (valuLength != OT_MAX_ENTRY_BITMAP * 4)
        {
            ret = OT_ERROR_NOT_FOUND;
            break;
        }

        i = aIndex >> 5;
        j = aIndex & 0xfffff;
        if (0 == (bitmapArray[i] & (1 << j)))
        {
            ret = OT_ERROR_NOT_FOUND;
            break;
        }

        sprintf(key, "ot-%x-%x", aKey, aIndex);
        if (!efd_get_env_obj(key, &obj))
        {

            sprintf(key, "ot-bm-%x", aKey);
            bitmapArray[i] = bitmapArray[i] & (~(1 << j));

            efd_set_env_blob(key, bitmapArray, OT_MAX_ENTRY_BITMAP * 4);
            ret = OT_ERROR_NOT_FOUND;
            break;
        }

        if (aValueLength)
        {
            if (aValue)
            {
                efd_get_env_blob(key, aValue, *aValueLength, (size_t *)&valuLength);
                memcpy(aValueLength, &valuLength, sizeof(uint16_t));
            }
            else
            {
                memcpy(aValueLength, &obj.value_len, sizeof(uint16_t));
            }
        }
    } while (0);

    efd_port_env_unlock();

    return ret;
}

static bool otPlatSettingsSet_raw(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength,
                                  int index, char *key, uint32_t *bitmapArray)
{
    EfErrCode   ret;
    uint32_t i = index >> 5;
    uint32_t j = index & 0xfffff;

    bitmapArray[i] = bitmapArray[i] | (1 << j);

    efd_port_env_lock();
    do
    {
        sprintf(key, "ot-%x-%x", aKey, index);
        ret = efd_set_env_blob(key, aValue, aValueLength);

        if (EFD_NO_ERR == ret)
        {
            sprintf(key, "ot-bm-%x", aKey);
            ret = efd_set_env_blob(key, bitmapArray, OT_MAX_ENTRY_BITMAP * 4);
        }
    } while (0);
    efd_port_env_unlock();

    return EFD_NO_ERR == ret ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

otError otPlatSettingsSet(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    char            key[OT_MAX_KEY_LEN];
    uint32_t        bitmapArray[OT_MAX_ENTRY_BITMAP];

    memset(bitmapArray, 0, OT_MAX_ENTRY_BITMAP * 4);
    return otPlatSettingsSet_raw(aInstance, aKey, aValue, aValueLength, 0, key, bitmapArray);
}

otError otPlatSettingsAdd(otInstance *aInstance, uint16_t aKey, const uint8_t *aValue, uint16_t aValueLength)
{
    char        key[OT_MAX_KEY_LEN];
    uint32_t    bitmapArray[OT_MAX_ENTRY_BITMAP];
    size_t      len = 0, index = 0, i, j, entryNum;
    otError     ret = OT_ERROR_NONE;

    efd_port_env_lock();

    do
    {
        sprintf(key, "ot-bm-%x", aKey);
        efd_get_env_blob(key, bitmapArray, OT_MAX_ENTRY_BITMAP * 4, (size_t *)&len);
        if (len != OT_MAX_ENTRY_BITMAP * 4)
        {
            memset(bitmapArray, 0, OT_MAX_ENTRY_BITMAP * 4);
            ret = otPlatSettingsSet_raw(aInstance, aKey, aValue, aValueLength, 0, key, bitmapArray);
        }
        else
        {
            entryNum = len * 8;
            for (index = 0; index < entryNum; index ++)
            {
                i = index >> 5;
                j = index & 0xfffff;
                if ((bitmapArray[i] & (1 << j)) == 0)
                {
                    break;
                }
            }

            if (index == entryNum)
            {
                ret = OT_ERROR_NO_BUFS;
                break;
            }

            ret = otPlatSettingsSet_raw(aInstance, aKey, aValue, aValueLength, index, key, bitmapArray);
        }
    } while (0);

    efd_port_env_unlock();

    return ret;
}

otError otPlatSettingsDelete(otInstance *aInstance, uint16_t aKey, int aIndex)
{
    char        key[OT_MAX_KEY_LEN];
    uint32_t    bitmapArray[OT_MAX_ENTRY_BITMAP];
    size_t      i, j, len;
    otError     ret = OT_ERROR_NONE;

    efd_port_env_lock();

    sprintf(key, "ot-bm-%x", aKey);
    do
    {

        if (aIndex < 0)
        {
            efd_del_env(key);
            break;
        }

        efd_get_env_blob(key, bitmapArray, OT_MAX_ENTRY_BITMAP * 4, (size_t *)&len);
        if (len != OT_MAX_ENTRY_BITMAP * 4)
        {
            ret = OT_ERROR_NOT_FOUND;
            break;
        }

        i = aIndex >> 5;
        j = aIndex & 0xfffff;
        if (! (bitmapArray[i] & (1 << j)))
        {
            ret = OT_ERROR_NOT_FOUND;
            break;
        }

        bitmapArray[i] = bitmapArray[i] & (~(1 << j));

        sprintf(key, "ot-bm-%x", aKey);
        efd_set_env_blob(key, bitmapArray, OT_MAX_ENTRY_BITMAP * 4);

        sprintf(key, "ot-%x-%x", aKey, aIndex);
        efd_del_env(key);

    } while (0);

    efd_port_env_unlock();

    return ret;
}

void otPlatSettingsWipe(otInstance *aInstance)
{
    /** reset all */
    efd_env_set_default();
}
