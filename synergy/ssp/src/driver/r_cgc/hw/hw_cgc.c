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
 * File Name    : hw_cgc.c
 * Description  : Hardware related  LLD functions for the CGC HAL
 **********************************************************************************************************************/

#include "r_cgc_api.h"
#include "hw_cgc_private.h"
#include "bsp_api.h"
#include "bsp_clock_cfg.h"
#include "r_cgc_cfg.h"
#include "hw_cgc.h"

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define CGC_PLL_DIV_1_SETTING 0
#define CGC_PLL_DIV_2_SETTING 1
#define CGC_PLL_DIV_4_SETTING 2

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/**********************************************************************************************************************
* Private function prototypes
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/

/** This section of RAM should not be initialized by the C runtime environment */
/*LDRA_NOANALYSIS LDRA_INSPECTED below not working. */
/* This is initialized in cgc_api_t::init, which is called before the C runtime environment is initialized. */
/*LDRA_INSPECTED 219 S */
static uint32_t           g_clock_freq[CGC_CLOCK_PLL + 1]  BSP_PLACE_IN_SECTION(".noinit");
/*LDRA_ANALYSIS */

/** These are the divisor values to use when calculating the system clock frequency, using the CGC_PLL_DIV enum type */
static const uint16_t g_pllccr_div_value[] =
{
    [CGC_PLL_DIV_1] = 0x01U,
    [CGC_PLL_DIV_2] = 0x02U,
    [CGC_PLL_DIV_3] = 0x03U
};
/** These are the values to use to set the PLL divider register according to the CGC_PLL_DIV enum type */
static const uint16_t g_pllccr_div_setting[] =
{
    [CGC_PLL_DIV_1] = 0x00U,
    [CGC_PLL_DIV_2] = 0x01U,
    [CGC_PLL_DIV_3] = 0x02U
};
/** These are the divisor values to use when calculating the system clock frequency, using the CGC_PLL_DIV enum type */
static const uint16_t g_pllccr2_div_value[] =
{
    [CGC_PLL_DIV_1_SETTING]         = 0x01U,
    [CGC_PLL_DIV_2_SETTING]         = 0x02U,
    [CGC_PLL_DIV_4_SETTING]         = 0x04U
};

/** These are the values to use to set the PLL divider register according to the CGC_PLL_DIV enum type */
static const uint16_t g_pllccr2_div_setting[] =
{
    [CGC_PLL_DIV_1] = 0x00U,
    [CGC_PLL_DIV_2] = 0x01U,
    [CGC_PLL_DIV_4] = 0x02U
};

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief      This function initializes CGC variables independent of the C runtime environment.
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_Init (void)
{
    /** initialize the clock frequency array */
    g_clock_freq[CGC_CLOCK_HOCO]     = gp_cgc_feature->hoco_freq_hz;  // Initialize the HOCO value.
    g_clock_freq[CGC_CLOCK_MOCO]     = CGC_MOCO_FREQ;       // Initialize the MOCO value.
    g_clock_freq[CGC_CLOCK_LOCO]     = CGC_LOCO_FREQ;       // Initialize the LOCO value.
    g_clock_freq[CGC_CLOCK_MAIN_OSC] = gp_cgc_feature->main_osc_freq_hz;   // Initialize the Main oscillator value.
    g_clock_freq[CGC_CLOCK_SUBCLOCK] = CGC_SUBCLOCK_FREQ;   // Initialize the subclock value.
    g_clock_freq[CGC_CLOCK_PLL]      = CGC_PLL_FREQ;        // The PLL value will be calculated at initialization.
}

/*******************************************************************************************************************//**
 * @brief      This function initializes PLL frequency value
 * @retval     none
 **********************************************************************************************************************/
void HW_CGC_InitPLLFreq (R_SYSTEM_Type * p_system_reg)
{
    if (0U != gp_cgc_feature->pllccr_type)
    {
        uint32_t divider = 0U;
        divider = HW_CGC_PLLDividerGet(p_system_reg);
        if (divider != 0U) /* prevent divide by 0 */
        {
            uint32_t clock_freq = 0U;
            if (1U == gp_cgc_feature->pllccr_type)
            {
                clock_freq = g_clock_freq[HW_CGC_PLLClockSourceGet(p_system_reg)];
            }
            if (2U == gp_cgc_feature->pllccr_type)
            {
                clock_freq = g_clock_freq[CGC_CLOCK_MAIN_OSC];
            }
            /* This casts the float result back to an integer.  The multiplier is always a multiple of 0.5, and the clock
             * frequency is always a multiple of 2, so casting to an integer is safe. */
            /* Float used because float32_t is not part of the C99 standard integer definitions. */
            /*LDRA_INSPECTED 90 s *//*LDRA_INSPECTED 90 s */
            g_clock_freq[CGC_CLOCK_PLL] = (uint32_t)
                (((float) clock_freq / (float)divider) * HW_CGC_PLLMultiplierGet(p_system_reg));
        }
    }
}

/*******************************************************************************************************************//**
 * @brief      This function locks CGC registers
 * @retval     none
 **********************************************************************************************************************/
void HW_CGC_HardwareLock (void)
{
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_CGC);
}

/*******************************************************************************************************************//**
 * @brief      This function unlocks CGC registers
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_HardwareUnLock (void)
{
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_CGC);
}

/*******************************************************************************************************************//**
 * @brief      This function locks CGC registers
 * @retval     none
 **********************************************************************************************************************/
void HW_CGC_LPMHardwareLock (void)
{
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_OM_LPC_BATT);
}

/*******************************************************************************************************************//**
 * @brief      This function unlocks CGC registers
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_LPMHardwareUnLock (void)
{
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_OM_LPC_BATT);
}


/*******************************************************************************************************************//**
 * @brief      This function sets the main clock drive
 * @param[in]  setting  - clock drive setting
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_MainClockDriveSet (R_SYSTEM_Type * p_system_reg, uint8_t setting)
{
    /*  Set the drive capacity for the main clock. */
    uint8_t modrv_mask = gp_cgc_feature->modrv_mask;
    uint8_t modrv_shift = gp_cgc_feature->modrv_shift;
    HW_CGC_HardwareUnLock();
    p_system_reg->MOMCR =
        (uint8_t) ((p_system_reg->MOMCR & (~modrv_mask)) | ((setting << modrv_shift) & modrv_mask));
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function sets the subclock drive
 * @param[in]  setting  - clock drive setting
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_SubClockDriveSet (R_SYSTEM_Type * p_system_reg, uint8_t setting)
{
    /*  Set the drive capacity for the subclock. */
    uint8_t sodrv_mask = gp_cgc_feature->sodrv_mask;
    uint8_t sodrv_shift = gp_cgc_feature->sodrv_shift;
    HW_CGC_HardwareUnLock();
    p_system_reg->SOMCR =
        (uint8_t) ((p_system_reg->SOMCR & (~sodrv_mask)) | ((setting << sodrv_shift) & sodrv_mask));
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function starts the selected clock
 * @param[in]  clock - the clock to start
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_ClockStart (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock)
{
    /* Start the selected clock. */
    HW_CGC_HardwareUnLock();

    switch (clock)
    {
        case CGC_CLOCK_HOCO:
            p_system_reg->HOCOCR_b.HCSTP = 0U; /* Start the HOCO clock. */
            break;

        case CGC_CLOCK_MOCO:
            p_system_reg->MOCOCR_b.MCSTP = 0U; /* Start the MOCO clock.*/
            break;

        case CGC_CLOCK_LOCO:
            p_system_reg->LOCOCR_b.LCSTP = 0U; /* Start the LOCO clock.*/
            break;

        case CGC_CLOCK_MAIN_OSC:
            p_system_reg->MOSCCR_b.MOSTP = 0U; /* Start the Main oscillator.*/
            break;

        case CGC_CLOCK_SUBCLOCK:
            p_system_reg->SOSCCR_b.SOSTP = 0U; /* Start the subClock.*/
            break;

        case CGC_CLOCK_PLL:
            p_system_reg->PLLCR_b.PLLSTP = 0U; /* Start the PLL clock.*/
            break;

        default:
            break;
    }

    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function stops the selected clock
 * @param[in]  clock - the clock to stop
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_ClockStop (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock)
{
    /*  Stop the selected clock. */
    HW_CGC_HardwareUnLock();
    switch (clock)
    {
        case CGC_CLOCK_HOCO:
            p_system_reg->HOCOCR_b.HCSTP = 1U; /* Stop the HOCO clock.*/
            break;

        case CGC_CLOCK_MOCO:
            p_system_reg->MOCOCR_b.MCSTP = 1U; /* Stop the MOCO clock.*/
            break;

        case CGC_CLOCK_LOCO:
            p_system_reg->LOCOCR_b.LCSTP = 1U; /* Stop the LOCO clock.*/
            break;

        case CGC_CLOCK_MAIN_OSC:
            p_system_reg->MOSCCR_b.MOSTP = 1U; /* Stop the  main oscillator.*/
            break;

        case CGC_CLOCK_SUBCLOCK:
            p_system_reg->SOSCCR_b.SOSTP = 1U; /* Stop the subClock.*/
            break;

        case CGC_CLOCK_PLL:
            p_system_reg->PLLCR_b.PLLSTP = 1U; /* Stop PLL clock.*/
            break;

        default:
            break;
    }

    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function checks the selected clock for stability
 * @param[in]  clock - the clock to check
 * @retval     true if stable, false if not stable or stopped
 **********************************************************************************************************************/

bool HW_CGC_ClockCheck (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock)
{
    /*  Check for stability of selected clock. */
    switch (clock)
    {
        case CGC_CLOCK_HOCO:
            if (p_system_reg->OSCSF_b.HOCOSF)
            {
                return true; /* HOCO is stable */
            }

            break;

        case CGC_CLOCK_MAIN_OSC:
            if (p_system_reg->OSCSF_b.MOSCSF)
            {
                return true; /* Main Osc is stable */
            }

            break;

        case CGC_CLOCK_PLL:
            if (p_system_reg->OSCSF_b.PLLSF)
            {
                return true; /* PLL is stable */
            }

            break;

        default:

            break;
    }

    return false;
}

/*******************************************************************************************************************//**
 * @brief      This function returns the run state of the selected clock
 * @param[in]  clock - the clock to check
 * @retval     true if running, false if stopped
 **********************************************************************************************************************/

bool HW_CGC_ClockRunStateGet (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock)
{
    /* Get clock run state. */

    switch (clock)
    {
        case CGC_CLOCK_HOCO:
            if (!(p_system_reg->HOCOCR_b.HCSTP))
            {
                return true; /* HOCO clock is running */
            }

            break;

        case CGC_CLOCK_MOCO:
            if (!(p_system_reg->MOCOCR_b.MCSTP))
            {
                return true; /* MOCO clock is running */
            }

            break;

        case CGC_CLOCK_LOCO:
            if (!(p_system_reg->LOCOCR_b.LCSTP))
            {
                return true; /* LOCO clock is running */
            }

            break;

        case CGC_CLOCK_MAIN_OSC:
            if (!(p_system_reg->MOSCCR_b.MOSTP))
            {
                return true; /* main osc clock is running */
            }

            break;

        case CGC_CLOCK_SUBCLOCK:
            if (!(p_system_reg->SOSCCR_b.SOSTP))
            {
                return true; /* Subclock is running */
            }

            break;

        case CGC_CLOCK_PLL:
            if (!(p_system_reg->PLLCR_b.PLLSTP))
            {
                return true; /* PLL clock is running */
            }

            break;

        default:
            return false;
            break;
    }

    return false;
}

/*******************************************************************************************************************//**
 * @brief      This function delays for a specified number of clock cycles, of the selected clock
 * @param[in]  clock - the clock to time
 * @param[in]  cycles - the number of cycles to delay
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_DelayCycles (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock, uint16_t cycles)
{
    /* delay for number of clock cycles specified */

    uint32_t               delay_count;
    uint32_t               clock_freq_in;
    uint32_t               system_clock_freq;

    system_clock_freq = HW_CGC_ClockHzGet(p_system_reg, CGC_SYSTEM_CLOCKS_ICLK);
    clock_freq_in     = g_clock_freq[clock];
    if (clock_freq_in != 0U)             // ensure divide by zero doesn't happen
    {
        delay_count = ((system_clock_freq / clock_freq_in) * cycles);

        while (delay_count > 0U)
        {
            delay_count--;
        }
    }
}

/*******************************************************************************************************************//**
 * @brief      This function sets the source of the main oscillator
 * @param[in]  osc - the source of the main clock oscillator
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_MainOscSourceSet (R_SYSTEM_Type * p_system_reg, cgc_osc_source_t osc)
{
    /* Set the source to resonator or external osc. */
    HW_CGC_HardwareUnLock();
    p_system_reg->MOMCR_b.MOSEL = osc;
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function sets the clock wait time
 * @param[in]  clock is the clock to set the wait time for
 * @param[in]  setting is wait time
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_ClockWaitSet (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock, uint8_t setting)
{
    SSP_PARAMETER_NOT_USED(clock);
    /* Set the clock wait time */
    HW_CGC_HardwareUnLock();
    p_system_reg->MOSCWTCR_b.MSTS = (uint8_t)(setting & 0x0F);
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function checks that the clock is available in the hardware
 * @param[in]  clock is the clock to check to see if it is valid
 * @retval     true if clock available, false if not
 **********************************************************************************************************************/

bool HW_CGC_ClockSourceValidCheck (cgc_clock_t clock)
{
    /* Check for valid clock source */
    if (0U == gp_cgc_feature->pllccr_type)
    {
        if (clock > CGC_CLOCK_SUBCLOCK)
        {
            return false;
        }
    }
    else
    {
        if (clock > CGC_CLOCK_PLL)
        {
            return false;
        }
    }

    return true;
}

/*******************************************************************************************************************//**
 * @brief      This function checks for a valid system clock
 * @param[in]  clock is the clock to check
 * @retval     true if clock available, false if not
 **********************************************************************************************************************/

bool HW_CGC_SystemClockValidCheck (cgc_system_clocks_t clock)
{
    /* Check for valid system clock */
    if (CGC_SYSTEMS_CLOCKS_MAX < clock)
    {
        return false;               // clock not valid
    }

    return true;
}

/*******************************************************************************************************************//**
 * @brief      This function returns the system clock source
 * @retval     the enum of the current system clock
 **********************************************************************************************************************/

cgc_clock_t HW_CGC_ClockSourceGet (R_SYSTEM_Type * p_system_reg)
{
    /* Get the system clock source */
    return (cgc_clock_t) p_system_reg->SCKSCR_b.CKSEL;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the system clock source
 * @param[in]  clock is the clock to use for the system clock
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_ClockSourceSet (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock)
{
    /* Set the system clock source */
    HW_CGC_HardwareUnLock();
    p_system_reg->SCKSCR_b.CKSEL = clock;    //set the system clock source
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function returns the PLL clock source
 * @retval     the enum of the PLL clock source
 **********************************************************************************************************************/

cgc_clock_t HW_CGC_PLLClockSourceGet (R_SYSTEM_Type * p_system_reg)
{
    /*  PLL source selection only available on S7G2 */
    if (gp_cgc_feature->pll_src_configurable)
    {
        /* Get the PLL clock source */
        if (p_system_reg->PLLCCR_b.PLLSRCSEL == 1U)
        {
            return CGC_CLOCK_HOCO;
        }
    }

    return CGC_CLOCK_MAIN_OSC;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the PLL clock source
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_PLLClockSourceSet (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock)
{    
    /* Set the PLL clock source */
    HW_CGC_HardwareUnLock();
    if (CGC_CLOCK_MAIN_OSC == clock)
    {
        p_system_reg->PLLCCR_b.PLLSRCSEL = CGC_PLL_MAIN_OSC;
    }
    else
    {
        /* The default value is HOCO. */
        p_system_reg->PLLCCR_b.PLLSRCSEL = CGC_PLL_HOCO;
    }
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function sets the PLL multiplier
 * @retval     none
 **********************************************************************************************************************/

/*LDRA_INSPECTED 90 s float used because float32_t is not part of the C99 standard integer definitions. */
void HW_CGC_PLLMultiplierSet (R_SYSTEM_Type * p_system_reg, float multiplier)
{
    /* Set the PLL multiplier */
    SSP_PARAMETER_NOT_USED(multiplier);          /// Prevent compiler 'unused' warning

    if (1U == gp_cgc_feature->pllccr_type)
    {
        uint32_t write_val                  = (uint32_t) (multiplier * 2) - 1;
        HW_CGC_HardwareUnLock();
        p_system_reg->PLLCCR_b.PLLMUL  = (uint8_t)(write_val & 0x3F);
        HW_CGC_HardwareLock();
    }
    if (2U == gp_cgc_feature->pllccr_type)
    {
        uint32_t write_val                  = (uint32_t) multiplier - 1;
        HW_CGC_HardwareUnLock();
        p_system_reg->PLLCCR2_b.PLLMUL = (uint8_t)(write_val & 0x1F);
        HW_CGC_HardwareLock();
    }
}

/*******************************************************************************************************************//**
 * @brief      This function gets the PLL multiplier
 * @retval     float multiplier value
 **********************************************************************************************************************/

/*LDRA_INSPECTED 90 s float used because float32_t is not part of the C99 standard integer definitions. */
float HW_CGC_PLLMultiplierGet (R_SYSTEM_Type * p_system_reg)
{
    /* Get the PLL multiplier */
    /*LDRA_INSPECTED 90 s */
    float pll_mul = 0.0f;

    if (1U == gp_cgc_feature->pllccr_type)
    {
        /* This cast is used for compatibility with the S7 implementation. */
        /*LDRA_INSPECTED 90 s */
        pll_mul = ((float)(p_system_reg->PLLCCR_b.PLLMUL + 1U)) / 2.0f;
    }
    if (2U == gp_cgc_feature->pllccr_type)
    {
        /* This cast is used for compatibility with the S1 and S3 implementation. */
        /*LDRA_INSPECTED 90 s */
        pll_mul = (float) p_system_reg->PLLCCR2_b.PLLMUL + 1.0f;
    }

    return pll_mul;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the PLL divider
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_PLLDividerSet (R_SYSTEM_Type * p_system_reg, cgc_pll_div_t divider)
{
    /* Set the PLL divider */
    if (1U == gp_cgc_feature->pllccr_type)
    {
        uint16_t register_value = g_pllccr_div_setting[divider];
        HW_CGC_HardwareUnLock();
        p_system_reg->PLLCCR_b.PLIDIV  = (uint16_t)(register_value & 0x3);
        HW_CGC_HardwareLock();
    }
    if (2U == gp_cgc_feature->pllccr_type)
    {
        uint16_t register_value = g_pllccr2_div_setting[divider];
        HW_CGC_HardwareUnLock();
        p_system_reg->PLLCCR2_b.PLODIV = (uint8_t)(register_value & 0x3);
        HW_CGC_HardwareLock();
    }
}

/*******************************************************************************************************************//**
 * @brief      This function gets the PLL divider
 * @retval     divider
 **********************************************************************************************************************/

uint16_t HW_CGC_PLLDividerGet (R_SYSTEM_Type * p_system_reg)
{
    /* Get the PLL divider */
    uint16_t ret = 1U;
    if (1U == gp_cgc_feature->pllccr_type)
    {
        /* This cast maps the register value to an enumerated list. */
        ret = g_pllccr_div_value[p_system_reg->PLLCCR_b.PLIDIV];
    }
    if (2U == gp_cgc_feature->pllccr_type)
    {
        /* This cast maps the register value to an enumerated list. */
        ret = g_pllccr2_div_value[p_system_reg->PLLCCR2_b.PLODIV];
    }
    return ret;
}

/*******************************************************************************************************************//**
 * @brief      This function sets the system dividers
 * @param[in]  cfg is a pointer to a cgc_system_clock_cfg_t struct
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_SystemDividersSet (R_SYSTEM_Type * p_system_reg, cgc_system_clock_cfg_t const * const cfg)
{
    sckdivcr_clone_t sckdivcr;
    /* The cgc_sys_clock_div_t fits in the 3 bits, and each of the elements of sckdivcr_b are 3 bits of a 32-bit value,
     * so these casts are safe. */
    sckdivcr.sckdivcr_w = (uint32_t) 0x00;

    if (1U == gp_cgc_feature->has_pclka)
    {
        sckdivcr.sckdivcr_b.pcka = (uint32_t)(cfg->pclka_div & 0x7);
    }
    if (1U == gp_cgc_feature->has_pclkb)
    {
        sckdivcr.sckdivcr_b.pckb = (uint32_t)(cfg->pclkb_div & 0x7);
    }
    if (1U == gp_cgc_feature->has_pclkc)
    {
        sckdivcr.sckdivcr_b.pckc = (uint32_t)(cfg->pclkc_div & 0x7);
    }
    if (1U == gp_cgc_feature->has_pclkd)
    {
        sckdivcr.sckdivcr_b.pckd = (uint32_t)(cfg->pclkd_div & 0x7);
    }
    if (1U == gp_cgc_feature->has_bclk)
    {
        sckdivcr.sckdivcr_b.bck  = (uint32_t)(cfg->bclk_div  & 0x7);
    }
    if (1U == gp_cgc_feature->has_fclk)
    {
        sckdivcr.sckdivcr_b.fck  = (uint32_t)(cfg->fclk_div  & 0x7);
    }

    /* All MCUs have ICLK. */
    sckdivcr.sckdivcr_b.ick  = (uint32_t)(cfg->iclk_div  & 0x7);

    /* Set the system dividers */
    HW_CGC_HardwareUnLock();
    p_system_reg->SCKDIVCR       = sckdivcr.sckdivcr_w;
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function gets the system dividers
 * @param[in]  cfg is a pointer to a cgc_system_clock_cfg_t struct
 * @param[out]  cfg is a pointer to a cgc_system_clock_cfg_t struct
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_SystemDividersGet (R_SYSTEM_Type * p_system_reg, cgc_system_clock_cfg_t * cfg)
{
    /* Get the system dividers */
    /* The cgc_sys_clock_div_t defines all valid values (3 bits each) for these registers as per the hardware manual,
     * and each of the elements of SCKDIVCR_b are 3 bits of a 32-bit value, so these casts are safe. */
    cfg->pclka_div = (cgc_sys_clock_div_t) p_system_reg->SCKDIVCR_b.PCKA;
    cfg->pclkb_div = (cgc_sys_clock_div_t) p_system_reg->SCKDIVCR_b.PCKB;
    cfg->pclkc_div = (cgc_sys_clock_div_t) p_system_reg->SCKDIVCR_b.PCKC;
    cfg->pclkd_div = (cgc_sys_clock_div_t) p_system_reg->SCKDIVCR_b.PCKD;
    cfg->bclk_div  = (cgc_sys_clock_div_t) p_system_reg->SCKDIVCR_b.BCK;
    cfg->fclk_div  = (cgc_sys_clock_div_t) p_system_reg->SCKDIVCR_b.FCK;
    cfg->iclk_div  = (cgc_sys_clock_div_t) p_system_reg->SCKDIVCR_b.ICK;
}

/*******************************************************************************************************************//**
 * @brief      This function tests the clock configuration for validity
 * @param[in]  cfg is a pointer to a cgc_clock_cfg_t struct
 * @retval     true if configuration is valid
 **********************************************************************************************************************/

bool HW_CGC_ClockCfgValidCheck (cgc_clock_cfg_t * cfg)
{
    /* check for valid configuration */

    /* Check maximum and minimum divider values */
    if (gp_cgc_feature->pll_div_max < cfg->divider)
    {
        return false;    // Value is out of range.
    }

    /* Check maximum and minimum multiplier values */
    /* Float used because float32_t is not part of the C99 standard integer definitions. */
    /*LDRA_INSPECTED 90 s *//*LDRA_INSPECTED 90 s */
    if (((float)gp_cgc_feature->pll_mul_max < cfg->multiplier) || ((float)gp_cgc_feature->pll_mul_min > cfg->multiplier))
    {
        return false;   // Value is out of range.
    }

    if ((CGC_CLOCK_MAIN_OSC != cfg->source_clock) && (CGC_CLOCK_HOCO != cfg->source_clock))
    {
        return false;   // Value is out of range.
    }

    return true;
}

/*******************************************************************************************************************//**
 * @brief      This function returns the divider of the selected clock
 * @param[in]  clock is the system clock to get the vivider for
 * @retval     divider
 **********************************************************************************************************************/

uint32_t HW_CGC_ClockDividerGet (R_SYSTEM_Type * p_system_reg, cgc_system_clocks_t clock)
{
    /*  get divider of selected clock */
    uint32_t divider;
    divider = 0U;

    switch (clock)
    {
        case CGC_SYSTEM_CLOCKS_PCLKA:
            divider = p_system_reg->SCKDIVCR_b.PCKA;
            break;

        case CGC_SYSTEM_CLOCKS_PCLKB:
            divider = p_system_reg->SCKDIVCR_b.PCKB;
            break;

        case CGC_SYSTEM_CLOCKS_PCLKC:
            divider = p_system_reg->SCKDIVCR_b.PCKC;
            break;

        case CGC_SYSTEM_CLOCKS_PCLKD:
            divider = p_system_reg->SCKDIVCR_b.PCKD;
            break;

        case CGC_SYSTEM_CLOCKS_BCLK:
            divider = p_system_reg->SCKDIVCR_b.BCK;
            break;

        case CGC_SYSTEM_CLOCKS_FCLK:
            divider = p_system_reg->SCKDIVCR_b.FCK;
            break;

        case CGC_SYSTEM_CLOCKS_ICLK:
            divider = p_system_reg->SCKDIVCR_b.ICK;
            break;
        default:
            break;
    }

    return (divider);
}

/*******************************************************************************************************************//**
 * @brief      This function returns the frequency of the selected clock
 * @param[in]  clock is the system clock to get the freq for
 * @retval     frequency
 **********************************************************************************************************************/

uint32_t HW_CGC_ClockHzGet (R_SYSTEM_Type * p_system_reg, cgc_system_clocks_t clock)
{
    /*  get frequency of selected clock */
    uint32_t divider;
    divider =  HW_CGC_ClockDividerGet(p_system_reg, clock);
    return (uint32_t) ((g_clock_freq[HW_CGC_ClockSourceGet(p_system_reg)]) >> divider);
}

/*******************************************************************************************************************//**
 * @brief      This function returns the frequency of the selected clock
 * @param[in]  clock is the system clock, such as iclk or fclk, to get the freq for
 * @param[in]  source_clock is the source clock, such as the main osc or PLL
 * @retval     frequency
 **********************************************************************************************************************/

uint32_t HW_CGC_ClockHzCalculate (cgc_clock_t source_clock,  cgc_sys_clock_div_t divider)
{
    /*  calculate frequency of selected system clock, given the source clock and divider */
    return (uint32_t) ((g_clock_freq[source_clock]) >> (uint32_t)divider);
}

/*******************************************************************************************************************//**
 * @brief      This function changes the operating power control mode
 * @param[in]  none
 * @retval     None
 **********************************************************************************************************************/

void HW_CGC_OperatingModeSet (R_SYSTEM_Type * p_system_reg, cgc_operating_modes_t operating_mode)
{
    bsp_cache_state_t cache_state;

    /** Enable writing to OPCCR and SOPCCR registers. */
    HW_CGC_LPMHardwareUnLock();

    cache_state = BSP_CACHE_STATE_OFF;
    R_BSP_CacheOff(&cache_state);                           // Turn the cache off.

    while ((0U != p_system_reg->SOPCCR_b.SOPCMTSF) || (0U != p_system_reg->OPCCR_b.OPCMTSF))
    {
        /** Wait for transition to complete. */
    }

    /** The Sub-osc bit has to be cleared first. */
    p_system_reg->SOPCCR_b.SOPCM = CGC_SOPCCR_CLEAR_SUBOSC_SPEED_MODE;
    while ((0U != p_system_reg->SOPCCR_b.SOPCMTSF) || (0U != p_system_reg->OPCCR_b.OPCMTSF))
    {
        /** Wait for transition to complete. */
    }

    /** Set OPCCR. */
    if(CGC_SUBOSC_SPEED_MODE == operating_mode)
    {
        p_system_reg->OPCCR_b.OPCM = CGC_OPPCR_OPCM_MASK & CGC_OPCCR_LOW_SPEED_MODE;
    }
    else
    {
        p_system_reg->OPCCR_b.OPCM = CGC_OPPCR_OPCM_MASK & (uint32_t)operating_mode;
    }
    while ((0U != p_system_reg->SOPCCR_b.SOPCMTSF) || (0U != p_system_reg->OPCCR_b.OPCMTSF))
    {
        /** Wait for transition to complete. */
    }

    /** Set SOPCCR. */
    if(CGC_SUBOSC_SPEED_MODE == operating_mode)
    {
        p_system_reg->SOPCCR_b.SOPCM = CGC_SOPPCR_SOPCM_MASK & CGC_SOPCCR_SET_SUBOSC_SPEED_MODE;
    }
    else
    {
        p_system_reg->SOPCCR_b.SOPCM = CGC_SOPPCR_SOPCM_MASK & CGC_SOPCCR_CLEAR_SUBOSC_SPEED_MODE;
    }
    while ((0U != p_system_reg->SOPCCR_b.SOPCMTSF) || (0U != p_system_reg->OPCCR_b.OPCMTSF))
    {
        /** Wait for transition to complete. */
    }

    R_BSP_CacheSet(cache_state);                            // Restore cache to previous state.

    /** Disable writing to OPCCR and SOPCCR registers. */
    HW_CGC_LPMHardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function checks the MCU for High Speed Mode
 * @param[in]  none
 * @retval     true if MCU is in High Speed Mode
 **********************************************************************************************************************/

cgc_operating_modes_t HW_CGC_OperatingModeGet (R_SYSTEM_Type * p_system_reg)
{
    cgc_operating_modes_t operating_mode = CGC_HIGH_SPEED_MODE;
    if (1U == p_system_reg->SOPCCR_b.SOPCM)
    {
        operating_mode = CGC_SUBOSC_SPEED_MODE;
    }
    else
    {
        operating_mode = (cgc_operating_modes_t)p_system_reg->OPCCR_b.OPCM;
    }
    return operating_mode;
}

/*******************************************************************************************************************//**
 * @brief      This function enables the osc stop detection function and interrupt
 * @retval     none
 **********************************************************************************************************************/
void HW_CGC_OscStopDetectEnable (R_SYSTEM_Type * p_system_reg)
{
    /* enable hardware oscillator stop detect function */
    HW_CGC_HardwareUnLock();
    p_system_reg->OSTDCR_b.OSTDE  = 1U;        // enable osc stop detection
    p_system_reg->OSTDCR_b.OSTDIE = 1U;        // enable osc stop detection interrupt
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function disables the osc stop detection function and interrupt
 * @retval     none
 **********************************************************************************************************************/
void HW_CGC_OscStopDetectDisable (R_SYSTEM_Type * p_system_reg)
{
    /* disable hardware oscillator stop detect function */
    HW_CGC_HardwareUnLock();
    p_system_reg->OSTDCR_b.OSTDIE = 0U;        // disable osc stop detection interrupt
    p_system_reg->OSTDCR_b.OSTDE  = 0U;        // disable osc stop detection
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function returns the status of the stop detection function
 * @retval     true if detected, false if not detected
 **********************************************************************************************************************/
bool HW_CGC_OscStopDetectEnabledGet (R_SYSTEM_Type * p_system_reg)
{
    if (p_system_reg->OSTDCR_b.OSTDE == 1U)          // is stop detection enabled?
    {
        return true;                       // Function enabled.
    }
    return false;                          // Function not enabled.

}
/*******************************************************************************************************************//**
 * @brief      This function returns the status of the stop detection function
 * @retval     true if detected, false if not detected
 **********************************************************************************************************************/

bool HW_CGC_OscStopDetectGet (R_SYSTEM_Type * p_system_reg)
{
    /* oscillator stop detected */
    if (p_system_reg->OSTDSR_b.OSTDF == 1U)
    {
        return true;            // stop detected
    }

    return false;               // no stop detected
}

/*******************************************************************************************************************//**
 * @brief      This function clear the status of the stop detection function
 * @retval     none
 **********************************************************************************************************************/

bool HW_CGC_OscStopStatusClear (R_SYSTEM_Type * p_system_reg)
{
    /* clear hardware oscillator stop detect status */
    if (p_system_reg->OSTDSR_b.OSTDF == 1U)
    {
        HW_CGC_HardwareUnLock();
        p_system_reg->OSTDSR_b.OSTDF = 0U;
        HW_CGC_HardwareLock();
        return true;            // stop detection cleared
    }

    return false;               // can't clear flag because no stop detected
}

/*******************************************************************************************************************//**
 * @brief      This function configures the BusClock Out divider
 * @param[in]  divider is the bus clock out divider
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_BusClockOutCfg (R_SYSTEM_Type * p_system_reg, cgc_bclockout_dividers_t divider)
{
    /*  */
    HW_CGC_HardwareUnLock();
    p_system_reg->BCKCR_b.BCLKDIV = divider;      // set external bus clock output divider
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function enables BusClockOut
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_BusClockOutEnable (R_SYSTEM_Type * p_system_reg)
{
    /*  */
    HW_CGC_HardwareUnLock();
    p_system_reg->EBCKOCR_b.EBCKOEN = 1U;           // enable bus clock output
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function disables BusClockOut
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_BusClockOutDisable (R_SYSTEM_Type * p_system_reg)
{
    /*  */
    HW_CGC_HardwareUnLock();
    p_system_reg->EBCKOCR_b.EBCKOEN = 0U;           // disable bus clock output (fixed high level)
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function configures the ClockOut divider and clock source
 * @param[in]  clock is the clock out source
 * @param[in]  divider is the clock out divider
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_ClockOutCfg (R_SYSTEM_Type * p_system_reg, cgc_clock_t clock, cgc_clockout_dividers_t divider)
{
    /*  */
    HW_CGC_HardwareUnLock();
    p_system_reg->CKOCR_b.CKOEN  = 0U;            // disable CLKOUT to change configuration
    p_system_reg->CKOCR_b.CKODIV = divider;       // set CLKOUT dividers
    p_system_reg->CKOCR_b.CKOSEL = clock;         // set CLKOUT clock source
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function enables ClockOut
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_ClockOutEnable (R_SYSTEM_Type * p_system_reg)
{
    /* Enable CLKOUT output  */
    HW_CGC_HardwareUnLock();
    p_system_reg->CKOCR_b.CKOEN = 1U;
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function disables ClockOut
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_ClockOutDisable (R_SYSTEM_Type * p_system_reg)
{
    /* Disable CLKOUT output */
    HW_CGC_HardwareUnLock();
    p_system_reg->CKOCR_b.CKOEN = 0U;
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function enables SDRAM ClockOut
 * @retval     none
 **********************************************************************************************************************/
void HW_CGC_SDRAMClockOutEnable (R_SYSTEM_Type * p_system_reg)
{
    HW_CGC_HardwareUnLock();
    p_system_reg->SDCKOCR_b.SDCKOEN = 1U;          // enable SDRAM output
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function disables SDRAM ClockOut
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_SDRAMClockOutDisable (R_SYSTEM_Type * p_system_reg)
{
    HW_CGC_HardwareUnLock();
    p_system_reg->SDCKOCR_b.SDCKOEN = 0U;          // disable SDRAM output (fixed high level)
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This function configures the USB clock divider
 * @param[in]  divider is the USB clock divider
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_USBClockCfg (R_SYSTEM_Type * p_system_reg, cgc_usb_clock_div_t divider)
{
    HW_CGC_HardwareUnLock();
    p_system_reg->SCKDIVCR2_b.UCK = divider;      // set USB divider
    HW_CGC_HardwareLock();
}

/*******************************************************************************************************************//**
 * @brief      This updates the Systick timer period
 * @param[in]  uint32_t reload_value - Reload value, calculated by caller
 * @retval     none
 **********************************************************************************************************************/
bool HW_CGC_SystickUpdate(uint32_t reload_value)
{
    bool result = false;
    /* SysTick is defined by CMSIS and will not be modified. */
    /*LDRA_INSPECTED 93 S *//*LDRA_INSPECTED 96 S *//*LDRA_INSPECTED 330 S *//*LDRA_INSPECTED 330 S */
    uint32_t systick_ctrl = SysTick->CTRL;

    /** If there is an RTOS in place AND the Systick interrupt is not yet enabled, just return and do nothing */
#if (1 == BSP_CFG_RTOS)
    /*LDRA_INSPECTED 93 S *//*LDRA_INSPECTED 96 S *//*LDRA_INSPECTED 330 S *//*LDRA_INSPECTED 330 S */
    if ((SysTick->CTRL & SysTick_CTRL_TICKINT_Msk) == 0)
    {
        return(true);           ///< Not really an error.
    }
#endif
    /*LDRA_INSPECTED 93 S *//*LDRA_INSPECTED 96 S *//*LDRA_INSPECTED 330 S *//*LDRA_INSPECTED 330 S */
    SysTick->CTRL = 0U;          ///< Disable systick while we reset the counter

    // Number of ticks between two interrupts.
    /* Save the Priority for Systick Interrupt, will need to be restored after SysTick_Config() */
    uint32_t systick_priority = NVIC_GetPriority (SysTick_IRQn);
    if (0U == SysTick_Config(reload_value))
    {
        result = true;
        NVIC_SetPriority (SysTick_IRQn, systick_priority); ///< Restore systick priority
    }
    else
    {
        /*LDRA_INSPECTED 93 S *//*LDRA_INSPECTED 96 S *//*LDRA_INSPECTED 330 S *//*LDRA_INSPECTED 330 S */
        SysTick->CTRL = systick_ctrl;          ///< If we were provided an invalid (ie too large) reload value,
                                               ///< keep using prior value.
    }
    return(result);
}


/*******************************************************************************************************************//**
 * @brief      This function configures the Segment LCD clock
 * @param[in]  clock is the LCD clock source
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_LCDClockCfg (R_SYSTEM_Type * p_system_reg, uint8_t clock)
{    
    p_system_reg->SLCDSCKCR_b.LCDSCKSEL = clock & 0xFU;
}

/*******************************************************************************************************************//**
 * @brief      This function returns the Segment LCD clock configuration
 * @retval     LCD clock configuration
 **********************************************************************************************************************/

uint8_t HW_CGC_LCDClockCfgGet (R_SYSTEM_Type * p_system_reg)
{
    return p_system_reg->SLCDSCKCR_b.LCDSCKSEL;
}

/*******************************************************************************************************************//**
 * @brief      This function enables LCD Clock Out
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_LCDClockEnable (R_SYSTEM_Type * p_system_reg)
{
    p_system_reg->SLCDSCKCR_b.LCDSCKEN = 1U;
}

/*******************************************************************************************************************//**
 * @brief      This function disables LCD Clock Out
 * @retval     none
 **********************************************************************************************************************/

void HW_CGC_LCDClockDisable (R_SYSTEM_Type * p_system_reg)
{
    p_system_reg->SLCDSCKCR_b.LCDSCKEN = 0U;
}

/*******************************************************************************************************************//**
 * @brief      This function queries LCD clock to see if it is enabled.
 * @retval     true if enabled, false if disabled
 **********************************************************************************************************************/

bool HW_CGC_LCDClockIsEnabled (R_SYSTEM_Type * p_system_reg)
{
    if (p_system_reg->SLCDSCKCR_b.LCDSCKEN == 1U)
    {
        return true;
    }
    else
    {
        return false;
    }
}


