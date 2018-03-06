/***********************************************************************************************************************
 * Copyright [2015-2017] Renesas Electronics Corporation and/or its licensors. All Rights Reserved.
 * 
 * This file is part of Renesas SynergyTM Software Package (SSP)
 *
 * The contents of this file (the "contents") are proprietary and confidential to Renesas Electronics Corporation
 * and/or its licensors ("Renesas") and subject to statutory and contractual protections.
 *
 * This file is subject to a Renesas SSP license agreement. Unless otherwise agreed in an SSP license agreement with
 * Renesas: 1) you may not use, copy, modify, distribute, display, or perform the contents; 2) you may not use any name
 * or mark of Renesas for advertising or publicity purposes or in connection with your use of the contents; 3) RENESAS
 * MAKES NO WARRANTY OR REPRESENTATIONS ABOUT THE SUITABILITY OF THE CONTENTS FOR ANY PURPOSE; THE CONTENTS ARE PROVIDED
 * "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT; AND 4) RENESAS SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, OR
 * CONSEQUENTIAL DAMAGES, INCLUDING DAMAGES RESULTING FROM LOSS OF USE, DATA, OR PROJECTS, WHETHER IN AN ACTION OF
 * CONTRACT OR TORT, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE CONTENTS. Third-party contents
 * included in this file may be subject to different terms.
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : r_fmi.c
 * Description  : FMI HAL API
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "r_fmi.h"
#include "r_fmi_cfg.h"
#include "hw/hw_fmi_private.h"
#include "hw/common/hw_fmi_common.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** Macro for error logger. */
#ifndef FMI_ERROR_RETURN
/*LDRA_INSPECTED 77 S This macro does not work when surrounded by parentheses. */
#define FMI_ERROR_RETURN(a, err) SSP_ERROR_RETURN((a), (err), &g_module_name[0], &g_fmi_version)
#endif

/* Compact address bit shift. */
#define FMI_PRIV_COMPACT_ADDR_SHIFT    (8U)

/* Number of channels in compact format. */
#define FMI_PRIV_COMPACT_CHANNEL_COUNT (1U)

/* Value in record_continue if next unit exists. */
#define FMI_PRIV_NEXT_UNIT_EXISTS      (1UL)

/* FMI variant data bit indicating that the IP exists on the die. */
#define FMI_VARIANT_IP_PINNED_OUT_ON_DIE (0x2U)

/* Memory size is the 7th character in the part number string (where the last character is character 0). */
#define FMI_PRIV_MEMORY_SIZE_INDEX     (7U)

/* Feature set is the 8th character in the part number string (where the last character is character 0). */
#define FMI_PRIV_FEATURE_SET_INDEX     (8U)

/* The character '7' represents the superset in the part number string. */
#define FMI_PRIV_FEATURE_SET_SUPERSET  ((uint8_t) '7')

/* The unique ID is at indices 5-8 in the data table. */
#define FMI_PRIV_UNIQUE_ID_INDEX_BEGIN (5U)
#define FMI_PRIV_UNIQUE_ID_INDEX_END   (8U)

/* Length of the part number array in the product information in bytes. */
#define FMI_PRIV_PART_NUMBER_LENGTH_BYTES (16U)

/* Length of the unique ID array in the product information in words. */
#define FMI_PRIV_UNIQUE_ID_LENGTH_WORDS   (4U)

/* The upper 16 bits of the address in FMIFRT must be 0x0100. */
#define FMI_PRIV_BASE_ADDRESS_UPPER_16_BITS (0x0100U)

/* The product name starts with 'R7FS' for all Synergy devices. */
#define FMI_PRIV_PRODUCT_NAME_START       (0x53463752U)

/* The product name starts at index 9 of the factory flash. */
#define FMI_PRIV_PRODUCT_NAME_INDEX_BEGIN (9U)

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
#if defined(__GNUC__)

/* This structure is affected by warnings from a GCC compiler bug. This pragma suppresses warnings in this structure 
 * only.*/
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
/** Version data structure used by error logger macro. */
static const ssp_version_t g_fmi_version =
{
    .api_version_minor  = FMI_API_VERSION_MINOR,
    .api_version_major  = FMI_API_VERSION_MAJOR,
    .code_version_major = FMI_CODE_VERSION_MAJOR,
    .code_version_minor = FMI_CODE_VERSION_MINOR
};
#if defined(__GNUC__)
/* Restore warning settings for 'missing-field-initializers' to as specified on command line. */
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic pop
#endif

#if BSP_CFG_ERROR_LOG != 0
static const char g_module_name[] = "fmi";
#endif

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/
static ssp_err_t R_FMI_Init (void);

static ssp_err_t R_FMI_ProductInfoGet (fmi_product_info_t ** pp_product_info);

static ssp_err_t R_FMI_UniqueIdGet(fmi_unique_id_t * p_unique_id);

static ssp_err_t R_FMI_FeatureGet (ssp_feature_t const * const p_feature, fmi_feature_info_t * const p_info);

static ssp_err_t R_FMI_EventInfoGet (ssp_feature_t const * const p_feature,
                                     ssp_signal_t                signal,
                                     fmi_event_info_t * const    p_info);

static ssp_err_t R_FMI_VersionGet (ssp_version_t * const p_version);


static uint32_t fmi_size_lookup(fmi_priv_ip_t * p_ip, fmi_priv_format_t format);

static ssp_err_t fmi_unit_locate(ssp_feature_t    const * const p_feature,
                                 fmi_priv_format_t      * const p_ip_format,
                                 fmi_priv_ip_t          **      pp_ip);

static ssp_err_t fmi_multichannel_feature_info_get(ssp_feature_t    const * const p_feature,
                                                   fmi_priv_format_t              ip_format,
                                                   fmi_priv_ip_t          *       p_ip,
                                                   fmi_feature_info_t     * const p_info);

static ssp_err_t fmi_feature_info_get(ssp_feature_t    const * const p_feature,
                                      uint32_t                       fmi_compact_base,
                                      fmi_priv_format_t              ip_format,
                                      fmi_priv_ip_t          *       p_ip,
                                      fmi_feature_info_t     * const p_info);

static ssp_err_t fmi_part_number_validate (void);

static ssp_err_t fmi_table_validate (uint32_t * p_table);

static ssp_err_t fmi_on_chip_table_validate (uint32_t * p_table);

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @addtogroup FMI
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
/*LDRA_INSPECTED 27 D This structure must be accessible in user code. It cannot be static. */
const fmi_api_t g_fmi_on_fmi =
{
    .init              = R_FMI_Init,
    .productInfoGet    = R_FMI_ProductInfoGet,
    .productFeatureGet = R_FMI_FeatureGet,
    .versionGet        = R_FMI_VersionGet,
    .eventInfoGet      = R_FMI_EventInfoGet,
    .uniqueIdGet       = R_FMI_UniqueIdGet
};

/*******************************************************************************************************************//**
 * Initializes factory flash base pointer.  Implements fmi_api_t::init.
 *
 * @retval SSP_SUCCESS                    Factory flash initialization successful.
 * @retval SSP_ERR_INVALID_FMI_DATA       The FMI data table provided is not valid, or at least one field in the part
 *                                        number is not compatible with the FMI data table provided.
 **********************************************************************************************************************/
static ssp_err_t R_FMI_Init (void)
{
    /** Verify the provided MCU information data table is valid.  This table is required to use the SSP.  If the data
     * in this table is not valid, log an unrecoverable error. */
    ssp_err_t err = fmi_table_validate((uint32_t *) R_FMI_TABLE);
    if (SSP_SUCCESS != err)
    {
        BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
    }
    FMI_ERROR_RETURN(SSP_SUCCESS == err, err);

    /** Check to see if the factory flash is valid. */
    R_FMI_FACTORY_FLASH_TABLE = NULL;

    /** The upper 16 bits of the base address of the factory flash must be 0x0100. */
    if (FMI_PRIV_BASE_ADDRESS_UPPER_16_BITS == ((uint16_t) (((uint32_t) *R_FMIFRT) >> 16)))
    {
        err = fmi_on_chip_table_validate((uint32_t *) *R_FMIFRT);
        if (SSP_SUCCESS == err)
        {
            /** If the factory flash is valid, store the base address for later and compare the part number to the
             * part number mask match. */
            R_FMI_FACTORY_FLASH_TABLE = (fmi_priv_table_t const *) *R_FMIFRT;

            err = fmi_part_number_validate();
            if (SSP_SUCCESS != err)
            {
                BSP_CFG_HANDLE_UNRECOVERABLE_ERROR(0);
            }
            FMI_ERROR_RETURN(SSP_SUCCESS == err, err);
        }
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Get pointer to Factory MCU Information product information record.  Implements fmi_api_t::productInfoGet.
 *
 * @retval        SSP_SUCCESS          Caller's pointer set to Product Information Record.
 * @retval        SSP_ERR_ASSERTION    Caller's pointer is NULL.
 **********************************************************************************************************************/
static ssp_err_t R_FMI_ProductInfoGet (fmi_product_info_t ** pp_product_info)
{
#if FMI_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(NULL != pp_product_info);
#endif

    fmi_header_t * product_info_address;

    /** Use the factory flash if it is valid.  Otherwise use the SSP MCU information. */
    fmi_priv_table_t const * p_table = R_FMI_TABLE;
    if (NULL != R_FMI_FACTORY_FLASH_TABLE)
    {
        p_table = R_FMI_FACTORY_FLASH_TABLE;
    }
    product_info_address = HW_FMI_RecordLocate(p_table, FMI_PRODUCT_INFORMATION);
    *pp_product_info = (fmi_product_info_t *) product_info_address;

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Get unique ID for this device.  Implements fmi_api_t::uniqueIdGet.
 *
 * @retval      SSP_SUCCESS                      Unique ID stored in p_unique_id
 * @retval      SSP_ERR_ASSERTION                p_unique_id was NULL
 * @retval      SSP_ERR_INVALID_FACTORY_FLASH    Factory flash is not valid
 **********************************************************************************************************************/
static ssp_err_t R_FMI_UniqueIdGet(fmi_unique_id_t * p_unique_id)
{
#if FMI_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(NULL != p_unique_id);
#endif

    /** If the factory flash is not valid, return an error. */
    FMI_ERROR_RETURN(NULL != R_FMI_FACTORY_FLASH_TABLE, SSP_ERR_INVALID_FACTORY_FLASH);

    /** Store unique ID in p_unique_id. */
    fmi_product_info_t * p_product_info = (fmi_product_info_t *)
            &R_FMI_FACTORY_FLASH_TABLE->data[R_FMI_FACTORY_FLASH_TABLE->offset[FMI_PRODUCT_INFORMATION]];

    uint32_t * p_src = (uint32_t *) &p_product_info->unique_id[0];
    uint32_t * p_dest = &p_unique_id->unique_id[0];

    for (uint32_t i = 0U; i < FMI_PRIV_UNIQUE_ID_LENGTH_WORDS; i++)
    {
        *p_dest = *p_src;
        p_dest++;
        p_src++;
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Get feature information for the requested feature.  Implements fmi_api_t::productFeatureGet.
 *
 * @retval   SSP_SUCCESS                     Feature information stored in p_info
 * @retval   SSP_ERR_ASSERTION               p_feature was NULL or p_info was NULL
 * @retval   SSP_ERR_IP_CHANNEL_NOT_PRESENT  Requested channel does not exist on this MCU
 * @retval   SSP_ERR_IP_UNIT_NOT_PRESENT     Requested unit does not exist on this MCU
 * @retval   SSP_ERR_INTERNAL                Requested feature is in a format not supported at this time
 **********************************************************************************************************************/
static ssp_err_t R_FMI_FeatureGet (ssp_feature_t const * const p_feature, fmi_feature_info_t * const p_info)
{
#if FMI_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(NULL != p_feature);
    SSP_ASSERT(NULL != p_info);
#endif
    ssp_err_t              err = SSP_ERR_INTERNAL;
    fmi_priv_format_t      ip_format = FMI_PRIV_FORMAT_COMPACT;
    fmi_priv_ip_t        * p_ip;
    uint32_t               fmi_compact_base = 0U;

    /** Get the factory flash base address for this IP. */
    p_ip = HW_FMI_IPLocate(p_feature->id, &ip_format, &fmi_compact_base);
    FMI_ERROR_RETURN(NULL != p_ip, SSP_ERR_IP_HARDWARE_NOT_PRESENT);

    /** Find the factory flash base address for this unit. */
    err = fmi_unit_locate(p_feature, &ip_format, &p_ip);
    FMI_ERROR_RETURN(SSP_SUCCESS == err, err);

    /** Populate the feature information. */
    err = fmi_feature_info_get(p_feature, fmi_compact_base, ip_format, p_ip, p_info);
    FMI_ERROR_RETURN(SSP_SUCCESS == err, err);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief      Get event information for the requested feature and signal.  Implements fmi_api_t::eventInfoGet.
 *
 * @retval      SSP_SUCCESS                Event information stored in p_info
 * @retval      SSP_ERR_ASSERTION          p_feature was NULL or p_info was NULL
 * @retval      SSP_ERR_IRQ_BSP_DISABLED   Event information could not be found.  p_info::irq is set to
 *                                         SSP_INVALID_VECTOR and p_info::event is set to 0xFF.
 **********************************************************************************************************************/
static ssp_err_t R_FMI_EventInfoGet (ssp_feature_t const * const p_feature,
                                     ssp_signal_t                signal,
                                     fmi_event_info_t * const    p_info)
{
#if FMI_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(NULL != p_feature);
    SSP_ASSERT(NULL != p_info);
#endif

    /* Calculate ELC event. */
    ssp_err_t err = HW_FMI_EventGet(p_feature, signal, &p_info->irq, &p_info->event);

    FMI_ERROR_RETURN(SSP_SUCCESS == err, err);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Get the driver version based on compile time macros.  Implements fmi_api_t::versionGet.
 *
 * @retval     SSP_SUCCESS          Caller's structure written.
 * @retval     SSP_ERR_ASSERTION    Caller's pointer is NULL.
 *
 **********************************************************************************************************************/
static ssp_err_t R_FMI_VersionGet (ssp_version_t * const p_version)
{
#if FMI_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(NULL != p_version);
#endif

    p_version->version_id = g_fmi_version.version_id;

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @} (end addtogroup FMI)
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * Validate the part number in the SSP MCU information is compatible with the part number in the factory flash.
 *
 * @retval  SSP_SUCCESS               Part number in MCU information is compatible with part number in factory flash
 * @retval  SSP_ERR_INVALID_FMI_DATA  Part number in MCU information is not compatible with part number in factory flash
 **********************************************************************************************************************/
static ssp_err_t fmi_part_number_validate (void)
{
    fmi_priv_table_t const * p_reg = *R_FMIFRT;
    fmi_product_info_t * p_product_info = (fmi_product_info_t *) &p_reg->data[p_reg->offset[FMI_PRODUCT_INFORMATION]];
    fmi_product_info_t * p_ssp_product_info = (fmi_product_info_t *) &R_FMI_TABLE->data[R_FMI_TABLE->offset[FMI_PRODUCT_INFORMATION]];

    for (uint32_t i = 0U; i < FMI_PRIV_PART_NUMBER_LENGTH_BYTES; i++)
    {
        if ((FMI_CFG_PART_NUMBER_VALIDATE_MASK & (1U << i)) > 0U)
        {
            uint32_t index = ((uint32_t) (FMI_PRIV_PART_NUMBER_LENGTH_BYTES - 1U) - i);
            if (FMI_PRIV_MEMORY_SIZE_INDEX == i)
            {
                /** For memory size, larger memory sizes are allowed. */
                FMI_ERROR_RETURN(p_product_info->product_name[index] >= p_ssp_product_info->product_name[index], SSP_ERR_INVALID_FMI_DATA);
            }
            else if (FMI_PRIV_FEATURE_SET_INDEX == i)
            {
                /** For feature set, superset is always allowed. */
                if (FMI_PRIV_FEATURE_SET_SUPERSET != p_product_info->product_name[index])
                {
                    FMI_ERROR_RETURN(p_ssp_product_info->product_name[index] == p_product_info->product_name[index], SSP_ERR_INVALID_FMI_DATA);
                }
            }
            else
            {
                /** All other part number fields must match exactly. */
                FMI_ERROR_RETURN(p_ssp_product_info->product_name[index] == p_product_info->product_name[index], SSP_ERR_INVALID_FMI_DATA);
            }
        }
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Validate an FMI data table by ensuring the version is non-zero and the checksum is correct.
 *
 * @param[in]  p_table     Base address of the table to validate
 *
 * @retval  SSP_SUCCESS               FMI data table is valid
 * @retval  SSP_ERR_INVALID_FMI_DATA  FMI data table is not valid
 **********************************************************************************************************************/
static ssp_err_t fmi_on_chip_table_validate (uint32_t * p_table)
{
    /** The first index entry in the data table is the version number.  A value of 0 or 0xFFFFFFFF indicates invalid
     * factory flash. */
    FMI_ERROR_RETURN(0U != p_table[0], SSP_ERR_INVALID_FMI_DATA);
    FMI_ERROR_RETURN(0xFFFFFFFFU != p_table[0], SSP_ERR_INVALID_FMI_DATA);

    /** The first 4 characters of the product name are 'R7FS' for all Synergy devices. */
    FMI_ERROR_RETURN(FMI_PRIV_PRODUCT_NAME_START == p_table[FMI_PRIV_PRODUCT_NAME_INDEX_BEGIN], SSP_ERR_INVALID_FMI_DATA);

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Validate an FMI data table by ensuring the version is non-zero and the checksum is correct.
 *
 * @param[in]  p_table     Base address of the table to validate
 *
 * @retval  SSP_SUCCESS               FMI data table is valid
 * @retval  SSP_ERR_INVALID_FMI_DATA  FMI data table is not valid
 **********************************************************************************************************************/
static ssp_err_t fmi_table_validate (uint32_t * p_table)
{
    uint32_t checksum  = 0U;

    /** The first index entry in the data table is the version number, which cannot be 0. */
    FMI_ERROR_RETURN(0U != p_table[0], SSP_ERR_INVALID_FMI_DATA);

    for (uint32_t i = 0U; i < FMI_TABLE_LENGTH_WORDS; i++)
    {
        checksum += p_table[i];
    }

    /** Checksum (sum of all bytes in factory flash) should equal 0 if data is valid. */
    FMI_ERROR_RETURN(0U == checksum, SSP_ERR_INVALID_FMI_DATA);

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Get the size of the information for this IP.
 *
 * @param[in]  p_ip        Pointer to the IP information
 * @param[in]  format      Format of the IP information in p_ip
 *
 * @return     size of p_ip in bytes
 **********************************************************************************************************************/
static uint32_t fmi_size_lookup(fmi_priv_ip_t * p_ip, fmi_priv_format_t format)
{
    uint32_t size_words = 0U;
    uint32_t channel_count = 0U;
    uint32_t contents = 0U;

    /** Size lookup is based on fmi_priv_ip_t. */
    switch (format)
    {
    case FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_COMMON_VARIANT:
    {
        /** For FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_COMMON_VARIANT,
         * sizeof()...always 3 words */
        /** 1 word for fmi_priv_ip_t::verbose. */
        /** 2 words for fmi_priv_ip_t::format_verbose (through fmi_priv_ip_t::format_verbose::base_address,
         * extra variant data array is not used in the common variant format). */
        size_words = 3U;
        break;
    }
    case FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT:
    {
        /** For FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT,
         * sizeof()... 3 + ((channel_count+3) / 4) words */
        /** 1 word for fmi_priv_ip_t::verbose. */
        /** 2 words for fmi_priv_ip_t::format_verbose (through fmi_priv_ip_t::format_verbose::base_address,
         * extra variant data array size calculated in next comment). */
        /** Each channel has 1 byte of variant data.  Round up to the nearest multiple of 4 bytes (1 word) by adding
         * 3 to the channel count, then dividing by 4. */
        channel_count = p_ip->format_verbose.channel_count;
        size_words = 3U + ((channel_count + 3U) / 4);
        break;
    }
    case FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT_MASK:
    {
        /** For FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT_MASK,
         * sizeof()... 3 + ((channel_count+3) / 4) + (channel_count * contents) words */
        /** 1 word for fmi_priv_ip_t::verbose. */
        /** 2 words for fmi_priv_ip_t::format_verbose (through fmi_priv_ip_t::format_verbose::base_address,
         * extra variant data array size calculated in next comment). */
        /** Each channel has 1 byte of variant data.  Round up to the nearest multiple of 4 bytes (1 word) by adding
         * 3 to the channel count, then dividing by 4. */
        /** In this format, each channel has one or more words of mask data.  The number of words of mask data per
         * channel is defined by fmi_priv_ip_t::verbose::contents. */
        channel_count = p_ip->format_verbose.channel_count;
        contents = p_ip->verbose.contents;
        size_words = 3U + ((channel_count + 3U) / 4) + (channel_count * contents);
        break;
    }
    case FMI_PRIV_FORMAT_VERBOSE_FLASH:
    {
        /** For FMI_PRIV_FORMAT_VERBOSE_FLASH,
         * sizeof()... 1 + (4 * contents) words */
        /** 1 word for fmi_priv_ip_t::verbose. */
        /** 4 words per region for start, end, erase, and write. The number of regions is defined in
         * fmi_priv_ip_t::verbose::contents. */
        contents = p_ip->verbose.contents;
        size_words = 1U + (4U * contents);
        break;
    }
    case FMI_PRIV_FORMAT_VERBOSE_NON_CHANNEL:
    {
        /** For FMI_PRIV_FORMAT_VERBOSE_NON_CHANNEL,
         * sizeof()... always 2 words */
        /** 1 word for fmi_priv_ip_t::verbose. */
        /** 1 word for fmi_priv_ip_t::format_verbose_non_channel_address. */
        size_words = 2U;
        break;
    }
    case FMI_PRIV_FORMAT_VERBOSE_RAM:
    {
        /** For FMI_PRIV_FORMAT_VERBOSE_RAM,
         * Not yet supported. sizeof()... 1 + ((contents+3) / 4) + (contents * 2)) words */
        /** 1 word for fmi_priv_ip_t::verbose. */
        /** Each region has 1 byte of variant data.  The number of regions is defined in
         * fmi_priv_ip_t::verbose::contents.  Round up to the nearest multiple of 4 bytes (1 word) by adding
         * 3 to the number of regions, then dividing by 4. */
        /** Each region has 2 words for region start and end.  The number of regions is defined in
         * fmi_priv_ip_t::verbose::contents. */
        uint32_t regions = p_ip->verbose.contents;
        size_words = 1U + ((regions + 3U) / 4) + (regions * 2U);
        break;
    }
    default:
        break;
    }

    return size_words * sizeof(uint32_t);
}

/*******************************************************************************************************************//**
 * Adjust the pointer to the information for this feature based on the unit requested.
 *
 * @param[in]      p_feature        Pointer to the feature requested
 * @param[in,out]  p_ip_format      Format of the IP information in pp_ip
 * @param[in,out]  pp_ip            Pointer to pointer to information for this feature
 *
 * @retval   SSP_SUCCESS                     *pp_ip points to the information for unit requested in p_feature
 * @retval   SSP_ERR_IP_UNIT_NOT_PRESENT     Requested unit does not exist on this IP
 **********************************************************************************************************************/
static ssp_err_t fmi_unit_locate(ssp_feature_t    const * const p_feature,
                                 fmi_priv_format_t      * const p_ip_format,
                                 fmi_priv_ip_t          **      pp_ip)
{
    /** If unit is non-zero, locate the unit. */
    fmi_priv_ip_t * p_ip = *pp_ip;
    if (0U != p_feature->unit)
    {
        /** Unit not yet supported in compact format. */
        FMI_ERROR_RETURN((FMI_PRIV_FORMAT_COMPACT != *p_ip_format), SSP_ERR_IP_UNIT_NOT_PRESENT);

        uint32_t unit_countdown = p_feature->unit;

        while (unit_countdown > 0U)
        {
            unit_countdown--;

            /** Make sure the next unit exists. */
            FMI_ERROR_RETURN((FMI_PRIV_NEXT_UNIT_EXISTS == p_ip->verbose.record_continue),
                    SSP_ERR_IP_UNIT_NOT_PRESENT);

            /** Increment unit pointer based on size of information at the current pointer. */
            p_ip = (fmi_priv_ip_t *) ((uint32_t) p_ip + fmi_size_lookup(p_ip, *p_ip_format));
            *p_ip_format = (fmi_priv_format_t) p_ip->verbose.format;
        }
    }

    *pp_ip = p_ip;

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Populate factory flash information for peripherals with multiple channels.
 *
 * @param[in]      p_feature        Pointer to the feature requested
 * @param[in]      ip_format        Format of the IP information in p_ip
 * @param[in]      p_ip             Pointer to raw information for this feature
 * @param[out]     p_info           Processed information for this feature stored here
 *
 * @retval   SSP_SUCCESS                     p_info points to the information p_feature
 * @retval   SSP_ERR_IP_CHANNEL_NOT_PRESENT  Requested channel does not exist on this IP
 **********************************************************************************************************************/
static ssp_err_t fmi_multichannel_feature_info_get(ssp_feature_t    const * const p_feature,
                                                   fmi_priv_format_t              ip_format,
                                                   fmi_priv_ip_t          *       p_ip,
                                                   fmi_feature_info_t     * const p_info)
{
    /** Version data is common for verbose formats. */
    p_info->version_major = p_ip->verbose.version_major;
    p_info->version_minor = p_ip->verbose.version_minor;

    /** Determine channel count.  If requested channel is larger than the channel count, return an error. */
    p_info->channel_count = p_ip->format_verbose.channel_count;
    FMI_ERROR_RETURN(p_feature->channel < p_info->channel_count, SSP_ERR_IP_CHANNEL_NOT_PRESENT);

    /** Calculate peripheral register base address */
    p_info->ptr = (void *) (p_ip->format_verbose.base_address +
                      (uint32_t) (p_feature->channel * p_ip->format_verbose.channel_size));

    /** Grab variant data and extended data */
    switch (ip_format)
    {
        default:
        case FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_COMMON_VARIANT:
            /* In this format, all channels have same variant */
            p_info->variant_data = p_ip->verbose.variant_data;

            /* Extended data N/A in this format */
            p_info->extended_data_count = 0U;
            p_info->ptr_extended_data   = NULL;
            break;

        case FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT:
            /* Find the variant information for this channel.  Channel is checked to be valid above. */
            p_info->variant_data = (uint32_t) p_ip->format_verbose.variant_data[p_feature->channel];

            /* Extended data N/A in this format */
            p_info->extended_data_count = 0U;
            p_info->ptr_extended_data   = NULL;
            break;

        case FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT_MASK:
            /* Find the variant information for this channel.  Channel is checked to be valid above. */
            p_info->variant_data = (uint32_t) p_ip->format_verbose.variant_data[p_feature->channel];

            /* How much extended data per channel */
            p_info->extended_data_count = p_ip->verbose.contents;
            /* Find the extended information for this channel */
            /* Step over attribute data -> ((info->channel_count + 3)/4 bytes per word) */
            /* Each channel has 1 byte of attribute data.  Attribute data storage is rounded up to the nearest
             * multiple of 4 bytes (1 word).  The mask array is uint32_t type, so add the number of bytes per word
             * minus 1, then divide by the number of bytes per word to get the number of words of attribute data. */
            /* Each channel has info->extended_data_count words of mask data, so index into the mask data using
             * the channel number multiplied by the number of extended data count words per channel. */
            p_info->ptr_extended_data =
                &p_ip->format_verbose.mask[((p_info->channel_count + (sizeof(uint32_t) - 1U)) / sizeof(uint32_t))
                                          + (uint32_t) (p_feature->channel * p_info->extended_data_count)];
            break;
    }

    /** If the variant data is zero this channel is not present */
    FMI_ERROR_RETURN(0U != p_info->variant_data, SSP_ERR_IP_CHANNEL_NOT_PRESENT);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * Populate factory flash information.
 *
 * @param[in]      p_feature        Pointer to the feature requested
 * @param[in]      fmi_compact_base Base address of raw information if compact format is used
 * @param[in]      ip_format        Format of the IP information in p_ip
 * @param[in]      p_ip             Pointer to raw information for this feature
 * @param[out]     p_info           Processed information for this feature stored here
 *
 * @retval   SSP_SUCCESS                     p_info points to the information p_feature
 * @retval   SSP_ERR_IP_CHANNEL_NOT_PRESENT  Requested channel does not exist on this MCU
 * @retval   SSP_ERR_IP_UNIT_NOT_PRESENT     Requested unit does not exist on this MCU
 * @retval   SSP_ERR_INTERNAL                Requested feature is in a format not supported at this time
 **********************************************************************************************************************/
static ssp_err_t fmi_feature_info_get(ssp_feature_t    const * const p_feature,
                                      uint32_t                       fmi_compact_base,
                                      fmi_priv_format_t              ip_format,
                                      fmi_priv_ip_t          *       p_ip,
                                      fmi_feature_info_t     * const p_info)
{
    ssp_err_t err = SSP_ERR_INTERNAL;
    switch (ip_format)
    {
        case FMI_PRIV_FORMAT_COMPACT:
        {
            p_info->version_major = p_ip->compact.version_major;
            p_info->version_minor = p_ip->compact.version_minor;

            /* Determine channel count.  Always 1 in this format */
            p_info->channel_count = FMI_PRIV_COMPACT_CHANNEL_COUNT;

            FMI_ERROR_RETURN(p_feature->channel < p_info->channel_count, SSP_ERR_IP_CHANNEL_NOT_PRESENT);

            /* Calculate peripheral address.  Peripheral address = base address + (compact address << 8). */
            p_info->ptr = (void *) (fmi_compact_base +
                    (uint32_t) (p_ip->compact.addr_16 << FMI_PRIV_COMPACT_ADDR_SHIFT));

            /* In this format, all channels have same variant */
            p_info->variant_data        = p_ip->compact.variant_data;
            /* Determine extended data info...N/A in this format */
            p_info->extended_data_count = 0U;
            p_info->ptr_extended_data   = NULL;

            err = SSP_SUCCESS;
            break;
        }

        case FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_COMMON_VARIANT:
        case FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT:
        case FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT_MASK:
        {
            err = fmi_multichannel_feature_info_get(p_feature, ip_format, p_ip, p_info);

            break;
        }
        case FMI_PRIV_FORMAT_VERBOSE_NON_CHANNEL:
        {
            /* This block of data is common for verbose formats. */
            p_info->version_major = p_ip->verbose.version_major;
            p_info->version_minor = p_ip->verbose.version_minor;

            /* In this format, all channels have same variant */
            p_info->variant_data = p_ip->verbose.variant_data;

            /* No channels or variant data in this format. */
            p_info->channel_count = 0U;
            p_info->extended_data_count = 0U;
            p_info->ptr_extended_data = NULL;

            /* Base address from verbose format. */
            p_info->ptr = (void *) p_ip->format_verbose_non_channel_address;

            if (p_info->variant_data & FMI_VARIANT_IP_PINNED_OUT_ON_DIE)
            {
                err = SSP_SUCCESS;
            }
            else
            {
                err = SSP_ERR_IP_UNIT_NOT_PRESENT;
            }
            break;
        }

        case FMI_PRIV_FORMAT_VERBOSE_FLASH:
        {
            /* This block of data is common for verbose formats. */
            p_info->version_major = p_ip->verbose.version_major;
            p_info->version_minor = p_ip->verbose.version_minor;

            /* In this format, all channels have same variant */
            p_info->variant_data = p_ip->verbose.variant_data;

            /* This format has 4 words of data. */
            p_info->channel_count = p_ip->verbose.contents;
            p_info->extended_data_count = 4U;
            p_info->ptr = NULL;
            p_info->ptr_extended_data = &(p_ip->format_verbose_flash[0].start);
            err = SSP_SUCCESS;
            break;
        }
        case FMI_PRIV_FORMAT_VERBOSE_RAM:
        default:
            err = SSP_ERR_INTERNAL;
    }

    FMI_ERROR_RETURN(SSP_SUCCESS == err, err);
    return SSP_SUCCESS;
}
