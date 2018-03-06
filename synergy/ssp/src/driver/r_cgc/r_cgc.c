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
 * File Name    : r_cgc.c
 * Description  : HAL driver for the Clock Generation circuit.
 **********************************************************************************************************************/


/***********************************************************************************************************************
* Includes
 **********************************************************************************************************************/
#include "r_cgc.h"
#include "r_cgc_private.h"
#include "r_cgc_private_api.h"
#include "./hw/hw_cgc_private.h"
/* Configuration for this package. */
#include "r_cgc_cfg.h"
#include "./hw/hw_cgc.h"

#if (1 == BSP_CFG_RTOS)
#include "tx_api.h"
#endif
/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** Macro for error logger. */
#ifndef CGC_ERROR_RETURN
/*LDRA_INSPECTED 77 S This macro does not work when surrounded by parentheses. */
#define CGC_ERROR_RETURN(a, err) SSP_ERROR_RETURN((a), (err), g_module_name, &g_cgc_version)
#endif

#define CGC_CLOCK_NUM_CLOCKS    ((uint8_t) CGC_CLOCK_PLL + 1U)

#define CGC_LDC_INVALID_CLOCK   (0xFFU)

#define CGC_LCD_CFG_TIMEOUT     (0xFFFFFU)

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static ssp_err_t r_cgc_clock_start_stop(cgc_clock_change_t clock_state, cgc_clock_t clock_to_change);
static ssp_err_t r_cgc_stabilization_wait(cgc_clock_t clock);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
#if defined(__GNUC__)
/* This structure is affected by warnings from a GCC compiler bug. This pragma suppresses the warnings in this
 * structure only.*/
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
/** Version data structure used by error logger macro. */
static const ssp_version_t g_cgc_version =
{
    .api_version_minor  = CGC_API_VERSION_MINOR,
    .api_version_major  = CGC_API_VERSION_MAJOR,
    .code_version_major = CGC_CODE_VERSION_MAJOR,
    .code_version_minor = CGC_CODE_VERSION_MINOR
};
#if defined(__GNUC__)
/* Restore warning settings for 'missing-field-initializers' to as specified on command line. */
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic pop
#endif

/** Name of module used by error logger macro */
#if BSP_CFG_ERROR_LOG != 0
static const char g_module_name[] = "cgc";
#endif
/*LDRA_NOANALYSIS LDRA_INSPECTED below not working. */
/* This is initialized in cgc_api_t::init, which is called before the C runtime environment is initialized. */
/*LDRA_INSPECTED 219 S */
bsp_feature_cgc_t const * gp_cgc_feature BSP_PLACE_IN_SECTION(".noinit");
/*LDRA_ANALYSIS */

/** Pointer to CGC base register. */
/* This is initialized in ioport_api_t::init, which is called before the C runtime environment is initialized. */
/*LDRA_INSPECTED 219 S */
static R_SYSTEM_Type * gp_system_reg BSP_PLACE_IN_SECTION(".noinit");

/** LCD clock selection register values. */
static const uint8_t g_lcd_clock_settings[] =
{
    [CGC_CLOCK_HOCO]      = 0x04U,     ///< The high speed on chip oscillator.
    [CGC_CLOCK_MOCO]      = CGC_LDC_INVALID_CLOCK,
    [CGC_CLOCK_LOCO]      = 0x00U,     ///< The low speed on chip oscillator.
    [CGC_CLOCK_MAIN_OSC]  = 0x02U,     ///< The main oscillator.
    [CGC_CLOCK_SUBCLOCK]  = 0x01U,     ///< The subclock oscillator.
    [CGC_CLOCK_PLL]       = CGC_LDC_INVALID_CLOCK,
};

/***********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
/*LDRA_INSPECTED 27 D This structure must be accessible in user code. It cannot be static. */
const cgc_api_t g_cgc_on_cgc =
{
    .init                 = R_CGC_Init,
    .clocksCfg            = R_CGC_ClocksCfg,
    .clockStart           = R_CGC_ClockStart,
    .clockStop            = R_CGC_ClockStop,
    .systemClockSet       = R_CGC_SystemClockSet,
    .systemClockGet       = R_CGC_SystemClockGet,
    .systemClockFreqGet   = R_CGC_SystemClockFreqGet,
    .clockCheck           = R_CGC_ClockCheck,
    .oscStopDetect        = R_CGC_OscStopDetect,
    .oscStopStatusClear   = R_CGC_OscStopStatusClear,
    .busClockOutCfg       = R_CGC_BusClockOutCfg,
    .busClockOutEnable    = R_CGC_BusClockOutEnable,
    .busClockOutDisable   = R_CGC_BusClockOutDisable,
    .clockOutCfg          = R_CGC_ClockOutCfg,
    .clockOutEnable       = R_CGC_ClockOutEnable,
    .clockOutDisable      = R_CGC_ClockOutDisable,
    .lcdClockCfg          = R_CGC_LCDClockCfg,
    .lcdClockEnable       = R_CGC_LCDClockEnable,
    .lcdClockDisable      = R_CGC_LCDClockDisable,
    .sdramClockOutEnable  = R_CGC_SDRAMClockOutEnable,
    .sdramClockOutDisable = R_CGC_SDRAMClockOutDisable,
    .usbClockCfg          = R_CGC_USBClockCfg,
    .systickUpdate        = R_CGC_SystickUpdate,
    .versionGet           = R_CGC_VersionGet
};

/*******************************************************************************************************************//**
 * @ingroup HAL_Library
 * @addtogroup CGC
 * @brief Clock Generation Circuit API
 *
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief  Initialize the CGC API.
 *
 *                Configures the following for the clock generator module
 *                - SubClock drive capacity (Compile time configurable: CGC_CFG_SUBCLOCK_DRIVE)
 *                - Initial setting for the SubClock
 *
 *                THIS FUNCTION MUST BE EXECUTED ONCE AT STARTUP BEFORE ANY OF THE OTHER CGC FUNCTIONS
 *                CAN BE USED OR THE CLOCK SOURCE IS CHANGED FROM THE MOCO.
 * @retval SSP_SUCCESS                  Clock initialized successfully.
 * @retval SSP_ERR_HARDWARE_TIMEOUT     Hardware timed out.
 **********************************************************************************************************************/
ssp_err_t R_CGC_Init (void)
{
    /* Initialize MCU specific feature pointer. */
    R_BSP_FeatureCgcGet(&gp_cgc_feature);

    ssp_feature_t ssp_feature= {{(ssp_ip_t) 0U}};
    fmi_feature_info_t info = {0U};
    ssp_feature.channel = 0U;
    ssp_feature.unit = 0U;
    ssp_feature.id = SSP_IP_CGC;
    g_fmi_on_fmi.productFeatureGet(&ssp_feature, &info);
    gp_system_reg = (R_SYSTEM_Type *) info.ptr;

    volatile uint32_t timeout;
    timeout = MAX_REGISTER_WAIT_COUNT;
    HW_CGC_Init();                              // initialize hardware functions
    HW_CGC_ClockStop(gp_system_reg, CGC_CLOCK_SUBCLOCK);       // stop SubClock
    while ((HW_CGC_ClockRunStateGet(gp_system_reg, CGC_CLOCK_SUBCLOCK)) && (0U != timeout))
    {
        /* wait until the clock state is stopped */
        timeout--;                                          // but timeout, if it never happens
    }

    CGC_ERROR_RETURN(timeout, SSP_ERR_HARDWARE_TIMEOUT);    // return error if timed out

    HW_CGC_DelayCycles(gp_system_reg, CGC_CLOCK_SUBCLOCK, SUBCLOCK_DELAY); // Delay for 5 SubClock cycles.
    HW_CGC_SubClockDriveSet(gp_system_reg, CGC_CFG_SUBCLOCK_DRIVE);        // set the SubClock drive according to the configuration

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Reconfigure all main system clocks.
 *
 * @retval SSP_SUCCESS                  Clock initialized successfully.
 * @retval SSP_ERR_INVALID_ARGUMENT     Invalid argument used.
 * @retval SSP_ERR_MAIN_OCO_INACTIVE    PLL Initialization attempted with Main OCO turned off/unstable.
 * @retval SSP_ERR_CLOCK_ACTIVE         Active clock source specified for modification. This applies specifically to the
 *                                      PLL dividers/multipliers which cannot be modified if the PLL is active. It has
 *                                      to be stopped first before modification.
 * @retval SSP_ERR_NOT_STABILIZED       The Clock source is not stabilized after being turned off.
 * @retval SSP_ERR_CLKOUT_EXCEEDED      The main oscillator can be only 8 or 16 MHz.
 * @retval SSP_ERR_NULL_PTR             A NULL is passed for configuration data when PLL is the clock_source.
 * @retval SSP_ERR_INVALID_MODE         Attempt to start a clock in a restricted operating power control mode.
 *
 **********************************************************************************************************************/
ssp_err_t R_CGC_ClocksCfg(cgc_clocks_cfg_t const * const p_clock_cfg)
{
    ssp_err_t err = SSP_SUCCESS;
    cgc_clock_t requested_system_clock = p_clock_cfg->system_clock;
    cgc_clock_cfg_t * p_pll_cfg = (cgc_clock_cfg_t *)&(p_clock_cfg->pll_cfg);
    cgc_system_clock_cfg_t sys_cfg = {
        .pclka_div = CGC_SYS_CLOCK_DIV_1,
        .pclkb_div = CGC_SYS_CLOCK_DIV_1,
        .pclkc_div = CGC_SYS_CLOCK_DIV_1,
        .pclkd_div = CGC_SYS_CLOCK_DIV_1,
        .bclk_div = CGC_SYS_CLOCK_DIV_1,
        .fclk_div = CGC_SYS_CLOCK_DIV_1,
        .iclk_div = CGC_SYS_CLOCK_DIV_1,
    };
    cgc_clock_t current_system_clock = CGC_CLOCK_HOCO;
    g_cgc_on_cgc.systemClockGet(&current_system_clock, &sys_cfg);

    cgc_clock_change_t options[CGC_CLOCK_NUM_CLOCKS];
    options[CGC_CLOCK_HOCO] = p_clock_cfg->hoco_state;
    options[CGC_CLOCK_LOCO] = p_clock_cfg->loco_state;
    options[CGC_CLOCK_MOCO] = p_clock_cfg->moco_state;
    options[CGC_CLOCK_MAIN_OSC] = p_clock_cfg->mainosc_state;
    options[CGC_CLOCK_SUBCLOCK] = p_clock_cfg->subosc_state;
    options[CGC_CLOCK_PLL] = p_clock_cfg->pll_state;

#if CGC_CFG_PARAM_CHECKING_ENABLE
    CGC_ERROR_RETURN(HW_CGC_ClockSourceValidCheck(requested_system_clock), SSP_ERR_INVALID_ARGUMENT);
    CGC_ERROR_RETURN(CGC_CLOCK_CHANGE_STOP != options[p_clock_cfg->system_clock], SSP_ERR_INVALID_ARGUMENT);
    if (CGC_CLOCK_CHANGE_START == options[CGC_CLOCK_PLL])
    {
        CGC_ERROR_RETURN(HW_CGC_ClockSourceValidCheck(p_pll_cfg->source_clock), SSP_ERR_INVALID_ARGUMENT);
        CGC_ERROR_RETURN(CGC_CLOCK_CHANGE_STOP != options[p_pll_cfg->source_clock], SSP_ERR_INVALID_ARGUMENT);
    }
#endif /* CGC_CFG_PARAM_CHECKING_ENABLE */

    uint32_t max_clock = CGC_CLOCK_NUM_CLOCKS;
    if (0U == gp_cgc_feature->pllccr_type)
    {
        max_clock = CGC_CLOCK_NUM_CLOCKS - 1U;
    }

    for (uint32_t i = 0U; i < max_clock; i++)
    {
        if (((cgc_clock_t) i == CGC_CLOCK_PLL) && (CGC_CLOCK_CHANGE_START == options[i]))
        {
            /* Need to start PLL source clock and let it stabilize before starting PLL */
            err = g_cgc_on_cgc.clockStart(p_pll_cfg->source_clock, NULL);
            CGC_ERROR_RETURN((SSP_SUCCESS == err) || (SSP_ERR_CLOCK_ACTIVE == err), err);

            err = r_cgc_stabilization_wait(p_pll_cfg->source_clock);
            CGC_ERROR_RETURN(SSP_SUCCESS == err, err);

            err = g_cgc_on_cgc.clockStart(CGC_CLOCK_PLL, p_pll_cfg);
            CGC_ERROR_RETURN((SSP_SUCCESS == err) || (SSP_ERR_CLOCK_ACTIVE == err), err);
        }
        else
        {
            err = r_cgc_clock_start_stop(options[i], (cgc_clock_t) i);
            CGC_ERROR_RETURN((SSP_SUCCESS == err) || (SSP_ERR_CLOCK_ACTIVE == err), err);
        }
    }

    err = r_cgc_stabilization_wait(requested_system_clock);
    CGC_ERROR_RETURN(SSP_SUCCESS == err, err);

    /* Set which clock to use for system clock and divisors for all system clocks. */
    err = g_cgc_on_cgc.systemClockSet(requested_system_clock, &(p_clock_cfg->sys_cfg));
    CGC_ERROR_RETURN(SSP_SUCCESS == err, err);

    cgc_clock_t previous_system_clock = current_system_clock;

    /* If the system clock has changed, stop previous system clock if requested. */
    if (requested_system_clock != previous_system_clock)
    {
        if (CGC_CLOCK_CHANGE_STOP == options[previous_system_clock])
        {
            err = r_cgc_clock_start_stop(options[previous_system_clock], previous_system_clock);
            CGC_ERROR_RETURN(SSP_SUCCESS == err, err);
        }

        /* If the system clock is no longer PLL, try to stop PLL source if requested and no longer used by PLL */
        cgc_clock_t old_pll_source = HW_CGC_PLLClockSourceGet(gp_system_reg);
        if (CGC_CLOCK_CHANGE_STOP == options[old_pll_source])
        {
            if ((CGC_CLOCK_PLL == previous_system_clock) && (requested_system_clock != old_pll_source))

            {
                err = r_cgc_clock_start_stop(options[old_pll_source], old_pll_source);
                CGC_ERROR_RETURN(SSP_SUCCESS == err, err);
            }
        }
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Start the specified clock if it is not currently active.
 *
 *                Configures the following when starting the Main Clock Oscillator:
 *                - MainClock drive capacity (Configured based on external clock frequency)
 *                - MainClock stabilization wait time (Compile time configurable: CGC_CFG_MAIN_OSC_WAIT)
 *
 * @retval SSP_SUCCESS                  Clock initialized successfully.
 * @retval SSP_ERR_INVALID_ARGUMENT     Invalid argument used.
 * @retval SSP_ERR_MAIN_OCO_INACTIVE    PLL Initialization attempted with Main OCO turned off/unstable.
 * @retval SSP_ERR_CLOCK_ACTIVE         Active clock source specified for modification. This applies specifically to the
 *                                      PLL dividers/multipliers which cannot be modified if the PLL is active. It has
 *                                      to be stopped first before modification.
 * @retval SSP_ERR_NOT_STABILIZED       The Clock source is not stabilized after being turned off.
 * @retval SSP_ERR_CLKOUT_EXCEEDED      The main oscillator can be only 8 or 16 MHz.
 * @retval SSP_ERR_NULL_PTR             A NULL is passed for configuration data when PLL is the clock_source.
 * @retval SSP_ERR_INVALID_MODE			Attempt to start a clock in a restricted operating power control mode.
 *
 **********************************************************************************************************************/

ssp_err_t R_CGC_ClockStart (cgc_clock_t clock_source, cgc_clock_cfg_t * p_clock_cfg)
{
    /* return error if invalid clock source or not supported by hardware */
    CGC_ERROR_RETURN((HW_CGC_ClockSourceValidCheck(clock_source)), SSP_ERR_INVALID_ARGUMENT);
    if (CGC_CLOCK_PLL == clock_source)                          /* It could be that PLL is already running but speed
                                                                 * info was overwritten */
    {
        bool state = HW_CGC_ClockRunStateGet(gp_system_reg, clock_source);
        if (state)
        {
            HW_CGC_InitPLLFreq(gp_system_reg);                                /* calculate  PLL clock frequency and save it */
        }
        CGC_ERROR_RETURN(false == state, SSP_ERR_CLOCK_ACTIVE);
    }
    /* return error if clock is already active */
    else
    {
        CGC_ERROR_RETURN(!(HW_CGC_ClockRunStateGet(gp_system_reg, clock_source)), SSP_ERR_CLOCK_ACTIVE);
    }


#if (CGC_CFG_PARAM_CHECKING_ENABLE == 1)
    if (clock_source == CGC_CLOCK_PLL)            // is clock source PLL?
    {
        SSP_ASSERT(NULL != p_clock_cfg);          // return error if NULL pointer to configuration

        /* if current clock is PLL, divider/multiplier it cannot be modified */
        CGC_ERROR_RETURN((CGC_CLOCK_PLL != HW_CGC_ClockSourceGet(gp_system_reg)), SSP_ERR_CLOCK_ACTIVE);

        /* return error if configuration contains illegal parameters */
        CGC_ERROR_RETURN((HW_CGC_ClockCfgValidCheck(p_clock_cfg)), SSP_ERR_INVALID_ARGUMENT);
    }
    else if ((CGC_CLOCK_HOCO == clock_source) || (CGC_CLOCK_MOCO == clock_source)|| (CGC_CLOCK_MAIN_OSC == clock_source) || (CGC_CLOCK_PLL == clock_source))
    {
        CGC_ERROR_RETURN(CGC_SUBOSC_SPEED_MODE != HW_CGC_OperatingModeGet(gp_system_reg), SSP_ERR_INVALID_MODE);
    }
#endif /* if (CGC_CFG_PARAM_CHECKING_ENABLE == 1) */

    /*  if clock source is LOCO, MOCO or SUBCLOCK */
    if (((clock_source == CGC_CLOCK_LOCO) || (clock_source == CGC_CLOCK_MOCO)) || (clock_source == CGC_CLOCK_SUBCLOCK))
    {
        HW_CGC_ClockStart(gp_system_reg, clock_source);       // start the clock
        /* if clock source is subclock */
        if (clock_source == CGC_CLOCK_SUBCLOCK)
        {
            volatile uint32_t timeout;
            timeout = MAX_REGISTER_WAIT_COUNT;
            while ((!(HW_CGC_ClockRunStateGet(gp_system_reg, CGC_CLOCK_SUBCLOCK))) && (0 != timeout))
            {
                /* wait until the clock state is started */
                timeout--;                          // but timeout if it never happens
            }

            CGC_ERROR_RETURN(timeout, SSP_ERR_HARDWARE_TIMEOUT);    // return error if timed out
        }
    }
    else if ((clock_source == CGC_CLOCK_HOCO) || (clock_source == CGC_CLOCK_MAIN_OSC))
    {
        /* make sure the oscillator has stopped before starting again */
        CGC_ERROR_RETURN(!(HW_CGC_ClockCheck(gp_system_reg, clock_source)), SSP_ERR_NOT_STABILIZED);

        if (CGC_CLOCK_MAIN_OSC == clock_source )
        {
            HW_CGC_MainClockDriveSet(gp_system_reg, gp_cgc_feature->mainclock_drive);          /* set the Main Clock drive according to
                                                                     * the configuration */
            HW_CGC_MainOscSourceSet(gp_system_reg, (cgc_osc_source_t) CGC_CFG_MAIN_OSC_CLOCK_SOURCE); /* set the main osc source
                                                                                        * to resonator or
                                                                                        * external osc. */
            HW_CGC_ClockWaitSet(gp_system_reg, CGC_CLOCK_MAIN_OSC, CGC_CFG_MAIN_OSC_WAIT);            /* set the main osc wait time */
        }

        HW_CGC_ClockStart(gp_system_reg, clock_source);       // start the clock
    }

    /*  if clock source is PLL */
    else if (clock_source == CGC_CLOCK_PLL)
    {
        /* if the PLL source clock isn't running, PLL cannot be turned on, return error */
        CGC_ERROR_RETURN(HW_CGC_ClockRunStateGet(gp_system_reg, p_clock_cfg->source_clock), SSP_ERR_MAIN_OSC_INACTIVE);
        /*  make sure the PLL has stopped before starting again */
        CGC_ERROR_RETURN(!(HW_CGC_ClockCheck(gp_system_reg, clock_source)), SSP_ERR_NOT_STABILIZED);

        if (1U == gp_cgc_feature->pll_src_configurable)
        {
            HW_CGC_PLLClockSourceSet(gp_system_reg, p_clock_cfg->source_clock); // configure PLL source clock for S7G2
        }

        HW_CGC_PLLDividerSet(gp_system_reg, p_clock_cfg->divider);          // configure PLL divider
        HW_CGC_PLLMultiplierSet(gp_system_reg, p_clock_cfg->multiplier);    // configure PLL multiplier
        HW_CGC_InitPLLFreq(gp_system_reg);                                // calculate  PLL clock frequency

        /** See if we need to switch to Middle or High Speed mode before starting the PLL. */
        uint32_t requested_frequency = HW_CGC_ClockHzCalculate(clock_source, (cgc_sys_clock_div_t) gp_cgc_feature->iclk_div);
        if (requested_frequency > gp_cgc_feature->middle_speed_max_freq_hz)
        {
            HW_CGC_SetHighSpeedMode(gp_system_reg);
        }
        else
        {
            HW_CGC_SetMiddleSpeedMode(gp_system_reg);    // PLL will only run in High or Middle Speed modes.
        }

        HW_CGC_ClockStart(gp_system_reg, clock_source);                     // start the clock
    }

    else
    {
        /* statement here to follow coding standard */
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Stop the specified clock if it is active and not configured as the system clock.
 * @retval SSP_SUCCESS              Clock stopped successfully.
 * @retval SSP_ERR_CLOCK_ACTIVE     Current System clock source specified for stopping. This is not allowed.
 * @retval SSP_ERR_OSC_DET_ENABLED  MOCO cannot be stopped if Oscillation stop detection is enabled.
 * @retval SSP_ERR_NOT_STABILIZED   Clock not stabilized after starting. A finite stabilization time after starting the
 *                                  clock has to elapse before it can be stopped.
 * @retval SSP_ERR_INVALID_ARGUMENT Invalid argument used.
 **********************************************************************************************************************/

ssp_err_t R_CGC_ClockStop (cgc_clock_t clock_source)
{
    cgc_clock_t current_clock;

    /*  return error if invalid clock source or not supported by hardware */
    CGC_ERROR_RETURN(HW_CGC_ClockSourceValidCheck(clock_source), SSP_ERR_INVALID_ARGUMENT);

    current_clock = HW_CGC_ClockSourceGet(gp_system_reg);     // The currently active system clock source cannot be stopped

    /* if clock source is the current system clock, return error */
    CGC_ERROR_RETURN((clock_source != current_clock), SSP_ERR_CLOCK_ACTIVE);

    /* if PLL is the current clock and its source is the same as clock_source to stop, return an error */
    if ((CGC_CLOCK_PLL == current_clock) && (HW_CGC_PLLClockSourceGet(gp_system_reg) == clock_source))
    {
        CGC_ERROR_RETURN(0, SSP_ERR_CLOCK_ACTIVE);
    }

    /* MOCO cannot be stopped if OSC Stop Detect is enabled */
    CGC_ERROR_RETURN(!((clock_source == CGC_CLOCK_MOCO) && ((HW_CGC_OscStopDetectEnabledGet(gp_system_reg)))), SSP_ERR_OSC_STOP_DET_ENABLED);

    if (!HW_CGC_ClockRunStateGet(gp_system_reg, clock_source))
    {
        return SSP_SUCCESS;    // if clock is already inactive, return success
    }

    /*  if clock source is LOCO, MOCO or SUBCLOCK */
    if (((clock_source == CGC_CLOCK_LOCO) || (clock_source == CGC_CLOCK_MOCO)) || (clock_source == CGC_CLOCK_SUBCLOCK))
    {
        HW_CGC_ClockStop(gp_system_reg, clock_source);       // stop the clock

        if (clock_source == CGC_CLOCK_SUBCLOCK)
        {
            volatile uint32_t timeout;
            timeout = MAX_REGISTER_WAIT_COUNT;
            while ((HW_CGC_ClockRunStateGet(gp_system_reg, CGC_CLOCK_SUBCLOCK)) && (0 != timeout))
            {
                /* wait until the clock state is stopped */
                timeout--;                          // but timeout if it never happens
            }

            CGC_ERROR_RETURN(timeout, SSP_ERR_HARDWARE_TIMEOUT);    // return error if timed out
        }
    }
    else if (((clock_source == CGC_CLOCK_HOCO) || (clock_source == CGC_CLOCK_MAIN_OSC))
             || (clock_source == CGC_CLOCK_PLL))
    {
        /*  make sure the oscillator is stable before stopping */
        CGC_ERROR_RETURN(HW_CGC_ClockCheck(gp_system_reg, clock_source), SSP_ERR_NOT_STABILIZED);

        HW_CGC_ClockStop(gp_system_reg, clock_source);         // stop the clock
    }
    else
    {
        /* statement here to follow coding standard */
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Set the specified clock as the system clock and configure the internal dividers for
 *              ICLK, PCLKA, PCLKB, PCLKC, PCLKD and FCLK.
 *
 *              THIS FUNCTION DOES NOT CHECK TO SEE IF THE OPERATING MODE SUPPORTS THE SPECIFIED CLOCK SOURCE
 *              AND DIVIDER VALUES. SETTING A CLOCK SOURCE AND DVIDER OUTSIDE THE RANGE SUPPORTED BY THE
 *              CURRENT OPERATING MODE WILL RESULT IN UNDEFINED OPERATION.
 *
 *              IF THE LOCO MOCO OR SUBCLOCK ARE CHOSEN AS THE SYSTEM CLOCK, THIS FUNCTION WILL SET THOSE AS THE
 *              SYSTEM CLOCK WITHOUT CHECKING FOR STABILIZATION. IT IS UP TO THE USER TO ENSURE THAT LOCO, MOCO
 *              OR SUBCLOCK ARE STABLE BEFORE USING THEM AS THE SYSTEM CLOCK.
 *
 *              Additionally this function sets the RAM and ROM wait states for the MCU.
 *              For the S7 MCU the ROMWT register controls ROM wait states.
 *              For the S3 MCU the MEMWAIT register controls ROM wait states.
 *
 * @retval SSP_SUCCESS                  Operation performed successfully.
 * @retval SSP_ERR_CLOCK_INACTIVE       The specified clock source is inactive.
 * @retval SSP_ERR_NULL_PTR             The p_clock_cfg parameter is NULL.
 * @retval SSP_ERR_STABILIZED           The clock source has not stabilized
 * @retval SSP_ERR_INVALID_ARGUMENT     Invalid argument used. ICLK is not set as the fastest clock.
 * @retval SSP_ERR_INVALID_MODE         Peripheral divisions are not valid in sub-osc mode
 * @retval SSP_ERR_INVALID_MODE         Oscillator stop detect not allowed in sub-osc mode
 **********************************************************************************************************************/

ssp_err_t R_CGC_SystemClockSet (cgc_clock_t clock_source, cgc_system_clock_cfg_t const * const p_clock_cfg)
{
    cgc_clock_t current_clock;
    bsp_clock_set_callback_args_t args;
    ssp_err_t err;

    /* return error if invalid clock source or not supported by hardware */
    CGC_ERROR_RETURN((HW_CGC_ClockSourceValidCheck(clock_source)), SSP_ERR_INVALID_ARGUMENT);

#if (CGC_CFG_PARAM_CHECKING_ENABLE == 1)
    SSP_ASSERT(NULL != p_clock_cfg);

    if ((1U == gp_cgc_feature->has_pclka) && (p_clock_cfg->pclka_div < p_clock_cfg->iclk_div))
    {
        CGC_ERROR_RETURN(0, SSP_ERR_INVALID_ARGUMENT);      // error if ratios are not correct
    }
    if ((1U == gp_cgc_feature->has_pclkb) && (p_clock_cfg->pclkb_div < p_clock_cfg->iclk_div))
    {
        CGC_ERROR_RETURN(0, SSP_ERR_INVALID_ARGUMENT);      // error if ratios are not correct
    }
    if ((1U == gp_cgc_feature->has_fclk) && (p_clock_cfg->fclk_div < p_clock_cfg->iclk_div))
    {
        CGC_ERROR_RETURN(0, SSP_ERR_INVALID_ARGUMENT);      // error if ratios are not correct
    }
    if ((1U == gp_cgc_feature->has_bclk) && (p_clock_cfg->bclk_div < p_clock_cfg->iclk_div))
    {
        CGC_ERROR_RETURN(0, SSP_ERR_INVALID_ARGUMENT);      // error if ratios are not correct
    }
    if ((CGC_CLOCK_SUBCLOCK == clock_source) &&
        ((0U != p_clock_cfg->iclk_div) || (0U != p_clock_cfg->fclk_div))
        )
    {
        CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);          // Peripheral divisions are not valid in sub-osc mode
    }
    if ((CGC_CLOCK_SUBCLOCK == clock_source) && ( true == HW_CGC_OscStopDetectEnabledGet(gp_system_reg)))
    {
        CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);          // Oscillator stop detect not allowed in sub-osc mode
    }
#endif /* CGC_CFG_PARAM_CHECKING_ENABLE */

    /* Handle MCU specific wait state adjustment. */
    args.event = BSP_CLOCK_SET_EVENT_PRE_CHANGE;
    args.new_rom_wait_state = 0U;
    args.requested_freq_hz = HW_CGC_ClockHzCalculate(clock_source, p_clock_cfg->iclk_div);
    args.current_freq_hz = HW_CGC_ClockHzGet(gp_system_reg, CGC_SYSTEM_CLOCKS_ICLK);
    err = bsp_clock_set_callback(&args);
    CGC_ERROR_RETURN(SSP_SUCCESS == err, err);

    /** In order to correctly set the ROM and RAM wait state registers we need to know the current (S3A7 only) and
     * requested iclk frequencies.
     */

    current_clock = HW_CGC_ClockSourceGet(gp_system_reg);
    if (clock_source != current_clock)    // if clock_source is not the current system clock, check stabilization
    {
        if (((clock_source == CGC_CLOCK_HOCO) || (clock_source == CGC_CLOCK_MAIN_OSC))
            || (clock_source == CGC_CLOCK_PLL))
        {
            /* make sure the oscillator is stable before setting as system clock */
            CGC_ERROR_RETURN(HW_CGC_ClockCheck(gp_system_reg, clock_source), SSP_ERR_NOT_STABILIZED);
        }
    }

    /* Switch to high-speed to prevent any issues with the pending system clock change. */
    HW_CGC_SetHighSpeedMode(gp_system_reg);

    /* In order to avoid a system clock (momentarily) higher than expected, the order of switching the clock and
     * dividers must be so that the frequency of the clock goes lower, instead of higher, before being correct.
     */

    cgc_system_clock_cfg_t  current_clock_cfg;
    HW_CGC_SystemDividersGet(gp_system_reg, &current_clock_cfg);
    /* If the current ICLK divider is less (higher frequency) than the requested ICLK divider,
     *  set the divider first.
     */
    if (current_clock_cfg.iclk_div < p_clock_cfg->iclk_div )
    {
        HW_CGC_SystemDividersSet(gp_system_reg, p_clock_cfg);
        HW_CGC_ClockSourceSet(gp_system_reg, clock_source);
    }
    /* The current ICLK divider is greater (lower frequency) than the requested ICLK divider, so
     * set the clock source first.
     */
    else
    {
        HW_CGC_ClockSourceSet(gp_system_reg, clock_source);
        HW_CGC_SystemDividersSet(gp_system_reg, p_clock_cfg);
    }

    /* Clock is now at requested frequency. */

    /* Update the CMSIS core clock variable so that it reflects the new Iclk freq */
    SystemCoreClock = bsp_cpu_clock_get();

    /* Set the operating mode based on the new system clock source. */

    if (CGC_CLOCK_SUBCLOCK == clock_source)
    {
#if CGC_CFG_USE_LOW_VOLTAGE_MODE
        if (0U != gp_cgc_feature->low_voltage_max_freq_hz)
        {
            HW_CGC_SetLowVoltageMode(gp_system_reg);
        }
        else
#endif
        if (0U != gp_cgc_feature->has_subosc_speed)
        {
            HW_CGC_SetSubOscSpeedMode(gp_system_reg);
        }
        else if (0U != gp_cgc_feature->low_speed_max_freq_hz)
        {
            HW_CGC_SetLowSpeedMode(gp_system_reg);
        }
        else if (0U != gp_cgc_feature->middle_speed_max_freq_hz)
        {
            HW_CGC_SetMiddleSpeedMode(gp_system_reg);
        }
        else
        {
            /* Nothing to do, stay in high speed mode */
        }
    }
    else if (CGC_CLOCK_LOCO == clock_source)
    {
#if CGC_CFG_USE_LOW_VOLTAGE_MODE
        if (0U != gp_cgc_feature->low_voltage_max_freq_hz)
        {
            HW_CGC_SetLowVoltageMode(gp_system_reg);
        }
        else
#endif
        if (0U != gp_cgc_feature->low_speed_max_freq_hz)
        {
            HW_CGC_SetLowSpeedMode(gp_system_reg);
        }
        else if (0U != gp_cgc_feature->middle_speed_max_freq_hz)
        {
            HW_CGC_SetMiddleSpeedMode(gp_system_reg);
        }
        else
        {
            /* Nothing to do, stay in high speed mode */
        }
    }
    else if (CGC_CLOCK_MOCO == clock_source)
    {
        if (0U != gp_cgc_feature->middle_speed_max_freq_hz)
        {
            /* Switch to middle speed mode */
            HW_CGC_SetMiddleSpeedMode(gp_system_reg);
        }
        else
        {
            /* Nothing to do, stay in high speed mode */
        }
    }
    else if (CGC_CLOCK_MAIN_OSC == clock_source)
    {
        /* See if we need can switch to middle or low speed mode safely */
        if ((SystemCoreClock > gp_cgc_feature->middle_speed_max_freq_hz) &&
            (0U != gp_cgc_feature->middle_speed_max_freq_hz)
            )
        {
            /* Nothing to do, stay in high speed mode */
        }
#if CGC_CFG_USE_LOW_VOLTAGE_MODE
        else if ((0U != gp_cgc_feature->low_voltage_max_freq_hz) &&
                 (SystemCoreClock < gp_cgc_feature->low_voltage_max_freq_hz)
                )
        {
            HW_CGC_SetLowVoltageMode(gp_system_reg);
        }
#endif
        else if ((SystemCoreClock > gp_cgc_feature->low_speed_max_freq_hz) &&
                 (0U != gp_cgc_feature->low_speed_max_freq_hz) &&
                 (0U != gp_cgc_feature->middle_speed_max_freq_hz)
                 )
        {
            /* Switch to middle speed mode */
            HW_CGC_SetMiddleSpeedMode(gp_system_reg);
        }
        else if (0U != gp_cgc_feature->low_speed_max_freq_hz)
        {
            /* Switch to low speed mode */
            HW_CGC_SetLowSpeedMode(gp_system_reg);
        }
        else
        {
            /* Nothing to do, stay in high speed mode */
        }
    }
    else if (CGC_CLOCK_PLL == clock_source)
    {
        /* PLL will only run in High or Middle Speed modes. */
        if (SystemCoreClock > gp_cgc_feature->middle_speed_max_freq_hz)
        {
            /* Nothing to do, stay in high speed mode */
        }
        else
        {
            HW_CGC_SetMiddleSpeedMode(gp_system_reg);
        }
    }
    else /* if (CGC_CLOCK_HOCO == clock_source) */
    {
        /* Nothing to do, stay in high speed mode */
    }

    args.current_freq_hz = HW_CGC_ClockHzGet(gp_system_reg, CGC_SYSTEM_CLOCKS_ICLK);
    args.event = BSP_CLOCK_SET_EVENT_POST_CHANGE;
    err = bsp_clock_set_callback(&args);
    CGC_ERROR_RETURN(SSP_SUCCESS == err, err);

#if (1 == BSP_CFG_RTOS)
    /* If an RTOS is in use, Update the Systick period based on the new frequency, using the ThreadX systick period in Microsecs */
    R_CGC_SystickUpdate((1000000 / TX_TIMER_TICKS_PER_SECOND), CGC_SYSTICK_PERIOD_UNITS_MICROSECONDS);
#endif

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Return the current system clock source and configuration.
 * @retval SSP_SUCCESS   Parameters returned successfully.
 *
 **********************************************************************************************************************/

ssp_err_t R_CGC_SystemClockGet (cgc_clock_t * clock_source, cgc_system_clock_cfg_t * p_set_clock_cfg)
{
    *clock_source = HW_CGC_ClockSourceGet(gp_system_reg);
    HW_CGC_SystemDividersGet(gp_system_reg, p_set_clock_cfg);

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Return the requested internal clock frequency in Hz.
 * @retval SSP_SUCCESS                  Operation performed successfully.
 * @retval SSP_ERR_INVALID_ARGUMENT     Invalid clock specified.
 **********************************************************************************************************************/
ssp_err_t R_CGC_SystemClockFreqGet (cgc_system_clocks_t clock, uint32_t * p_freq_hz)
{
    /* return error if invalid system clock or not supported by hardware */
    *p_freq_hz = 0x00U;
    CGC_ERROR_RETURN((HW_CGC_SystemClockValidCheck(clock)), SSP_ERR_INVALID_ARGUMENT);

    *p_freq_hz = HW_CGC_ClockHzGet(gp_system_reg, clock);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Check the specified clock for stability.
 * @retval SSP_ERR_STABILIZED           Clock stabilized.
 * @retval SSP_ERR_NOT_STABILIZED       Clock not stabilized.
 * @retval SSP_ERR_CLOCK_ACTIVE         Clock active but not able to check for stability.
 * @retval SSP_ERR_CLOCK_INACTIVE       Clock not turned on.
 * @retval SSP_ERR_INVALID_ARGUMENT     Illegal parameter passed.
 **********************************************************************************************************************/
ssp_err_t R_CGC_ClockCheck (cgc_clock_t clock_source)
{
    /* return error if invalid clock source or not supported by hardware */
    CGC_ERROR_RETURN((HW_CGC_ClockSourceValidCheck(clock_source)), SSP_ERR_INVALID_ARGUMENT);

    /*  There is no function to check for LOCO, MOCO or SUBCLOCK stability */
    if (((clock_source == CGC_CLOCK_LOCO) || (clock_source == CGC_CLOCK_MOCO)) || (clock_source == CGC_CLOCK_SUBCLOCK))
    {
        if (true == HW_CGC_ClockRunStateGet(gp_system_reg, clock_source))
        {
            return SSP_ERR_CLOCK_ACTIVE;      // There is no hardware to check for stability so just check for state.
        }
        else
        {
            return SSP_ERR_CLOCK_INACTIVE;    // There is no hardware to check for stability so just check for state.
        }
    }

    /*  There is a function to check for HOCO, MAIN_OSC and PLL stability */
    else if (((clock_source == CGC_CLOCK_HOCO) || (clock_source == CGC_CLOCK_MAIN_OSC))
             || (clock_source == CGC_CLOCK_PLL))
    {
        /* if clock is not active, can't check for stability, return error */
        CGC_ERROR_RETURN(HW_CGC_ClockRunStateGet(gp_system_reg, clock_source), SSP_ERR_CLOCK_INACTIVE);

        /*  check oscillator for stability, return error if not stable */
        CGC_ERROR_RETURN(HW_CGC_ClockCheck(gp_system_reg, clock_source), SSP_ERR_NOT_STABILIZED);
        return SSP_ERR_STABILIZED;              // otherwise, return affirmative, not really an error
    }
    else
    {
        /* statement here to follow coding standard */
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Enable or disable the oscillation stop detection for the main clock.
 *  The MCU will automatically switch the system clock to MOCO when a stop is detected if Main Clock is
 *  the system clock. If the system clock is the PLL, then the clock source will not be changed and the
 *  PLL free running frequency will be the system clock frequency.
 * @retval SSP_SUCCESS                  Operation performed successfully.
 * @retval SSP_ERR_OSC_STOP_DETECTED    The Oscillation stop detect status flag is set. Under this condition it is not
 *                                      possible to disable the Oscillation stop detection function.
 * @retval SSP_ERR_NULL_PTR             Null pointer passed for callback function when the second argument is "true".
 * @retval SSP_ERR_ASSERTION            Cannot enable oscillator stop detect in sub-osc speed mode
 * @retval SSP_ERR_ASSERTION            Invalid peripheral clock divisions for oscillator stop detect
 **********************************************************************************************************************/
ssp_err_t R_CGC_OscStopDetect (void (* p_callback) (cgc_callback_args_t * p_args), bool enable)
{
    if (true == enable)
    {
#if (CGC_CFG_PARAM_CHECKING_ENABLE == 1)
        SSP_ASSERT(NULL != p_callback);
        cgc_operating_modes_t operating_mode = HW_CGC_OperatingModeGet(gp_system_reg);
        SSP_ASSERT(CGC_SUBOSC_SPEED_MODE != operating_mode);
        cgc_system_clock_cfg_t clock_cfg;
        HW_CGC_SystemDividersGet(gp_system_reg, &clock_cfg);
        if((0U != gp_cgc_feature->low_speed_max_freq_hz) && (CGC_LOW_SPEED_MODE == operating_mode))
        {
            if((1U == gp_cgc_feature->has_pclka) && (clock_cfg.pclka_div < gp_cgc_feature->low_speed_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_pclkb) && (clock_cfg.pclkb_div < gp_cgc_feature->low_speed_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_pclkc) && (clock_cfg.pclkc_div < gp_cgc_feature->low_speed_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_pclkd) && (clock_cfg.pclkd_div < gp_cgc_feature->low_speed_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_fclk) && (clock_cfg.fclk_div < gp_cgc_feature->low_speed_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_bclk) && (clock_cfg.bclk_div < gp_cgc_feature->low_speed_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            CGC_ERROR_RETURN(clock_cfg.iclk_div >= gp_cgc_feature->low_speed_pclk_div_min, SSP_ERR_INVALID_MODE);
        }
        else if((0U != gp_cgc_feature->low_voltage_max_freq_hz) && (CGC_LOW_VOLTAGE_MODE == operating_mode))
        {
            if((1U == gp_cgc_feature->has_pclka) && (clock_cfg.pclka_div < gp_cgc_feature->low_voltage_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_pclkb) && (clock_cfg.pclkb_div < gp_cgc_feature->low_voltage_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_pclkc) && (clock_cfg.pclkc_div < gp_cgc_feature->low_voltage_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_pclkd) && (clock_cfg.pclkd_div < gp_cgc_feature->low_voltage_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_fclk) && (clock_cfg.fclk_div < gp_cgc_feature->low_voltage_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            if((1U == gp_cgc_feature->has_bclk) && (clock_cfg.bclk_div < gp_cgc_feature->low_voltage_pclk_div_min))
            {
                CGC_ERROR_RETURN(0, SSP_ERR_INVALID_MODE);
            }
            CGC_ERROR_RETURN(clock_cfg.iclk_div >= gp_cgc_feature->low_voltage_pclk_div_min, SSP_ERR_INVALID_MODE);
        }
        else
        {
            /* Do nothing */
        }
#endif
        /* - add callback function to BSP */
        R_BSP_GroupIrqWrite(BSP_GRP_IRQ_OSC_STOP_DETECT, (bsp_grp_irq_cb_t) p_callback);
        HW_CGC_OscStopDetectEnable(gp_system_reg);          // enable hardware oscillator stop detect
    }
    else
    {
        /* if oscillator stop detected, return error */
        CGC_ERROR_RETURN(!(HW_CGC_OscStopDetectGet(gp_system_reg)), SSP_ERR_OSC_STOP_DETECTED);
        HW_CGC_OscStopDetectDisable(gp_system_reg);          // disable hardware oscillator stop detect
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Clear the Oscillation Stop Detection Status register.
 *
 *                This register is not cleared automatically if the stopped clock is restarted.
 *                This function blocks for about 3 ICLK cycles until the status register is cleared.
 * @retval SSP_SUCCESS 						Operation performed successfully.
 * @retval SSP_ERR_OSC_STOP_CLOCK_ACTIVE    The Oscillation Detect Status flag cannot be cleared if the
 *                                          Main Osc or PLL is set as the system clock. Change the
 *                                          system clock before attempting to clear this bit.
 **********************************************************************************************************************/

ssp_err_t R_CGC_OscStopStatusClear (void)
{
    cgc_clock_t current_clock;

    if (HW_CGC_OscStopDetectGet(gp_system_reg))               // if oscillator stop detected
    {
        current_clock = HW_CGC_ClockSourceGet(gp_system_reg); // The currently active system clock source

        if (0U != gp_cgc_feature->pllccr_type)
        {
            /* MCU has PLL. */
            if (gp_cgc_feature->pll_src_configurable)
            {
                cgc_clock_t alt_clock;
                alt_clock = HW_CGC_PLLClockSourceGet(gp_system_reg);
                /* cannot clear oscillator stop status if Main Osc is source of PLL */
                if ((CGC_CLOCK_PLL == current_clock) && (CGC_CLOCK_MAIN_OSC == alt_clock))
                {
                    CGC_ERROR_RETURN(0, SSP_ERR_OSC_STOP_CLOCK_ACTIVE);    // return error
                }
            }
            else
            {
                /* cannot clear oscillator stop status if PLL is current clock */
                CGC_ERROR_RETURN(!(CGC_CLOCK_PLL == current_clock), SSP_ERR_OSC_STOP_CLOCK_ACTIVE);
            }
        }

        /* cannot clear oscillator stop status if Main Osc is current clock */
        CGC_ERROR_RETURN(!(CGC_CLOCK_MAIN_OSC == current_clock), SSP_ERR_OSC_STOP_CLOCK_ACTIVE);
    }

    HW_CGC_OscStopStatusClear(gp_system_reg);          // clear hardware oscillator stop detect status

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Configure the secondary dividers for BCLKOUT. The primary divider is set using the
 *           bsp clock configuration and the R_CGC_SystemClockSet function.
 * @retval SSP_SUCCESS                  Operation performed successfully.
  **********************************************************************************************************************/

ssp_err_t R_CGC_BusClockOutCfg (cgc_bclockout_dividers_t divider)
{
    /* The application must set up the PFS register so the BCLK pin is an output */

    HW_CGC_BusClockOutCfg(gp_system_reg, divider);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Enable the BCLKOUT output.
 * @retval SSP_SUCCESS  Operation performed successfully.
 **********************************************************************************************************************/

ssp_err_t R_CGC_BusClockOutEnable (void)
{
    HW_CGC_BusClockOutEnable(gp_system_reg);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Disable the BCLKOUT output.
 * @retval SSP_SUCCESS  Operation performed successfully.
 **********************************************************************************************************************/

ssp_err_t R_CGC_BusClockOutDisable (void)
{
    HW_CGC_BusClockOutDisable(gp_system_reg);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Configure the dividers for CLKOUT.
 * @retval SSP_SUCCESS                  Operation performed successfully.
 **********************************************************************************************************************/
ssp_err_t R_CGC_ClockOutCfg (cgc_clock_t clock, cgc_clockout_dividers_t divider)
{
    /* The application must set up the PFS register so the CLKOUT pin is an output */

    HW_CGC_ClockOutCfg(gp_system_reg, clock, divider);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Enable the CLKOUT output.
 * @retval SSP_SUCCESS  Operation performed successfully.
 **********************************************************************************************************************/

ssp_err_t R_CGC_ClockOutEnable (void)
{
    HW_CGC_ClockOutEnable(gp_system_reg);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Disable the CLKOUT output.
 * @retval SSP_SUCCESS  Operation performed successfully.
 **********************************************************************************************************************/

ssp_err_t R_CGC_ClockOutDisable (void)
{
    HW_CGC_ClockOutDisable(gp_system_reg);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Configure the source for the segment LCDCLK.
 * @retval SSP_SUCCESS                  Operation performed successfully.
  **********************************************************************************************************************/

ssp_err_t R_CGC_LCDClockCfg (cgc_clock_t clock)
{
    /* The application must set up the PFS register so the LCDCLKOUT pin is an output */
    if (1U == gp_cgc_feature->has_lcd_clock)
    {
        CGC_ERROR_RETURN(CGC_LDC_INVALID_CLOCK != g_lcd_clock_settings[clock], SSP_ERR_INVALID_ARGUMENT);

        /* Protect OFF for CGC. */
        R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);

        bool lcd_clock_was_enabled = HW_CGC_LCDClockIsEnabled(gp_system_reg);
        R_CGC_LCDClockDisable();
        HW_CGC_LCDClockCfg(gp_system_reg, g_lcd_clock_settings[clock]);
        uint32_t timeout = CGC_LCD_CFG_TIMEOUT;
        while ((g_lcd_clock_settings[clock] != HW_CGC_LCDClockCfgGet(gp_system_reg)) && (0U != timeout))  /* wait for the bit to set */
        {
            timeout--;
        }

        if (lcd_clock_was_enabled)
        {
            R_CGC_LCDClockEnable();
        }

        /* Protect ON for CGC. */
        R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);

        CGC_ERROR_RETURN(timeout > 0U, SSP_ERR_TIMEOUT);
    }
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Enable the segment LCDCLK output.
 * @retval SSP_SUCCESS  Operation performed successfully.
 **********************************************************************************************************************/

ssp_err_t R_CGC_LCDClockEnable (void)
{
    if (1U == gp_cgc_feature->has_lcd_clock)
    {
        /* Protect OFF for CGC. */
        R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);

        HW_CGC_LCDClockEnable(gp_system_reg);
        uint32_t timeout = CGC_LCD_CFG_TIMEOUT;
        while ((true != HW_CGC_LCDClockIsEnabled(gp_system_reg)) && (0U != timeout))  /* wait for the bit to set */
        {
            timeout--;
        }

        /* Protect ON for CGC. */
        R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);

        CGC_ERROR_RETURN(timeout > 0U, SSP_ERR_TIMEOUT);
    }
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Disable the segment LCDCLK output.
 * @retval SSP_SUCCESS      Operation performed successfully.
 **********************************************************************************************************************/

ssp_err_t R_CGC_LCDClockDisable (void)
{
    if (1U == gp_cgc_feature->has_lcd_clock)
    {
        /* Protect OFF for CGC. */
        R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);

        HW_CGC_LCDClockDisable(gp_system_reg);
        uint32_t timeout = CGC_LCD_CFG_TIMEOUT;
        while ((false != HW_CGC_LCDClockIsEnabled(gp_system_reg)) && (0U != timeout))  /* wait for the bit to set */
        {
            timeout--;
        }

        /* Protect ON for CGC. */
        R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);

        CGC_ERROR_RETURN(timeout > 0U, SSP_ERR_TIMEOUT);
    }
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Enable the SDCLK output.
 * @retval SSP_SUCCESS  Operation performed successfully.
 **********************************************************************************************************************/

ssp_err_t R_CGC_SDRAMClockOutEnable (void)
{
    if (1U == gp_cgc_feature->has_sdram_clock)
    {
        HW_CGC_SDRAMClockOutEnable(gp_system_reg);
    }
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Disable the SDCLK output.
 * @retval SSP_SUCCESS      Operation performed successfully.
 **********************************************************************************************************************/

ssp_err_t R_CGC_SDRAMClockOutDisable (void)
{
    if (1U == gp_cgc_feature->has_sdram_clock)
    {
        HW_CGC_SDRAMClockOutDisable(gp_system_reg);
    }
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Configure the dividers for UCLK.
 * @retval SSP_SUCCESS                  Operation performed successfully.
 * @retval SSP_ERR_INVALID_ARGUMENT     Invalid divider specified.
 **********************************************************************************************************************/

ssp_err_t R_CGC_USBClockCfg (cgc_usb_clock_div_t divider)
{
    /* The application must set up the PFS register so the USBCLKOUT pin is an output */

    if (1U == gp_cgc_feature->has_usb_clock_div)
    {
        HW_CGC_USBClockCfg(gp_system_reg, divider);
    }
    return SSP_SUCCESS;
}


/*******************************************************************************************************************//**
 * @brief  Re-Configure the systick based on the provided period and current system clock frequency.
 * @param[in]   period_count       The duration for the systick period.
 * @param[in]   units              The units for the provided period.
 * @retval SSP_SUCCESS                  Operation performed successfully.
 * @retval SSP_ERR_INVALID_ARGUMENT     Invalid period specified.
 * @retval SSP_ERR_ABORTED              Attempt to update systick timer failed.
 **********************************************************************************************************************/
ssp_err_t R_CGC_SystickUpdate(uint32_t period_count, cgc_systick_period_units_t units)
{
    uint32_t requested_period_count = period_count;
    uint32_t reload_value;
    uint32_t freq;
    cgc_systick_period_units_t period_units = units;
    /*LDRA_INSPECTED 90 s float used because float32_t is not part of the C99 standard integer definitions. */
    float period = 0.0f;

#if (CGC_CFG_PARAM_CHECKING_ENABLE)
    if (0 == period_count)
    {
        CGC_ERROR_RETURN(0, SSP_ERR_INVALID_ARGUMENT);     // Invalid period provided
    }
#endif

    freq = bsp_cpu_clock_get();		                // Get the current ICLK frequency

    /* If an RTOS is in use then we want to set the Systick period to that defined by the RTOS. So we'll convert the macro
     * settings use the existing code and calculate the reload value
     */
#if (1 == BSP_CFG_RTOS)
    period_units = CGC_SYSTICK_PERIOD_UNITS_MICROSECONDS;
    requested_period_count = (RELOAD_COUNT_FOR_1US) / TX_TIMER_TICKS_PER_SECOND;        // Convert ticks per sec to ticks per us
#endif


    /*LDRA_INSPECTED 90 s *//*LDRA_INSPECTED 90 s */
    period = ((1.0f)/(float)freq) * (float)period_units;           // This is the period in the provided units
    /*LDRA_INSPECTED 90 s */
    reload_value = (uint32_t)((float)requested_period_count/period);

    if (HW_CGC_SystickUpdate(reload_value) == false)	// Configure the systick period as requested
    {
        return SSP_ERR_ABORTED;
    }
    return SSP_SUCCESS;
}


/*******************************************************************************************************************//**
 * @brief  Return the driver version.
 * @retval SSP_SUCCESS      Operation performed successfully.
 **********************************************************************************************************************/

ssp_err_t R_CGC_VersionGet (ssp_version_t * const p_version)
{
    p_version->version_id = g_cgc_version.version_id;
    return SSP_SUCCESS;
}

static ssp_err_t r_cgc_stabilization_wait(cgc_clock_t clock)
{
    ssp_err_t err = SSP_ERR_NOT_STABILIZED;

    int32_t timeout = 0xFFFF;
    while (SSP_ERR_NOT_STABILIZED == err)
    {
        /* Wait for clock source to stabilize */
        timeout--;
        CGC_ERROR_RETURN(0 < timeout, SSP_ERR_NOT_STABILIZED);
        err = g_cgc_on_cgc.clockCheck(clock);
    }

    CGC_ERROR_RETURN((SSP_SUCCESS == err) || (SSP_ERR_STABILIZED == err) ||
        (SSP_ERR_CLOCK_ACTIVE == err), err);

    return SSP_SUCCESS;
}

static ssp_err_t r_cgc_clock_start_stop(cgc_clock_change_t clock_state, cgc_clock_t clock_to_change)
{
    ssp_err_t err = SSP_SUCCESS;

    if(CGC_CLOCK_CHANGE_STOP == clock_state)
    {
        err = g_cgc_on_cgc.clockStop(clock_to_change);
        CGC_ERROR_RETURN(SSP_SUCCESS == err, err);
    }
    else if(CGC_CLOCK_CHANGE_START == clock_state)
    {
        err = g_cgc_on_cgc.clockStart(clock_to_change, NULL);
        CGC_ERROR_RETURN((SSP_SUCCESS == err) || (SSP_ERR_CLOCK_ACTIVE == err), err);
    }
    else /* CGC_CLOCK_OPTION_NO_CHANGE */
    {
        /* Do nothing */
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @} (end addtogroup CGC)
 **********************************************************************************************************************/
