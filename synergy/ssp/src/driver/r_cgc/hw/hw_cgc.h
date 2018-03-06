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
 * File Name    : hw_cgc.h
 * Description  : Header file for hw_cgc.c
 **********************************************************************************************************************/
#ifndef HW_CGC_H
#define HW_CGC_H

#include "bsp_cfg.h"
#include "bsp_clock_cfg.h"
#include "hw_cgc_private.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/** PRC0 mask */
#define CGC_PRC0_MASK             ((uint16_t) 0x0001)
/** PRC1 mask */
#define CGC_PRC1_MASK             ((uint16_t) 0x0002)
/** Key code for writing PRCR register. */
#define CGC_PRCR_KEY              ((uint16_t) 0xA500)
#define CGC_PLL_MAIN_OSC          (0x00U)            ///< PLL clock source register value for the main oscillator
#define CGC_PLL_HOCO              (0x01U)            ///< PLL clock source register value for the HOCO

#define CGC_LOCO_FREQ             (32768U)           ///< LOCO frequency is fixed at 32768 Hz
#define CGC_MOCO_FREQ             (8000000U)         ///< MOCO frequency is fixed at 8 MHz
#define CGC_SUBCLOCK_FREQ         (32768U)           ///< Subclock frequency is 32768 Hz
#define CGC_PLL_FREQ              (0U)               ///< The PLL frequency must be calculated.
#define CGC_IWDT_FREQ             (15000U)           ///< The IWDT frequency is fixed at 15 kHz
#define CGC_S124_LOW_V_MODE_FREQ  (4000000U)         ///< Max ICLK frequency while in Low Voltage Mode for S124

#define CGC_MAX_ZERO_WAIT_FREQ    (32000000U)
#define CGC_MAX_MIDDLE_SPEED_FREQ (12000000U)        ///< Maximum frequency for Middle Speed mode

#define    CGC_CLOCKOUT_MAX       (CGC_CLOCK_SUBCLOCK)     ///< Highest enum value for CLKOUT clock source

#define    CGC_SYSTEMS_CLOCKS_MAX (CGC_SYSTEM_CLOCKS_ICLK) ///< Highest enum value for system clock

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

typedef union
{
    __IO uint32_t  sckdivcr_w; /*!< (@ 0x4001E020) System clock Division control register
                                *                */
    /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
    struct
    {
        __IO uint32_t  pckd : 3; /*!< [0..2] Peripheral Module Clock D (PCLKD) Select
                                  *                      */
        uint32_t            : 1;
        __IO uint32_t  pckc : 3; /*!< [4..6] Peripheral Module Clock C (PCLKC) Select
                                  *                      */
        uint32_t            : 1;
        __IO uint32_t  pckb : 3; /*!< [8..10] Peripheral Module Clock B (PCLKB) Select
                                  *                     */
        uint32_t            : 1;
        __IO uint32_t  pcka : 3; /*!< [12..14] Peripheral Module Clock A (PCLKA) Select
                                  *                    */
        uint32_t            : 1;
        __IO uint32_t  bck  : 3; /*!< [16..18] External Bus Clock (BCLK) Select
                                  *                            */
        uint32_t            : 5;
        __IO uint32_t  ick  : 3; /*!< [24..26] System Clock (ICLK) Select
                                  *                                  */
        uint32_t            : 1;
        __IO uint32_t  fck  : 3; /*!< [28..30] Flash IF Clock (FCLK) Select
                                  *                                */
    }  sckdivcr_b;               /*!< [31] BitSize
                                  *                                                         */
} sckdivcr_clone_t;

extern bsp_feature_cgc_t const * gp_cgc_feature; //TODO remove this

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER
/**********************************************************************************************************************
* Function Prototypes
 **********************************************************************************************************************/
void HW_CGC_OperatingModeSet (R_SYSTEM_Type * p_system_reg, cgc_operating_modes_t operating_mode);

/**********************************************************************************************************************
* Inline Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief      This function places the MCU in High Speed Mode
 * @param[in]  none
 * @retval     None
 **********************************************************************************************************************/
__STATIC_INLINE void HW_CGC_SetHighSpeedMode (R_SYSTEM_Type * p_system_reg)
{
    HW_CGC_OperatingModeSet(p_system_reg, CGC_HIGH_SPEED_MODE);
}

/*******************************************************************************************************************//**
 * @brief      This function places the MCU in Middle Speed Mode
 * @param[in]  none
 * @retval     None
 **********************************************************************************************************************/

__STATIC_INLINE void HW_CGC_SetMiddleSpeedMode (R_SYSTEM_Type * p_system_reg)
{
    HW_CGC_OperatingModeSet(p_system_reg, CGC_MIDDLE_SPEED_MODE);
}

/*******************************************************************************************************************//**
 * @brief      This function places the MCU in Low Voltage Mode
 * @param[in]  none
 * @retval     None
 **********************************************************************************************************************/

__STATIC_INLINE void HW_CGC_SetLowVoltageMode (R_SYSTEM_Type * p_system_reg)
{
    HW_CGC_OperatingModeSet(p_system_reg, CGC_LOW_VOLTAGE_MODE);
}

/*******************************************************************************************************************//**
 * @brief      This function places the MCU in Low Speed Mode
 * @param[in]  none
 * @retval     None
 **********************************************************************************************************************/

__STATIC_INLINE void HW_CGC_SetLowSpeedMode (R_SYSTEM_Type * p_system_reg)
{
    HW_CGC_OperatingModeSet(p_system_reg, CGC_LOW_SPEED_MODE);
}

/*******************************************************************************************************************//**
 * @brief      This function places the MCU in Sub-Osc Speed Mode
 * @param[in]  none
 * @retval     None
 **********************************************************************************************************************/

__STATIC_INLINE void HW_CGC_SetSubOscSpeedMode (R_SYSTEM_Type * p_system_reg)
{
    HW_CGC_OperatingModeSet(p_system_reg, CGC_SUBOSC_SPEED_MODE);
}

#endif // ifndef HW_CGC_H
