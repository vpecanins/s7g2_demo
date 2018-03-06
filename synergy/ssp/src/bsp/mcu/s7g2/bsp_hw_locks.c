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
/***********************************************************************************************************************
* File Name    : bsp_hw_locks.c
* Description  : Defines BSP hardware locks available on this MCU.
***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @ingroup BSP_MCU_S7G2
 * @defgroup BSP_MCU_HW_LOCKS Hardware Locks
 *
 * This file allocates hardware locks used in @ref BSP_MCU_LOCKING.
 *
 * @{
 **********************************************************************************************************************/



/***********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
***********************************************************************************************************************/
#include "bsp_api.h"

#if defined(BSP_MCU_GROUP_S7G2)

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Exported global variables (to be accessed by other files)
***********************************************************************************************************************/

/***********************************************************************************************************************
Private global variables and functions
***********************************************************************************************************************/
#if defined(__GNUC__)
/* This structure is affected by warnings from a GCC compiler bug related to initializing anonymous structures.
 * This pragma suppresses the warnings in this structure only, and will be removed when the SSP compiler is updated to
 * v5.3.*/
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

/** Used to allocated hardware locks.  Parameters are as follows:
 *    1. IP name (ssp_ip_t enum without the SSP_IP_ prefix).
 *    2. Unit number (used for blocks with variations like USB, not to be confused with ADC unit).
 *    3. Channel number
 */

/* The variables defined by the following macros are never referenced by name. They are accessed as a list and index by the BSP Hardware locking functions.
 * Defining them as static, as requested by LDRA, results in both an additional LDRA warning that the variable in not initialized, as well as compiler warnings
 * that the variable is defined but not referenced. That warning would need to be suppressed as it would otherwise generate warnings in user projects.
 */
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 219 S - This is an allowed exception to LDRA standard 219 S "User name starts with underscore."*/
SSP_HW_LOCK_DEFINE(ADC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ADC, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(AGT, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(AGT, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(BSC, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(CAC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(CAN, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(CAN, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(COMP_HS, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(COMP_HS, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(COMP_HS, 0U, 2U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(COMP_HS, 0U, 3U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(COMP_HS, 0U, 4U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(COMP_HS, 0U, 5U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(CRC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(CTSU, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DAAD, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DAC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DAC, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DOC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DMAC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DMAC, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DMAC, 0U, 2U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DMAC, 0U, 3U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DMAC, 0U, 4U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DMAC, 0U, 5U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DMAC, 0U, 6U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DMAC, 0U, 7U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DRW, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(DTC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ELC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(EPTPC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ETHER, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ETHER, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(FCU, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GLCDC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 2U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 3U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 4U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 5U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 6U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 7U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 8U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 9U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 10U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 11U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 12U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(GPT, 0U, 13U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 2U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 3U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 4U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 5U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 6U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 7U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 8U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 9U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 10U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 11U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 12U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 13U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 14U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(ICU, 0U, 15U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(IIC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(IIC, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(IIC, 0U, 2U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(IRDA, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(IWDT, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(JPEG, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(KEY, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(LPM, 1U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(LVD, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(LVD, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(MMF, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(MPU, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(OPS, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(PDC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(POEG, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(POEG, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(POEG, 0U, 2U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(POEG, 0U, 3U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(QSPI, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SPI, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SPI, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(RTC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCE, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 2U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 3U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 4U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 5U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 6U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 7U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 8U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SCI, 0U, 9U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SRC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SSI, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SSI, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SDHIMMC, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(SDHIMMC, 0U, 1U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(TSN, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(USB, 0U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(USB, 1U, 0U);
/*LDRA_INSPECTED 27 D */
/*LDRA_INSPECTED 27 D */
SSP_HW_LOCK_DEFINE(WDT, 0U, 0U);

#if defined(__GNUC__)
/* Restore warning settings for 'missing-field-initializers' to as specified on command line. */
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic pop
#endif

#endif

/** @} (end defgroup BSP_MCU_HW_LOCKS) */
