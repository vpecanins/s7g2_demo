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
 * File Name    : hw_cgc_private.h
 * Description  : hw_cgc Private header file.
 **********************************************************************************************************************/

#ifndef HW_CGC_PRIVATE_H
#define HW_CGC_PRIVATE_H
#include "r_cgc_api.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define CGC_OPCCR_HIGH_SPEED_MODE   (0U)
#define CGC_OPCCR_MIDDLE_SPEED_MODE (1U)
#define CGC_OPCCR_LOW_VOLTAGE_MODE (2U)
#define CGC_OPCCR_LOW_SPEED_MODE (3U)
#define CGC_SOPCCR_SET_SUBOSC_SPEED_MODE (1U)
#define CGC_SOPCCR_CLEAR_SUBOSC_SPEED_MODE (0U)
#define CGC_OPPCR_OPCM_MASK (0x3U)
#define CGC_SOPPCR_SOPCM_MASK (0x1U)

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** Main oscillator source identifier. */
typedef enum e_cgc_osc_source
{
    CGC_OSC_SOURCE_RESONATOR,       ///< A resonator is used as the source of the main oscillator.
    CGC_OSC_SOURCE_EXTERNAL_OSC     ///< An external oscillator is used as the source of the main oscillator.
} cgc_osc_source_t;

/** Operating power control modes. */
typedef enum
{
    CGC_HIGH_SPEED_MODE     = 0U,   // Should match OPCCR OPCM high speed
    CGC_MIDDLE_SPEED_MODE   = 1U,   // Should match OPCCR OPCM middle speed
    CGC_LOW_VOLTAGE_MODE    = 2U,   // Should match OPCCR OPCM low voltage
    CGC_LOW_SPEED_MODE      = 3U,   // Should match OPCCR OPCM low speed
    CGC_SUBOSC_SPEED_MODE   = 4U,   // Can be any value not otherwise used
}cgc_operating_modes_t;

/**********************************************************************************************************************
* Functions
 **********************************************************************************************************************/
void          HW_CGC_Init (void);

void          HW_CGC_InitPLLFreq (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_LPMHardwareLock (void);

void          HW_CGC_LPMHardwareUnLock (void);

void          HW_CGC_HardwareLock (void);

void          HW_CGC_HardwareUnLock (void);

void          HW_CGC_MainClockDriveSet (R_SYSTEM_Type * p_system_reg, uint8_t val);                   // Set the drive capacity for the main clock.

void          HW_CGC_SubClockDriveSet (R_SYSTEM_Type * p_system_reg, uint8_t val);                    // Set the drive capacity for the subclock.

void          HW_CGC_ClockStart (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock);                    // Start selected clock.

void          HW_CGC_ClockStop (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock);                     //  Stop selected clock.

bool          HW_CGC_ClockCheck (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock);                    //  Check for stability of selected clock.

bool          HW_CGC_ClockRunStateGet (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock);              // Get clock run state.

void          HW_CGC_DelayCycles (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock, uint16_t cycles);  // delay for number of clock cycles specified

void          HW_CGC_MainOscSourceSet (R_SYSTEM_Type * p_system_reg, cgc_osc_source_t osc);           // Set the source to resonator or external osc.

void          HW_CGC_ClockWaitSet (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock, uint8_t time);    // Set the clock wait time

bool          HW_CGC_ClockSourceValidCheck (cgc_clock_t clock);         // Check for valid clock source

bool          HW_CGC_SystemClockValidCheck (cgc_system_clocks_t clock); // Check for valid system clock

cgc_clock_t   HW_CGC_ClockSourceGet (R_SYSTEM_Type * p_system_reg);                             // Get the system clock source

void          HW_CGC_ClockSourceSet (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock);                // Set the system clock source

cgc_clock_t   HW_CGC_PLLClockSourceGet (R_SYSTEM_Type * p_system_reg);                          // Get the PLL clock source

void          HW_CGC_PLLClockSourceSet (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock);             // Set the PLL clock source

/*LDRA_INSPECTED 90 s float used because float32_t is not part of the C99 standard integer definitions. */
void          HW_CGC_PLLMultiplierSet (R_SYSTEM_Type * p_system_reg, float multiplier);

/*LDRA_INSPECTED 90 s float used because float32_t is not part of the C99 standard integer definitions. */
float         HW_CGC_PLLMultiplierGet (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_PLLDividerSet (R_SYSTEM_Type * p_system_reg, cgc_pll_div_t divider);

uint16_t      HW_CGC_PLLDividerGet (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_SystemDividersSet (R_SYSTEM_Type * p_system_reg, cgc_system_clock_cfg_t const * const cfg);  // Set the system dividers

void          HW_CGC_SystemDividersGet (R_SYSTEM_Type * p_system_reg, cgc_system_clock_cfg_t * cfg);  // Get the system dividers

bool          HW_CGC_ClockCfgValidCheck (cgc_clock_cfg_t * cfg);        // check for valid configuration

uint32_t      HW_CGC_ClockDividerGet (R_SYSTEM_Type * p_system_reg, cgc_system_clocks_t clock);

uint32_t      HW_CGC_ClockHzGet (R_SYSTEM_Type * p_system_reg, cgc_system_clocks_t clock);            // get frequency of selected clock

uint32_t      HW_CGC_ClockHzCalculate (cgc_clock_t source_clock,  cgc_sys_clock_div_t divider);

void          HW_CGC_MemWaitSet (R_SYSTEM_Type * p_system_reg, uint32_t setting);

uint32_t      HW_CGC_MemWaitGet (R_SYSTEM_Type * p_system_reg);

cgc_operating_modes_t HW_CGC_OperatingModeGet (R_SYSTEM_Type * p_system_reg);

__STATIC_INLINE void HW_CGC_SetHighSpeedMode (R_SYSTEM_Type * p_system_reg);

__STATIC_INLINE void HW_CGC_SetMiddleSpeedMode (R_SYSTEM_Type * p_system_reg);

__STATIC_INLINE void HW_CGC_SetLowSpeedMode (R_SYSTEM_Type * p_system_reg);

__STATIC_INLINE void HW_CGC_SetSubOscSpeedMode (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_OscStopDetectEnable (R_SYSTEM_Type * p_system_reg);            // enable hardware oscillator stop detect function

void          HW_CGC_OscStopDetectDisable (R_SYSTEM_Type * p_system_reg);           // disable hardware oscillator stop detect function

bool          HW_CGC_OscStopDetectEnabledGet (R_SYSTEM_Type * p_system_reg);        // status of hardware oscillator stop detect enabled

bool          HW_CGC_OscStopDetectGet (R_SYSTEM_Type * p_system_reg);               // oscillator stop detected

bool          HW_CGC_OscStopStatusClear (R_SYSTEM_Type * p_system_reg);             // clear hardware oscillator stop detect status

void          HW_CGC_BusClockOutCfg (R_SYSTEM_Type * p_system_reg, cgc_bclockout_dividers_t divider);

void          HW_CGC_BusClockOutEnable (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_BusClockOutDisable (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_ClockOutCfg (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock, cgc_clockout_dividers_t divider);

void          HW_CGC_ClockOutEnable (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_ClockOutDisable (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_LCDClockCfg (R_SYSTEM_Type * p_system_reg, uint8_t clock);

uint8_t       HW_CGC_LCDClockCfgGet (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_LCDClockEnable (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_LCDClockDisable (R_SYSTEM_Type * p_system_reg);

bool          HW_CGC_LCDClockIsEnabled (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_SDRAMClockOutEnable (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_SDRAMClockOutDisable (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_USBClockCfg (R_SYSTEM_Type * p_system_reg, cgc_usb_clock_div_t divider);

void          HW_CGC_SRAM_ProtectLock (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_SRAM_ProtectUnLock (R_SYSTEM_Type * p_system_reg);

void          HW_CGC_SRAM_RAMWaitSet (R_SYSTEM_Type * p_system_reg, uint32_t setting);

void          HW_CGC_SRAM_HSRAMWaitSet (R_SYSTEM_Type * p_system_reg, uint32_t setting);

void          HW_CGC_ROMWaitSet (R_SYSTEM_Type * p_system_reg, uint32_t setting);

uint32_t      HW_CGC_ROMWaitGet (R_SYSTEM_Type * p_system_reg);

bool          HW_CGC_SystickUpdate(uint32_t ticks_per_second);

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /* HW_CGC_PRIVATE_H */
