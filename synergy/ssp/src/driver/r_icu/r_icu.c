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
 * File Name    : r_icu.c
 * Description  : ICU functions used to implement various interrupt interfaces.
 **********************************************************************************************************************/



/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "r_icu.h"
#include "r_icu_cfg.h"
#include "hw/hw_icu_private.h"
#include "r_icu_private_api.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** "ICU" in ASCII, used to determine if channel is open. */
#define ICU_OPEN                (0x00494355U)

/** Macro for error logger. */
#ifndef ICU_ERROR_RETURN
/*LDRA_INSPECTED 77 S This macro does not work when surrounded by parentheses. */
#define ICU_ERROR_RETURN(a, err) SSP_ERROR_RETURN((a), (err), &g_module_name[0], &g_icu_version)
#endif

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
void icu_irq_isr (void);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
#if defined(__GNUC__)
/* This structure is affected by warnings from a GCC compiler bug.  This pragma suppresses the warnings in this 
 * structure only.*/
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
/** Version data structure used by error logger macro. */
static const ssp_version_t g_icu_version =
{
    .api_version_minor  = EXTERNAL_IRQ_API_VERSION_MINOR,
    .api_version_major  = EXTERNAL_IRQ_API_VERSION_MAJOR,
    .code_version_major = ICU_CODE_VERSION_MAJOR,
    .code_version_minor = ICU_CODE_VERSION_MINOR
};
#if defined(__GNUC__)
/* Restore warning settings for 'missing-field-initializers' to as specified on command line. */
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic pop
#endif

/** Name of module used by error logger macro */
static const char g_module_name[] = "icu";

/**********************************************************************************************************************
 * Global Variables
 **********************************************************************************************************************/
/* ICU BUTTON Driver  */
/*LDRA_INSPECTED 27 D This structure must be accessible in user code. It cannot be static. */
const external_irq_api_t g_external_irq_on_icu =
{
    .open          = R_ICU_ExternalIrqOpen,
    .enable        = R_ICU_ExternalIrqEnable,
    .disable       = R_ICU_ExternalIrqDisable,
    .triggerSet    = R_ICU_ExternalIrqTriggerSet,
    .filterEnable  = R_ICU_ExternalIrqFilterEnable,
    .filterDisable = R_ICU_ExternalIrqFilterDisable,
    .close         = R_ICU_ExternalIrqClose,
    .versionGet    = R_ICU_ExternalIrqVersionGet
};

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/
/*******************************************************************************************************************//**
 * @addtogroup ICU
 * @{
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief  Configure an external input pin for use with the button interface.  Implements external_irq_api_t::open.
 *
 * The Open function is responsible for preparing an external input pin for operation. After completion
 * of the Open function the external input pin shall be enabled and ready to service interrupts. This
 * function must be called once prior to calling any other external input pin API functions. Once
 * successfully completed, the status of the selected external input pin will be set to "open". After
 * that this function should not be called again for the same external input pin without first
 * performing a "close" by calling R_ICU_ExternalIrqClose().
 *
 * @retval SSP_SUCCESS            Command successfully completed.
 * @retval SSP_ERR_ASSERTION      One of the following is invalid:
 *                                  - p_ctrl or p_cfg is NULL
 *                                  - The channel requested in p_cfg is not available on the device selected in
 *                                    r_bsp_cfg.h.
 * @retval SSP_ERR_INVALID_ARGUMENT   p_cfg->p_callback is not NULL, but ISR is not enabled. ISR must be enabled to
 *                                    use callback function.  Enable channel's overflow ISR in bsp_irq_cfg.h.
 * @retval SSP_ERR_IN_USE         The channel specified has already been opened. No configurations were changed. Call
 *                                the associated Close function to reconfigure the channel.
 *
 * @note This function is reentrant for different channels.  It is not reentrant for the same channel.
 **********************************************************************************************************************/
ssp_err_t R_ICU_ExternalIrqOpen (external_irq_ctrl_t      * const p_api_ctrl,
                                 external_irq_cfg_t const * const p_cfg)
{
    icu_instance_ctrl_t * p_ctrl = (icu_instance_ctrl_t *) p_api_ctrl;

#if ICU_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(NULL != p_cfg);
    SSP_ASSERT(NULL != p_ctrl);
#endif

    SSP_PARAMETER_NOT_USED(g_module_name);

    ssp_feature_t ssp_feature = {{(ssp_ip_t) 0U}};
    ssp_feature.channel = 0U;                       // Register base address is at channel 0
    ssp_feature.unit = 0U;
    ssp_feature.id = SSP_IP_ICU;
    fmi_feature_info_t info = {0U};
    ssp_err_t err = g_fmi_on_fmi.productFeatureGet(&ssp_feature, &info);
    ICU_ERROR_RETURN(SSP_SUCCESS == err, err);
    p_ctrl->p_reg = (R_ICU_Type *) info.ptr;

#if ICU_CFG_PARAM_CHECKING_ENABLE
    /* Maximum number of channels on any device so far is 16, so this cast is safe. */
    uint8_t max_channels = (uint8_t) ((((info.variant_data >> 2) & 1U) + 1U) * 8U);
    ICU_ERROR_RETURN(p_cfg->channel < max_channels, SSP_ERR_IP_CHANNEL_NOT_PRESENT);
#endif /* if ICU_CFG_PARAM_CHECKING_ENABLE */

    ssp_vector_info_t * p_vector_info;
    fmi_event_info_t event_info = {(IRQn_Type) 0U};
    err = g_fmi_on_fmi.eventInfoGet(&ssp_feature, (ssp_signal_t) p_cfg->channel, &event_info);
    p_ctrl->irq = event_info.irq;
    if (SSP_INVALID_VECTOR != p_ctrl->irq)
    {
        R_SSP_VectorInfoGet(p_ctrl->irq, &p_vector_info);
        NVIC_SetPriority(p_ctrl->irq, p_cfg->irq_ipl);
        *(p_vector_info->pp_ctrl) = p_ctrl;
    }

    /** Verify channel is not already used */
    ssp_feature.channel = p_cfg->channel;          // BSP hardware locks use channel number
    ssp_err_t     bsp_err = R_BSP_HardwareLock(&ssp_feature);
    ICU_ERROR_RETURN((SSP_SUCCESS == bsp_err), SSP_ERR_IN_USE);
    
    if (p_cfg->p_callback)
    {
        /** Store control block for use by ISR */
        p_ctrl->p_callback       = p_cfg->p_callback;
        p_ctrl->p_context        = p_cfg->p_context;

        /** Lookup IRQ number */
        if (SSP_INVALID_VECTOR == p_ctrl->irq)
        {
            /* IRQ is not enabled */
            R_BSP_HardwareUnlock(&ssp_feature);
            ICU_ERROR_RETURN(false, SSP_ERR_INVALID_ARGUMENT);
        }

        /** Enable interrupts */
        if (true == p_cfg->autostart)
        {
            R_BSP_IrqStatusClear(p_ctrl->irq);
            NVIC_ClearPendingIRQ(p_ctrl->irq);
            NVIC_EnableIRQ(p_ctrl->irq);
        }
    }

    /** Initialize control block. */
    p_ctrl->channel    = p_cfg->channel;
    p_ctrl->p_callback = p_cfg->p_callback;
    p_ctrl->p_context  = p_cfg->p_context;

    HW_ICU_FilterDisable(p_ctrl->p_reg, p_ctrl->channel);
    HW_ICU_DivisorSet(p_ctrl->p_reg, p_ctrl->channel, p_cfg->pclk_div);
    HW_ICU_TriggerSet(p_ctrl->p_reg, p_ctrl->channel, p_cfg->trigger);

    if (p_cfg->filter_enable)
    {
        HW_ICU_FilterEnable(p_ctrl->p_reg, p_ctrl->channel);
    }

    p_ctrl->open = ICU_OPEN;

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Enable external interrupt for specified channel at NVIC. Implements external_irq_api_t::enable.
 *
 * @retval SSP_SUCCESS         Interrupt disabled successfully.
 * @retval SSP_ERR_ASSERTION   The p_ctrl parameter was null.
 * @retval SSP_ERR_NOT_OPEN    The channel is not opened.
 **********************************************************************************************************************/
ssp_err_t R_ICU_ExternalIrqEnable (external_irq_ctrl_t * const p_api_ctrl)
{
    icu_instance_ctrl_t * p_ctrl = (icu_instance_ctrl_t *) p_api_ctrl;

#if ICU_CFG_PARAM_CHECKING_ENABLE
    /* Check that control block pointer is valid. */
    SSP_ASSERT(NULL != p_ctrl);
    ICU_ERROR_RETURN(ICU_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);
#endif

    ICU_ERROR_RETURN(SSP_INVALID_VECTOR != p_ctrl->irq, SSP_ERR_INTERNAL);

    NVIC_EnableIRQ(p_ctrl->irq);

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Disable external interrupt for specified channel at NVIC. Implements external_irq_api_t::disable.
 *
 * @retval SSP_SUCCESS         Interrupt disabled successfully.
 * @retval SSP_ERR_ASSERTION   The p_ctrl parameter was null.
 * @retval SSP_ERR_NOT_OPEN    The channel is not opened.
 **********************************************************************************************************************/
ssp_err_t R_ICU_ExternalIrqDisable (external_irq_ctrl_t * const p_api_ctrl)
{
    icu_instance_ctrl_t * p_ctrl = (icu_instance_ctrl_t *) p_api_ctrl;

#if ICU_CFG_PARAM_CHECKING_ENABLE
    /* Check that control block pointer is valid. */
    SSP_ASSERT(NULL != p_ctrl);
    ICU_ERROR_RETURN(ICU_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);
#endif

    ICU_ERROR_RETURN(SSP_INVALID_VECTOR != p_ctrl->irq, SSP_ERR_INTERNAL);

    NVIC_DisableIRQ(p_ctrl->irq);

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Set trigger value provided. Implements external_irq_api_t::triggerSet.
 *
 * @retval SSP_SUCCESS         Period value written successfully.
 * @retval SSP_ERR_ASSERTION   The p_ctrl or p_period parameter was null.
 * @retval SSP_ERR_NOT_OPEN    The channel is not opened.
 **********************************************************************************************************************/
ssp_err_t R_ICU_ExternalIrqTriggerSet (external_irq_ctrl_t * const p_api_ctrl,
                                       external_irq_trigger_t      hw_trigger)
{
    icu_instance_ctrl_t * p_ctrl = (icu_instance_ctrl_t *) p_api_ctrl;

#if ICU_CFG_PARAM_CHECKING_ENABLE
    /* Check that control block pointer is valid. */
    SSP_ASSERT(NULL != p_ctrl);
    ICU_ERROR_RETURN(ICU_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);
#endif

    HW_ICU_TriggerSet(p_ctrl->p_reg, p_ctrl->channel, hw_trigger);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Enable external interrupt digital filter for specified channel. Implements external_irq_api_t::filterEnable.
 *
 * @retval SSP_SUCCESS         Interrupt disabled successfully.
 * @retval SSP_ERR_ASSERTION   The p_ctrl parameter was null.
 * @retval SSP_ERR_NOT_OPEN    The channel is not opened.
 **********************************************************************************************************************/
ssp_err_t R_ICU_ExternalIrqFilterEnable (external_irq_ctrl_t * const p_api_ctrl)
{
    icu_instance_ctrl_t * p_ctrl = (icu_instance_ctrl_t *) p_api_ctrl;

#if ICU_CFG_PARAM_CHECKING_ENABLE
    /* Check that control block pointer is valid. */
    SSP_ASSERT(NULL != p_ctrl);
    ICU_ERROR_RETURN(ICU_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);
#endif

    HW_ICU_FilterEnable(p_ctrl->p_reg, p_ctrl->channel);

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief  Enable external interrupt digital filter for specified channel. Implements external_irq_api_t::filterDisable.
 *
 * @retval SSP_SUCCESS         Interrupt disabled successfully.
 * @retval SSP_ERR_ASSERTION   The p_ctrl parameter was null.
 * @retval SSP_ERR_NOT_OPEN    The channel is not opened.
 **********************************************************************************************************************/
ssp_err_t R_ICU_ExternalIrqFilterDisable (external_irq_ctrl_t * const p_api_ctrl)
{
    icu_instance_ctrl_t * p_ctrl = (icu_instance_ctrl_t *) p_api_ctrl;

#if ICU_CFG_PARAM_CHECKING_ENABLE
    /* Check that control block pointer is valid. */
    SSP_ASSERT(NULL != p_ctrl);
    ICU_ERROR_RETURN(ICU_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);
#endif

    HW_ICU_FilterDisable(p_ctrl->p_reg, p_ctrl->channel);

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief      Set driver version based on compile time macros.  Implements external_irq_api_t::versionGet.
 *
 * @retval     SSP_SUCCESS        Successful close.
 * @retval     SSP_ERR_ASSERTION  The parameter p_version is NULL.
 **********************************************************************************************************************/
ssp_err_t R_ICU_ExternalIrqVersionGet (ssp_version_t * const p_version)
{
#if ICU_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(NULL != p_version);
#endif

    p_version->version_id  = g_icu_version.version_id;

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief      Disable external interrupt. Implements external_irq_api_t::close.
 *
 * @retval     SSP_SUCCESS          Successful close.
 * @retval     SSP_ERR_ASSERTION  The parameter p_ctrl is NULL.
 * @retval     SSP_ERR_NOT_OPEN  The channel is not opened.
 **********************************************************************************************************************/
ssp_err_t R_ICU_ExternalIrqClose (external_irq_ctrl_t * const p_api_ctrl)
{
    icu_instance_ctrl_t * p_ctrl = (icu_instance_ctrl_t *) p_api_ctrl;

#if ICU_CFG_PARAM_CHECKING_ENABLE
    /* Check that control block pointer is valid. */
    SSP_ASSERT(NULL != p_ctrl);
    ICU_ERROR_RETURN(ICU_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);
#endif

    /** Cleanup. Disable interrupt */
    ssp_feature_t ssp_feature = {{(ssp_ip_t) 0U}};
    ssp_feature.channel = p_ctrl->channel;
    ssp_feature.unit = 0U;
    ssp_feature.id = SSP_IP_ICU;
    ssp_vector_info_t * p_vector_info;
    if (SSP_INVALID_VECTOR != p_ctrl->irq)
    {
        NVIC_DisableIRQ(p_ctrl->irq);
        R_SSP_VectorInfoGet(p_ctrl->irq, &p_vector_info);
        *(p_vector_info->pp_ctrl) = NULL;
    }

    /** Release BSP hardware lock */
    R_BSP_HardwareUnlock(&ssp_feature);

    p_ctrl->open = 0U;

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @} (end addtogroup ICU)
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief      ICU External Interrupt ISR.
 *
 * Saves context if RTOS is used, clears interrupts, calls callback if one was provided in the open function,
 * and restores context if RTOS is used.
 **********************************************************************************************************************/
void icu_irq_isr (void)
{
    /* Save context if RTOS is used */
    SF_CONTEXT_SAVE

    R_BSP_IrqStatusClear (R_SSP_CurrentIrqGet());

    ssp_vector_info_t * p_vector_info = NULL;
    R_SSP_VectorInfoGet(R_SSP_CurrentIrqGet(), &p_vector_info);
    icu_instance_ctrl_t * p_ctrl = (icu_instance_ctrl_t *) *(p_vector_info->pp_ctrl);

    if (NULL != p_ctrl->p_callback)
    {
        /** Set data to identify callback to user, then call user callback. */
        external_irq_callback_args_t args;
        args.channel   = p_ctrl->channel;
        args.p_context = p_ctrl->p_context;
        p_ctrl->p_callback(&args);
    }

    /* Restore context if RTOS is used */
    SF_CONTEXT_RESTORE
}

/***********************************************************************************************************************
 * Private Functions
 **********************************************************************************************************************/


