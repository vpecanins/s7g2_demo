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
 * File Name    : hw_fmi_private.h
 * Description  : FMI
 **********************************************************************************************************************/


/*******************************************************************************************************************//**
 * @addtogroup FMI
 * @{
 **********************************************************************************************************************/

#ifndef HW_FMI_PRIVATE_H
#define HW_FMI_PRIVATE_H

/**********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "bsp_api.h"
#include "r_fmi.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define FMI_TABLE_LENGTH_WORDS        (256) ///< FMI table is 256 4-byte words
#define FMI_PRIV_MAX_PERIPHERAL_MAPS  (16)  ///< Maximum number peripheral maps per record

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
typedef enum e_fmi_priv_record_enum
{
    FMI_PRODUCT_INFORMATION,//!< FMI_PRODUCT_INFORMATION
    SSP_IP_INFORMATION,     //!< SSP_IP_INFORMATION
    FMI_SW_PROVISIONING,    //!< FMI_SW_PROVISIONING
    FMI_TYPE_COUNT          //!< FMI_TYPE_COUNT
} fmi_priv_record_enum_t;

typedef union u_fmi_priv_table
{
    /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
    struct
    {
        fmi_header_t header;
        uint32_t offset[FMI_TYPE_COUNT];
    };
    uint32_t data[FMI_TABLE_LENGTH_WORDS];
} fmi_priv_table_t;

typedef struct st_fmi_priv_peripheral_map
{
    uint32_t mask :16; // [0:15] ip mask
    uint32_t prior :8; // [16:23] count of bits in all prior mask records
    uint32_t lower :4; // [24:27] number of bits in lower byte of mask
    uint32_t reserved :4;
} fmi_priv_peripheral_map_t;

typedef struct st_fmi_priv_peripheral_record
{
    fmi_header_t header;
    uint32_t base;               // base address of IP for compact format

    /* likely only partially populated based on bit count in header */
    fmi_priv_peripheral_map_t map[FMI_PRIV_MAX_PERIPHERAL_MAPS];
} fmi_priv_peripheral_record_t;

typedef struct st_fmi_priv_peripheral_locator
{
    uint16_t offset :10;          //[0:9] index from ff base to IP record (as uint32_t index...pFF[offset])
    uint16_t reserved :5;
    uint16_t verbose :1;          // [15:15] flag to indicate verbose formats
} fmi_priv_peripheral_locator_t;

/** Factory MCU information feature formats. */
typedef enum e_fmi_priv_format
{
    FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_COMMON_VARIANT = 0,     ///< Verbose multichannel common variant
    FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT = 1,      ///< Verbose multichannel multi variant
    FMI_PRIV_FORMAT_VERBOSE_MULTICHANNEL_MULTI_VARIANT_MASK = 2, ///< Verbose multichannel multi variant mask
    FMI_PRIV_FORMAT_VERBOSE_FLASH = 3,                           ///< Verbose flash
    FMI_PRIV_FORMAT_VERBOSE_NON_CHANNEL = 4,                     ///< Verbose non-channel
    FMI_PRIV_FORMAT_VERBOSE_RAM = 5,                             ///< Verbose RAM
    FMI_PRIV_FORMAT_COMPACT = 0XFF                               ///< Compact
} fmi_priv_format_t;

/** Factory MCU information feature data format. */
typedef struct st_fmi_priv_ip
{
    /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
    union
    {
        /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
        struct
        {
            uint32_t addr_16:16;      ///< Bits 8-23 of register base address
            uint32_t variant_data:8;  ///< Peripheral specific variant data
            uint32_t version_minor:4; ///< Major version of factory flash information
            uint32_t version_major:4; ///< Minor version of factory flash information
        }compact;
        /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
        struct
        {
            uint32_t record_continue:1; ///< Set if there is another unit of this peripheral after the current unit
            uint32_t format:7;          ///< One of ::fmi_priv_format_t (FMI_PRIV_FORMAT_COMPACT not valid)
            uint32_t variant_data:8;    ///< Peripheral specific variant data
            uint32_t contents:8;        ///< Size of extended data or number of flash regions
            uint32_t version_minor:4;   ///< Major version of factory flash information
            uint32_t version_major:4;   ///< Minor version of factory flash information
        }verbose;
    };
    /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
    union
    {
        /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
        struct
        {
            uint32_t channel_count:8;  ///< Largest valid channel number in this peripheral
            uint32_t :8;
            uint32_t channel_size:16;  ///< Difference between channel 0 base address and channel 1 base address
            uint32_t base_address;     ///< Base register address for channel 0
            /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
            union
            {
                uint8_t variant_data[128];  ///< Channel specific variant data (defined by peripheral)
                uint32_t mask[32];          ///< Channel specific extended data (defined by peripheral)
            };
        }format_verbose;
        /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
        struct
        {
            uint32_t start;    ///< Start address of flash region
            uint32_t end;      ///< End address of flash region
            uint32_t erase;    ///< Minimum flash erase size
            uint32_t write;    ///< Minimum flash program size
        }format_verbose_flash[32];
        /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
        struct
        {
            uint8_t variant[4];  ///< RAM variant
            /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
            struct
            {
                uint32_t start;  ///< Start address of RAM region
                uint32_t end;    ///< End address of RAM region
            }region[4];
        }format_verbose_ram[4];
        uint32_t format_verbose_non_channel_address;  ///< Base address for non-channel format
     };
} fmi_priv_ip_t;

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /* HW_FMI_PRIVATE_H */

/*******************************************************************************************************************//**
 * @} (end addtogroup FMI)
 **********************************************************************************************************************/
