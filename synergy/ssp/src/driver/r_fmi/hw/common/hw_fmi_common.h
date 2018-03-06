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
 * File Name    : hw_fmi_common.h
 * Description  : FMI
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 *
 **********************************************************************************************************************/

#ifndef HW_FMI_COMMON_H
#define HW_FMI_COMMON_H

/**********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "bsp_api.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

#define R_FMIFRT ((fmi_priv_table_t const * const *) 0x407FB19C)

#define FMI_PRIV_BITS_PER_MAP_ENTRY        (16U)
#define FMI_PRIV_HALF_BITS_PER_MAP_ENTRY   (FMI_PRIV_BITS_PER_MAP_ENTRY / 2U)


/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static ssp_err_t HW_FMI_EventGet (ssp_feature_t const * const feature, ssp_signal_t signal,
                                  IRQn_Type * p_irq, elc_event_t * p_event);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
static fmi_priv_table_t const * const R_FMI_TABLE = (fmi_priv_table_t const *) FMI_CFG_FACTORY_FLASH_BASE_ADDRESS;

/* This is initialized in fmi_api_t::init, which is called before the C runtime environment is initialized. */
/*LDRA_INSPECTED 219 S *//*LDRA_INSPECTED 57 D *//*LDRA_INSPECTED 57 D */
static fmi_priv_table_t const * R_FMI_FACTORY_FLASH_TABLE BSP_PLACE_IN_SECTION(".noinit");

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief      This function locks FMI registers
 * @retval     Void pointer to FMI record indexed by record type.
 **********************************************************************************************************************/

__STATIC_INLINE fmi_header_t * HW_FMI_RecordLocate (fmi_priv_table_t const * const p_table, fmi_priv_record_enum_t type)
{
    /* Return the address of the header of the record type. */
    return ((fmi_header_t *) &p_table->data[p_table->offset[type]]);
}

/* This table represents the number of bits asserted in any given 7 bit value */
static const uint8_t bit_count[128] =
{
    0U, 1U, 1U, 2U, 1U, 2U, 2U, 3U, 1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U,
    1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U, 2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U,
    1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U, 2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U,
    2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U, 3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U,
    1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U, 2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U,
    2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U, 3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U,
    2U, 3U, 3U, 4U, 3U, 4U, 4U, 5U, 3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U,
    3U, 4U, 4U, 5U, 4U, 5U, 5U, 6U, 4U, 5U, 5U, 6U, 5U, 6U, 6U, 7U,
};

static const uint8_t bit_count_mask[8] =
{
    0x00U, 0x01U, 0x03U, 0x07U, 0x0FU, 0x1FU, 0x3FU, 0x7FU
};

/*******************************************************************************************************************//**
 * @brief      This function returns pointer to locator information about the IP.
 *
 * @return     If IP is not present, return NULL. If IP is present return pointer to structure that indicates type and
 *             table offset.
 **********************************************************************************************************************/
__STATIC_INLINE fmi_priv_ip_t * HW_FMI_IPLocate (ssp_ip_t           ip,
                                                 fmi_priv_format_t * p_fmi_format,
                                                 uint32_t          * p_fmi_compact_base)
{
    fmi_priv_peripheral_record_t const * const p_ip_record =  (fmi_priv_peripheral_record_t *) HW_FMI_RecordLocate(
        R_FMI_TABLE, SSP_IP_INFORMATION);
    /* Check to make sure IP type can exist within the record. */
    if (ip >= p_ip_record->header.count)
    {
        return (NULL);
    }

    *p_fmi_compact_base = p_ip_record->base;
    /* Select correct map value based on IP value (16 bits/map entry). */
    fmi_priv_peripheral_map_t const * const p_map  = &p_ip_record->map[ip / FMI_PRIV_BITS_PER_MAP_ENTRY];
    /* Determine value to use to parse this map entry. */
    uint32_t                                ip_bit = ip % FMI_PRIV_BITS_PER_MAP_ENTRY;
    /* Read in map entry mask value for parsing. */
    uint16_t                                mask   = p_map->mask;
    /* Check if requested IP available in this device (mask bit will be set if available). */
    if (0U == (mask & (1U << ip_bit)))
    {
        return (NULL);
    }

    /* IP is available, now we have to locate its information */

    /* First account for number of prior map IP entries.  */
    uint32_t index = p_map->prior;
    /* If IP is in upper side of this map entry, account for lower side. */
    if (ip_bit >= FMI_PRIV_HALF_BITS_PER_MAP_ENTRY)
    {
        /* Adjust bit location and mask. */
        ip_bit %= FMI_PRIV_HALF_BITS_PER_MAP_ENTRY;
        mask  >>= FMI_PRIV_HALF_BITS_PER_MAP_ENTRY;
        /* Add in bit count of lower byte. */
        index  += p_map->lower;
    }

    /* Add in bits for present IP that is less than this IP.  */
    index += bit_count[bit_count_mask[ip_bit] & mask];

    /* List of IP pointer records follows the map (actual map size). */
    fmi_priv_peripheral_locator_t const * const p_ip_locator =
        (fmi_priv_peripheral_locator_t *) &p_ip_record->map[(p_ip_record->header.count) / FMI_PRIV_BITS_PER_MAP_ENTRY];
    /* Point to the actual ip information type. */
    fmi_priv_ip_t                               * p_ip =
        (fmi_priv_ip_t *) &R_FMI_TABLE->data[p_ip_locator[index].offset];
    if (0 == p_ip_locator[index].verbose)
    {
        *p_fmi_format = FMI_PRIV_FORMAT_COMPACT;
    }
    else
    {
        *p_fmi_format = (fmi_priv_format_t) p_ip->verbose.format;
    }

    return (p_ip);
}

/*******************************************************************************************************************//**
 * @brief      This function provides the interrupt and event number associated with a particular feature and signal.
 *
 * @param[in]  feature  Pointer to feature definition
 * @param[in]  signal   Signal associated with IRQ and event
 * @param[out] p_irq    IRQ number stored here if call is successful
 * @param[out] p_event  ELC event number stored here if call is successful
 *
 * @retval      SSP_SUCCESS                Event information successfully stored in p_irq and p_event
 * @retval      SSP_ERR_IRQ_BSP_DISABLED   Event information could not be found.  *p_irq is set to SSP_INVALID_VECTOR
 *                                         and *p_event is set to 0xFF.
 **********************************************************************************************************************/
static ssp_err_t HW_FMI_EventGet (ssp_feature_t const * const feature, ssp_signal_t signal,
                                  IRQn_Type * p_irq, elc_event_t * p_event)
{
    /* These variables are defined in bsp_irq.c. */
    extern ssp_vector_info_t * const gp_vector_information;
    extern uint32_t                  g_vector_information_size;
    for (uint32_t i = 0U; i < g_vector_information_size; i++)
    {
        if (gp_vector_information[i].ip_id == feature->id)
        {
            if (gp_vector_information[i].ip_channel == feature->channel)
            {
                if (gp_vector_information[i].ip_unit == feature->unit)
                {
                    if (gp_vector_information[i].ip_signal == signal)
                    {
                        *p_irq   = (IRQn_Type) i;
                        *p_event = (elc_event_t) gp_vector_information[i].event_number;
                        return SSP_SUCCESS;
                    }
                }
            }
        }
    }

    *p_irq   = (IRQn_Type) SSP_INVALID_VECTOR;
    *p_event = (elc_event_t) 0xFF;

    return SSP_ERR_IRQ_BSP_DISABLED;
}
/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /* HW_FMI_COMMON_H */
