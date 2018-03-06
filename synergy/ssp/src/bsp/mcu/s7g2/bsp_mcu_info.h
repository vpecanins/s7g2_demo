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
* File Name    : bsp_mcu_info.h
* Description  : Information about the MCU on this board
***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @ingroup BSP_MCUs
 * @defgroup BSP_MCU_S7G2 S7G2
 * @brief Code that is common to S7G2 MCUs.
 *
 * Implements functions that are common to S7G2 MCUs.
 *
 * @{
***********************************************************************************************************************/

/** @} (end defgroup BSP_MCU_S7G2) */

#ifndef BSP_MCU_INFO_H_
#define BSP_MCU_INFO_H_

/***********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
***********************************************************************************************************************/
#if defined(__GNUC__)
/* CMSIS-CORE currently generates 2 warnings when compiling with GCC. One in core_cmInstr.h and one in core_cm4_simd.h.
 * We are not modifying these files so we will ignore these warnings temporarily. */
/*LDRA_INSPECTED 69 s */
#pragma GCC diagnostic ignored "-Wconversion"
/*LDRA_INSPECTED 69 s */
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

/* The following files are provided by CMSIS, so violations are not reported during static analysis. */
/*LDRA_NOANALYSIS */

/* CMSIS-CORE Renesas Device Files. */
#include "../../src/bsp/cmsis/Device/RENESAS/S7G2/Include/S7G2.h"

/* Static analysis resumes as normal after CMSIS files. */
/*LDRA_ANALYSIS */

#if defined(__GNUC__)
/* Restore warning settings for 'conversion' and 'sign-conversion' to as specified on command line. */
/*LDRA_INSPECTED 69 s */
#pragma GCC diagnostic pop
#endif

/* BSP MCU Specific Includes. */
#include "bsp_elc.h"
#include "bsp_cache.h"


/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Exported global variables
***********************************************************************************************************************/

/***********************************************************************************************************************
Exported global functions (to be accessed by other files)
***********************************************************************************************************************/

#endif /* BSP_MCU_INFO_H_ */


