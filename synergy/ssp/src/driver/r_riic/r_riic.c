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
* File Name    : r_riic.c
* Description  : I2C Master mode driver implementation on the RIIC peripheral.
***********************************************************************************************************************/


/***********************************************************************************************************************
Includes
***********************************************************************************************************************/
#include "r_riic.h"
#include "r_riic_private.h"
#include "r_riic_private_api.h"
#include "r_cgc.h"

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @addtogroup RIIC
 * @{
***********************************************************************************************************************/
/** "RIIC" in ASCII, used to determine if channel is open. */
#define RIIC_OPEN               (0x52494943ULL)

/* Variant bit for FM+ support */
#define VARIANT_FMPLUS_MASK     0x04U

/** Macro for error logger. */
#ifndef RIIC_ERROR_RETURN
/*LDRA_INSPECTED 77 S This macro does not work when surrounded by parentheses. */
#define RIIC_ERROR_RETURN(a, err)  SSP_ERROR_RETURN((a), (err), &g_module_name[0], &g_riic_master_version)
#endif

#define I2C_CODE_READ          (0x01U)
#define I2C_CODE_10BIT         (0xF0U)

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Private function prototypes
***********************************************************************************************************************/
/** Internal helper functions */
static void riic_notify                (riic_instance_ctrl_t  * const p_ctrl,
                                        i2c_event_t             const event);
static ssp_err_t riic_clock_settings   (riic_instance_ctrl_t * const p_ctrl);
static ssp_err_t riic_abort_seq_master (riic_instance_ctrl_t * const p_ctrl);

static ssp_err_t riic_transfer_open    (riic_instance_ctrl_t * p_ctrl, i2c_cfg_t const * const p_cfg);

static ssp_err_t r_riic_irq_cfg        (ssp_feature_t * p_feature, ssp_signal_t signal, uint8_t ipl,
                                                                             void * p_ctrl, IRQn_Type * p_irq);

/** Functions that manipulate hardware */
static ssp_err_t riic_param_check     (riic_instance_ctrl_t * const p_ctrl, i2c_cfg_t const   * const p_cfg);
static ssp_err_t riic_open_hw_master  (riic_instance_ctrl_t * const p_ctrl, ssp_feature_t * p_feature);
static void      riic_close_hw_master             (riic_instance_ctrl_t * const p_ctrl);
static void      riic_configure_interrupts_master (riic_instance_ctrl_t  * const p_ctrl);
static void      riic_abort_hw_master             (riic_instance_ctrl_t  * const p_ctrl);
static void      riic_enable_transfer_support_tx  (riic_instance_ctrl_t  * const p_ctrl);
static void      riic_enable_transfer_support_rx  (riic_instance_ctrl_t  * const p_ctrl);
static ssp_err_t riic_run_hw_master               (riic_instance_ctrl_t  * const p_ctrl);
static  void     riic_rxi_read_data               (riic_instance_ctrl_t  * const p_ctrl);
static  void     riic_txi_send_address            (riic_instance_ctrl_t  * const p_ctrl);
static ssp_err_t riic_set_valid_interrupts_priority   (riic_instance_ctrl_t * p_ctrl, i2c_cfg_t const * const p_cfg);
static ssp_err_t riic_transfer_configure_rx           (riic_instance_ctrl_t * p_ctrl, i2c_cfg_t const * const p_cfg);
static ssp_err_t riic_transfer_configure_tx           (riic_instance_ctrl_t * p_ctrl, i2c_cfg_t const * const p_cfg);

/** Interrupt handlers */
static void riic_rxi_master (riic_instance_ctrl_t * p_ctrl);
static void riic_txi_master (riic_instance_ctrl_t * p_ctrl);
static void riic_tei_master (riic_instance_ctrl_t * p_ctrl);
static void riic_err_master (riic_instance_ctrl_t * p_ctrl);

/***********************************************************************************************************************
Private global variables
***********************************************************************************************************************/
/** Name of module used by error logger macro */
#if BSP_CFG_ERROR_LOG != 0
static const char g_module_name[] = "riic";
#endif

#if defined(__GNUC__)
/* This structure is affected by warnings from the GCC compiler bug https:/gcc.gnu.org/bugzilla/show_bug.cgi?id=60784
 * This pragma suppresses the warnings in this structure only, and will be removed when the SSP compiler is updated to
 * v5.3.*/
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
/** Version data structure used by error logger macro. */
static ssp_version_t const g_riic_master_version =
{
    .api_version_minor  = I2C_MASTER_API_VERSION_MINOR,
    .api_version_major  = I2C_MASTER_API_VERSION_MAJOR,
    .code_version_major = RIIC_MASTER_CODE_VERSION_MAJOR,
    .code_version_minor = RIIC_MASTER_CODE_VERSION_MINOR
};
#if defined(__GNUC__)
/* Restore warning settings for 'missing-field-initializers' to as specified on command line. */
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic pop
#endif

/***********************************************************************************************************************
Global variables
***********************************************************************************************************************/

/** RIIC Implementation of I2C device master interface */
/*LDRA_INSPECTED 27 D This structure must be accessible in user code. It cannot be static. */
i2c_api_master_t const g_i2c_master_on_riic =
{
    .open            = R_RIIC_MasterOpen,
    .close           = R_RIIC_MasterClose,
    .read            = R_RIIC_MasterRead,
    .write           = R_RIIC_MasterWrite,
    .reset           = R_RIIC_MasterReset,
    .versionGet      = R_RIIC_MasterVersionGet,
    .slaveAddressSet = R_RIIC_MasterSlaveAddressSet
};

/***********************************************************************************************************************
Functions
***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief   Gets version information and stores it in the provided version struct.
 *
 * @retval  SSP_SUCCESS                 Successful version get.
 * @retval  SSP_ERR_ASSERTION           p_version is NULL.
***********************************************************************************************************************/
ssp_err_t R_RIIC_MasterVersionGet   (ssp_version_t          * const p_version)
{
#if RIIC_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(p_version != NULL);
#endif

    p_version->version_id = g_riic_master_version.version_id;

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief   Opens the I2C device. May power on IIC peripheral and perform initialization described in hardware manual.
 *
 *  This function will reconfigure the clock settings of the peripheral when a device with a lower rate than
 *  previously configured is opened.
 *
 * @retval  SSP_SUCCESS               Requested clock rate was set exactly.
 * @retval  SSP_ERR_ASSERTION         The parameter p_api_ctrl or p_cfg is NULL or clock rate is greater than 1MHz.
 *                                    or p_cfg->p_extend not equal to NULL.
 * @retval  SSP_ERR_IN_USE            Attempted to open an already open device instance.
 * @retval  SSP_ERR_INVALID_ARGUMENT  If fast mode plus is configured and the channel does not support it
 * @retval  SSP_ERR_INVALID_RATE      The requested rate cannot be set.
 * @return                             See @ref Common_Error_Codes for other possible return codes. This function calls
 *                                    * fmi_api_t::productFeatureGet
 *                                    * g_cgc_on_cgc.systemClockFreqGet
***********************************************************************************************************************/
ssp_err_t R_RIIC_MasterOpen         (i2c_ctrl_t             * const p_api_ctrl,
                                     i2c_cfg_t      const   * const p_cfg)
{
    riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) p_api_ctrl;

    ssp_err_t err;

#if RIIC_CFG_PARAM_CHECKING_ENABLE
    err = riic_param_check(p_ctrl, p_cfg);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);
#endif

    ssp_feature_t ssp_feature = {{(ssp_ip_t) 0U}};
    ssp_feature.channel = p_cfg->channel;
    ssp_feature.unit = 0U;
    ssp_feature.id = SSP_IP_IIC;
    fmi_feature_info_t info = {0U};
    err = g_fmi_on_fmi.productFeatureGet(&ssp_feature, &info);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);

    /* If rate is configured as Fast mode plus, check wether the channel supports it */
    if(I2C_RATE_FASTPLUS == p_cfg->rate)
    {
        RIIC_ERROR_RETURN((0U != (info.variant_data & VARIANT_FMPLUS_MASK)), SSP_ERR_INVALID_ARGUMENT);
    }

    err = R_BSP_HardwareLock(&ssp_feature);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);

    p_ctrl->p_reg = (R_IIC0_Type *) info.ptr;

    /* set valid interrupts with user provided priority */
    err = riic_set_valid_interrupts_priority(p_ctrl, p_cfg);
    if(SSP_SUCCESS != err)
    {
        /* Release hardware lock. */
        R_BSP_HardwareUnlock(&ssp_feature);
        return err;
    }

    /* Record the configuration on the device for use later */
    p_ctrl->info = *p_cfg;

    /* Open the hardware in master mode */
    err = riic_open_hw_master(p_ctrl, &ssp_feature);
    if(SSP_SUCCESS != err)
    {
        R_BSP_ModuleStop(&ssp_feature);

        /* Release hardware lock. */
        R_BSP_HardwareUnlock(&ssp_feature);
        return err;
    }

    /*Open the RIIC transfer interface if available */
    err = riic_transfer_open(p_ctrl, p_cfg);
    if(SSP_SUCCESS != err)
    {
        R_BSP_ModuleStop(&ssp_feature);

        /* Release hardware lock. */
        R_BSP_HardwareUnlock(&ssp_feature);
        return err;
    }

    /* Finally, we can consider the device opened */
    p_ctrl->transaction_completed = true;
    p_ctrl->p_buff                = NULL;
    p_ctrl->total                 = 0U;
    p_ctrl->remain                = 0U;
    p_ctrl->loaded                = 0U;
    p_ctrl->read                  = false;
    p_ctrl->restart               = false;
    p_ctrl->err                   = false;
    p_ctrl->restarted             = false;
    p_ctrl->open                  = RIIC_OPEN;

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief   Closes the I2C device. May power down IIC peripheral.
 *
 *  This function will safely terminate any in-progress I2C transfer with the device. If a transfer is aborted, the user
 *  will be notified via callback with an abort event. Since the callback is optional, this function will also return
 *  a specific error code in this situation.
 *
 * @retval  SSP_SUCCESS         Device closed without issue.
 * @retval  SSP_ERR_ASSERTION   p_api_ctrl is NULL.
 * @retval  SSP_ERR_ABORTED     Device was closed while a transfer was in progress.
***********************************************************************************************************************/
ssp_err_t R_RIIC_MasterClose        (i2c_ctrl_t             * const p_api_ctrl)
{
    riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) p_api_ctrl;

#if RIIC_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(p_ctrl != NULL);
#endif

    ssp_err_t err = SSP_SUCCESS;

    /* Check if the device is even open, return an error if not */
    RIIC_ERROR_RETURN(RIIC_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);

    /* Abort an in-progress transfer with this device only */
    err = riic_abort_seq_master(p_ctrl);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);

    /* The device is now considered closed */
    p_ctrl->open = 0U;

    /* Close the handles for the transfer interfaces */
    if (NULL != p_ctrl->info.p_transfer_rx)
    {
        err = p_ctrl->info.p_transfer_rx->p_api->close(p_ctrl->info.p_transfer_rx->p_ctrl);
        RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);
    }

    if (NULL != p_ctrl->info.p_transfer_tx)
    {
        err = p_ctrl->info.p_transfer_tx->p_api->close(p_ctrl->info.p_transfer_tx->p_ctrl);
        RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);
    }

    /* De-configure everything. */
    riic_close_hw_master(p_ctrl);

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief   Performs a read from the I2C device.
 *
 *  This function will fail if there is already an in-progress I2C transfer on the associated channel. Otherwise, the
 *  I2C read operation will begin. When no callback is provided by the user, this function performs a blocking read.
 *  Otherwise, the read operation is non-blocking and the caller will be notified when the operation has finished by
 *  an I2C_EVENT_RX_COMPLETE in the callback.
 *
 * @retval  SSP_SUCCESS             Function executed without issue,
 *                                  if no callback was provided, the process was kicked off.
 * @retval  SSP_ERR_ASSERTION       p_api_ctrl, p_dest or bytes is NULL.
 * @retval  SSP_ERR_IN_USE          Another transfer was in progress.
 * @retval  SSP_ERR_ABORTED         The transfer failed.
***********************************************************************************************************************/
ssp_err_t R_RIIC_MasterRead         (i2c_ctrl_t             * const p_api_ctrl,
                                     uint8_t                * const p_dest,
                                     uint32_t                 const bytes,
                                     bool                     const restart)
{
    riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) p_api_ctrl;

#if RIIC_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(p_ctrl != NULL);
    SSP_ASSERT(p_dest != NULL);
    SSP_ASSERT(bytes != 0U);
#endif

    ssp_err_t err = SSP_SUCCESS;

    /* Check if the device is even open, return an error if not */
    RIIC_ERROR_RETURN(RIIC_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);

    /* Fail if there is already a transfer in progress */
    if (false == p_ctrl->transaction_completed)
    {
        err = SSP_ERR_IN_USE;
    }
    else
    {
        /* Record the new information about this transfer */
        p_ctrl->p_buff  = p_dest;
        p_ctrl->total   = bytes;
        p_ctrl->restart = restart;
        p_ctrl->read    = true;

        /* Handle the different addressing modes */
        if (p_ctrl->info.addr_mode == I2C_ADDR_MODE_7BIT)
        {
            /* Set the address bytes according to a 7-bit slave read command */
            p_ctrl->addr_high  = 0U;
            p_ctrl->addr_low   = (uint8_t) ((p_ctrl->info.slave << 1U) | (uint8_t) I2C_CODE_READ);
            p_ctrl->addr_total = 1U;
        }
        else
        {
            /* Set the address bytes according to a 10-bit slave read command */
            p_ctrl->addr_high  = (uint8_t)(((p_ctrl->info.slave >> 7U) | I2C_CODE_10BIT) & (uint8_t)~I2C_CODE_READ);
            p_ctrl->addr_low   = (uint8_t) p_ctrl->info.slave;
            p_ctrl->addr_total = 3U;
        }

        /* Kickoff the read operation as a master */
        err = riic_run_hw_master(p_ctrl);
    }

    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief   Performs a write to the I2C device.
 *
 *  This function will fail if there is already an in-progress I2C transfer on the associated channel. Otherwise, the
 *  I2C write operation will begin. When no callback is provided by the user, this function performs a blocking write.
 *  Otherwise, the write operation is non-blocking and the caller will be notified when the operation has finished by
 *  an I2C_EVENT_TX_COMPLETE in the callback.
 *
 * @retval  SSP_SUCCESS           Function executed without issue,
 *                                if no callback was provided, the process was kicked off.
 * @retval  SSP_ERR_ASSERTION     p_api_ctrl or p_src is NULL.
 * @retval  SSP_ERR_IN_USE        Another transfer was in progress.
 * @retval  SSP_ERR_ABORTED       The transfer failed.
***********************************************************************************************************************/
ssp_err_t R_RIIC_MasterWrite        (i2c_ctrl_t             * const p_api_ctrl,
                                     uint8_t                * const p_src,
                                     uint32_t                 const bytes,
                                     bool                     const restart)
{
    riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) p_api_ctrl;

#if RIIC_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(p_ctrl != NULL);
    SSP_ASSERT(p_src != NULL);
#endif

    ssp_err_t err = SSP_SUCCESS;

    /* Check if the device is even open, return an error if not */
    RIIC_ERROR_RETURN(RIIC_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);

    /* Fail if there is already a transfer in progress */
    if (false == p_ctrl->transaction_completed)
    {
        err = SSP_ERR_IN_USE;
    }
    else
    {
        /* Record the new information about this transfer */
        p_ctrl->p_buff  = p_src;
        p_ctrl->total   = bytes;
        p_ctrl->remain  = bytes;
        p_ctrl->restart = restart;
        p_ctrl->read    = false;

        /* Handle the different addressing modes */
        if (p_ctrl->info.addr_mode == I2C_ADDR_MODE_7BIT)
        {
            /* Set the address bytes according to a 7-bit slave write command */
            p_ctrl->addr_high  = 0U;
            p_ctrl->addr_low   = (uint8_t) (p_ctrl->info.slave << 1U) & ~I2C_CODE_READ;
            p_ctrl->addr_total = 1U;
        }
        else
        {
            /* Set the address bytes according to a 10-bit slave read command */
            p_ctrl->addr_high  = (uint8_t)((p_ctrl->info.slave >> 7U) | I2C_CODE_10BIT) & (uint8_t)~I2C_CODE_READ;
            p_ctrl->addr_low   = (uint8_t) p_ctrl->info.slave;
            p_ctrl->addr_total = 2U;
        }

        /* Kickoff the write operation as a master */
        err = riic_run_hw_master(p_ctrl);
    }

    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief   Aborts any in-progress transfer and forces the IIC peripheral into a ready state.
 *
 *  This function will safely terminate any in-progress I2C transfer with the device. If a transfer is aborted, the user
 *  will be notified via callback with an abort event. Since the callback is optional, this function will also return
 *  a specific error code in this situation.
 *
 * @retval  SSP_SUCCESS                 Channel was reset without issue.
 * @retval  SSP_ERR_ASSERTION           p_api_ctrl is NULL.
 * @retval  SSP_ERR_ABORTED             A transfer was aborted while resetting the hardware.
***********************************************************************************************************************/
ssp_err_t R_RIIC_MasterReset        (i2c_ctrl_t             * const p_api_ctrl)
{
    riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) p_api_ctrl;

#if RIIC_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(p_ctrl != NULL);
#endif

    /* Check if the device is even open, return an error if not */
    RIIC_ERROR_RETURN(RIIC_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);

    /* Abort any transfer happening on the channel */
    ssp_err_t err = riic_abort_seq_master(p_ctrl);

    return err;
}

/*******************************************************************************************************************//**
 * @brief Sets address and addressing mode of the slave device.
 *
 * This function is used to set the device address and addressing mode of the slave without reconfiguring the entire bus.
 *
 * @retval  SSP_SUCCESS                 Address of the slave is set correctly.
 * @retval  SSP_ERR_ASSERTION           Pointer to control structure is NULL.
 * @retval  SSP_ERR_IN_USE              Another transfer was in-progress.
 * @retval  SSP_ERR_NOT_OPEN            Device was not even opened.
 *
 **********************************************************************************************************************/
ssp_err_t R_RIIC_MasterSlaveAddressSet (i2c_ctrl_t    * const p_api_ctrl,
                                        uint16_t        const slave,
                                        i2c_addr_mode_t const addr_mode)
{
	riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) p_api_ctrl;

    ssp_err_t err = SSP_SUCCESS;

#if RIIC_CFG_PARAM_CHECKING_ENABLE
    SSP_ASSERT(p_ctrl != NULL);
#endif

    /* Check if the device is open, return an error if not */
    RIIC_ERROR_RETURN(RIIC_OPEN == p_ctrl->open, SSP_ERR_NOT_OPEN);

    /* Fail if there is already a transfer in progress */
    if (false == p_ctrl->transaction_completed)
    {
        err = SSP_ERR_IN_USE;
    }
    else
    {
    	/* Sets the address of the slave device */
    	p_ctrl->info.slave     = slave;

    	/* Sets the mode of addressing */
    	p_ctrl->info.addr_mode = addr_mode;
    }

    return err;
}


/*******************************************************************************************************************//**
 * @} (end addtogroup RIIC)
***********************************************************************************************************************/

/***********************************************************************************************************************
Private Functions
***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief   Single point for managing the logic around notifying a transfer has finished.
 *
 * @param[in]       p_ctrl      Pointer to transfer that is ending.
 * @param[in]       event       The event code to pass to the callback.
***********************************************************************************************************************/
static void        riic_notify             (riic_instance_ctrl_t * const p_ctrl,
                                            i2c_event_t            const event)
{
    /* Check if we can notify the caller of the abort via callback */
    if (NULL != p_ctrl->info.p_callback)
    {
        /* Fill in the argument to the callback */
        uint32_t total_bytes = p_ctrl->total;
        i2c_callback_args_t args =
        {
            .p_context  = p_ctrl->info.p_context,
            .bytes      = total_bytes - p_ctrl->remain,
            .event      = event
        };

        /* Now do the callback here */
        p_ctrl->info.p_callback(&args);
    }

    /* Turn off all the interrupts here since this always means the transfer is over */
    HW_RIIC_DisableInts(p_ctrl->p_reg);

    NVIC_DisableIRQ(p_ctrl->eri_irq);
    NVIC_DisableIRQ(p_ctrl->rxi_irq);
    NVIC_DisableIRQ(p_ctrl->tei_irq);
    NVIC_DisableIRQ(p_ctrl->txi_irq);

    /** Set the flag indicating that the transaction is completed*/
    p_ctrl->transaction_completed = true;
}

/*******************************************************************************************************************//**
 * @brief   Configures the clock and filter settings
 *
 * @param[in]       p_ctrl      Pointer to RIIC software control block.
 *
 * @retval      SSP_SUCCESS           Successfully configured the clock
 * @retval      SSP_ERR_INVALID_RATE  Failed to configure the clock settings for this rate
***********************************************************************************************************************/
static ssp_err_t      riic_clock_settings     (riic_instance_ctrl_t * const p_ctrl)
{
	ssp_err_t ssp_err = SSP_ERR_INVALID_RATE;
    const uint32_t  precision = 100U;
	uint32_t divisor = 1U;
    uint32_t period_count32 = 0U;
    uint64_t period_count64 = 0U;
    uint64_t pclk_64 = 0U;
    uint32_t pclk_32 = 0U;
    uint8_t cks_value = 0U;
    uint8_t brch_value = 0U;
    uint8_t brcl_value = 0U;

    ssp_err  = g_cgc_on_cgc.systemClockFreqGet(CGC_SYSTEM_CLOCKS_PCLKB, &pclk_32);
    RIIC_ERROR_RETURN(SSP_SUCCESS == ssp_err, ssp_err);

    /* Convert to a higher integral type for increased precision */
    pclk_64   = (uint64_t)pclk_32 * precision;

    /* Note: When requesting 400 kHz, observed rate is ~365 kHz,
     * while this algorithm suggests we should be able to achieve ~394736kHz */
    while (divisor <= RIIC_MAX_DIVISOR)
    {
        /* Calculate how many counts (at the reference clock frequency)
         * it would take to get close to the requested rate
         * NOTE: p_ctrl->info.rate is not checked for zero here since it is validated in the
         * outer function */
        period_count64 = (pclk_64 / divisor) / (uint64_t)p_ctrl->info.rate;

        /* Reduce the precision back to 32 bit (division with round up)*/
        period_count32 = (uint32_t)((period_count64 + (precision-1)) / precision);

        /* If we can store this number of counts, we are close enough, exit the loop */
        if (period_count32 <= (2U << RIIC_MAX_BITS_BR_REGS))
        {
           break;
        }

        /* Otherwise, divide the clock once and try again */
        divisor <<= 1;
        cks_value++;
    }
    /* Found a good match, proceed with the register settings */
    if (divisor <= RIIC_MAX_DIVISOR)
    {
        /* Stick to a 50/50 duty cycle */
        brcl_value  = (uint8_t) (period_count32 / 2U);
        brch_value  = (uint8_t)(period_count32 - brcl_value);

        /* Refer to HW manual ICBRH settings for the following adjustments */
        if(brch_value > 5U)
        {
        	brcl_value = (uint8_t)(brcl_value - (3U + ((cks_value > 0U) ? 0U : 1U)));
        	brch_value = (uint8_t)(brch_value - (3U + ((cks_value > 0U) ? 0U : 1U)));
        }

        /* Configure the clock and filters */
        HW_RIIC_SetICBRL(p_ctrl->p_reg, brcl_value);
        HW_RIIC_SetICBRH(p_ctrl->p_reg, brch_value);
        HW_RIIC_SetCKS(p_ctrl->p_reg,cks_value);

        /* Disable digital filter */
        HW_RIIC_EnableNFE(p_ctrl->p_reg, 0U);

        ssp_err = SSP_SUCCESS;
    }
    return ssp_err;
}

/*******************************************************************************************************************//**
 * @brief   Single point for managing the logic around aborting a transfer when operating as a master.
 *
 * @param[in]       p_ctrl  Pointer to control struct of specific device
 *
 * @retval  SSP_SUCCESS                 Stopped any DTC assisted transfer.
 * @retval  SSP_ERR_ABORTED             If a transfer is in-progress.
***********************************************************************************************************************/
static ssp_err_t riic_abort_seq_master (riic_instance_ctrl_t * const p_ctrl)
{
    /* Stop any DTC assisted transfer for tx */
    const transfer_instance_t * p_transfer_tx = p_ctrl->info.p_transfer_tx;
    if((NULL != p_transfer_tx) && (!p_ctrl->read))
    {
        transfer_properties_t info;
        info.in_progress = false;
        p_transfer_tx->p_api->infoGet(p_transfer_tx->p_ctrl,&info);
        if(true == info.in_progress)
        {
            p_transfer_tx->p_api->stop(p_transfer_tx->p_ctrl);

            /* There was a transfer in progress that we aborted,set the remaining count
             * to non zero, so that we can continue with processing the error.
             */
            p_ctrl->remain = 1U;
        }
    }

    /* Stop any DTC assisted transfer for rx */
    const transfer_instance_t * p_transfer_rx = p_ctrl->info.p_transfer_rx;
    if((NULL != p_transfer_rx) && (p_ctrl->read))
    {
        transfer_properties_t info;
        info.in_progress = false;
        p_transfer_rx->p_api->infoGet(p_transfer_rx->p_ctrl,&info);
        if(true == info.in_progress)
        {
            p_transfer_rx->p_api->stop(p_transfer_rx->p_ctrl);

            /* There was a transfer in progress that we aborted,set the remaining count
             * to non zero, so that we can continue with processing the error
             */
            p_ctrl->remain = 1U;
        }
    }

    /* Check if there is an in-progress transfer associated with the match */
    if ((0U != p_ctrl->remain) || (p_ctrl->restarted))
    {
        /* Safely stop the hardware from operating */
        riic_abort_hw_master(p_ctrl);

        /* Notify anyone waiting that the transfer is finished */
        riic_notify(p_ctrl, I2C_EVENT_ABORTED);

        /* Update the transfer descriptor to show no longer in-progress and an error */
        p_ctrl->remain = 0U;
        p_ctrl->err    = true;

        /* Update the transfer descriptor to make sure interrupts no longer process */
        p_ctrl->addr_loaded = p_ctrl->addr_total;
        p_ctrl->loaded      = p_ctrl->total;
        p_ctrl->restarted   = false;

        /* Notify the caller via error code since the callback is optional */
        return SSP_ERR_ABORTED;
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Configures RIIC related transfer drivers (if enabled).
 *
 * @param[in]   p_ctrl    Pointer to IIC specific control structure
 * @param[in]   p_cfg     Pointer to IIC specific configuration structure
 *
 * @retval      SSP_SUCCESS       Transfer interface initialized successfully.
 * @retval      SSP_ERR_ASSERTION          Pointer to transfer instance for I2C receive in p_cfg is NULL.
 * @retval      SSP_ERR_IRQ_BSP_DISABLED   Interrupt does not exist in the vector table.
 * @return                  See @ref Common_Error_Codes for other possible return codes. This function internally calls
 *                        * fmi_api_t::productFeatureGet
 **********************************************************************************************************************/
static ssp_err_t    riic_transfer_open     (riic_instance_ctrl_t * p_ctrl, i2c_cfg_t const * const p_cfg)
{
    ssp_err_t result = SSP_SUCCESS;

    if (NULL != p_cfg->p_transfer_rx)
    {
        result = riic_transfer_configure_rx(p_ctrl, p_cfg);
    }

    if (NULL != p_cfg->p_transfer_tx)
    {
        result = riic_transfer_configure_tx(p_ctrl, p_cfg);
    }

    RIIC_ERROR_RETURN(SSP_SUCCESS == result, result);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief   Performs the hardware initialization sequence when operating as a master.
 *
 * @param[in]       p_ctrl      Pointer to control struct of specific device
 * @param[in]       p_feature   SSP Feature
 *
 * @retval          SSP_SUCCESS           Hardware initialized with proper configurations.
 * @retval          SSP_ERR_INVALID_RATE  The requested rate could not be set.
 **********************************************************************************************************************/
static ssp_err_t riic_open_hw_master       (riic_instance_ctrl_t * const p_ctrl, ssp_feature_t * p_feature)
{
	ssp_err_t ssp_err = SSP_SUCCESS;

    /* Perform the first part of the initialization sequence */
    R_BSP_ModuleStart(p_feature);

    /* Perform IIC reset */
    HW_RIIC_Output(p_ctrl->p_reg, false);
    HW_RIIC_Reset(p_ctrl->p_reg, true);

    /* Clear all IRQs */
    HW_RIIC_ClearIrqs(p_ctrl->p_reg);
    R_BSP_IrqStatusClear(p_ctrl->eri_irq);
    NVIC_ClearPendingIRQ(p_ctrl->eri_irq);
    R_BSP_IrqStatusClear(p_ctrl->rxi_irq);
    NVIC_ClearPendingIRQ(p_ctrl->rxi_irq);
    R_BSP_IrqStatusClear(p_ctrl->tei_irq);
    NVIC_ClearPendingIRQ(p_ctrl->tei_irq);
    R_BSP_IrqStatusClear(p_ctrl->txi_irq);
    NVIC_ClearPendingIRQ(p_ctrl->txi_irq);

    /* Come out of IIC reset to internal reset */
    HW_RIIC_Output(p_ctrl->p_reg, true);

    /* Configure the clock settings */
    ssp_err = riic_clock_settings(p_ctrl);
    RIIC_ERROR_RETURN(SSP_SUCCESS == ssp_err, ssp_err);

    /* Enable FM+ slope circuit if fast mode plus is enabled */
    if(I2C_RATE_FASTPLUS == p_ctrl->info.rate)
    {
        HW_RIIC_FMPSlopeCircuit(p_ctrl->p_reg, true);
    }

    /* Allow timeouts to be generated on the low value of SCL using short count mode */
    HW_RIIC_TimeoutCfg(p_ctrl->p_reg, RIIC_TIMEOUT_ON_SCL_LOW, true);

    /* Enable master arbitration loss detection */
    HW_RIIC_EnableMALE(p_ctrl->p_reg, true);

    /* Enable NACK arbitration loss detection */
    HW_RIIC_EnableNALE(p_ctrl->p_reg, true);

    /* Disable Slave arbitration loss detection */
    HW_RIIC_EnableSALE(p_ctrl->p_reg, false);

    /* Enable NACK reception transfer suspension */
    HW_RIIC_EnableNACKE(p_ctrl->p_reg, true);

    /* Setup the interrupts for master mode */
    riic_configure_interrupts_master(p_ctrl);

    /* Release IIC from internal reset */
    HW_RIIC_Reset(p_ctrl->p_reg, false);

    return ssp_err;
}

/*******************************************************************************************************************//**
 * @brief   Performs the hardware initialization sequence when operating as a master.
 *
 * @param[in]       p_ctrl  Pointer to control struct of specific device
***********************************************************************************************************************/
static void        riic_close_hw_master    (riic_instance_ctrl_t * const p_ctrl)
{
    /* De-configure the interrupts used in master mode */
    HW_RIIC_DisableInts(p_ctrl->p_reg);

    NVIC_DisableIRQ(p_ctrl->eri_irq);
    NVIC_DisableIRQ(p_ctrl->rxi_irq);
    NVIC_DisableIRQ(p_ctrl->tei_irq);
    NVIC_DisableIRQ(p_ctrl->txi_irq);

    /* Shutdown the hardware to save some power */
    ssp_feature_t ssp_feature = {{(ssp_ip_t) 0U}};
    ssp_feature.id = SSP_IP_IIC;
    ssp_feature.channel = p_ctrl->info.channel;
    ssp_feature.unit = 0U;
    R_BSP_ModuleStop(&ssp_feature);

    /* Release hardware lock. */
    R_BSP_HardwareUnlock(&ssp_feature);
}

/*******************************************************************************************************************//**
 * @brief   Enables and assigns the interrupts to be used in master mode.
 *
 * @param[in]       p_ctrl  Pointer to control struct of specific device
***********************************************************************************************************************/
static void        riic_configure_interrupts_master    (riic_instance_ctrl_t * const p_ctrl)
{
    /* These are the interrupts we will accept */
    HW_RIIC_ErrIntEn(p_ctrl->p_reg, p_ctrl->eri_irq, true);
    HW_RIIC_RxFullIntEn(p_ctrl->p_reg, p_ctrl->rxi_irq, true);
    HW_RIIC_TxEndIntEn(p_ctrl->p_reg, p_ctrl->tei_irq, false);
    HW_RIIC_TxEmptyIntEn(p_ctrl->p_reg, p_ctrl->txi_irq, true);
}

/*******************************************************************************************************************//**
 * @brief   Safely stops the current data transfer when operating as a master.
 *
 * @param[in]       p_ctrl  Pointer to control struct of specific device
***********************************************************************************************************************/
static void        riic_abort_hw_master    (riic_instance_ctrl_t        * const p_ctrl)
{
    /* Disable channel interrupts */
    HW_RIIC_DisableInts(p_ctrl->p_reg);

    NVIC_DisableIRQ(p_ctrl->eri_irq);
    NVIC_DisableIRQ(p_ctrl->rxi_irq);
    NVIC_DisableIRQ(p_ctrl->tei_irq);
    NVIC_DisableIRQ(p_ctrl->txi_irq);

    /* Check if we are the active master on the bus right now */
    if (HW_RIIC_ActiveMaster(p_ctrl->p_reg))
    {
        /* Check if the error happened in the middle of a transfer */
        uint8_t bits_pending = HW_RIIC_BitsPending(p_ctrl->p_reg);

        /*HW_RIIC_BitsPending returns value 9 at the end of a data transfer including the acknowledge bit.
        (values of the BC[2:0] bits return to 000b (ie, 9 bits) at the end of a data transfer as per HM)*/
        if (9U != bits_pending)
        {
            /* Manually issue enough clocks to finish the transfer */
            while (bits_pending)
            {
                HW_RIIC_ExtraClock(p_ctrl->p_reg);
                while (!HW_RIIC_ExtraClockDone(p_ctrl->p_reg))
                {
                    /* Waiting for clock cycle to finish */
                }
                bits_pending = (uint8_t) (bits_pending - 1U);
            }

            /* Check if the peripheral is holding the line low */
            if (HW_RIIC_SDALow(p_ctrl->p_reg))
            {
                /* Release the line manually */
                HW_RIIC_SDARelease(p_ctrl->p_reg);
            }
        }

        /* Try to issue the stop condition now */
        HW_RIIC_SendStop(p_ctrl->p_reg);

        /* Allow timeouts to be generated on the low value of SCL using short count mode */
        HW_RIIC_TimeoutCfg(p_ctrl->p_reg, RIIC_TIMEOUT_ON_SCL_LOW, true);

        /* Wait until the stop condition is sent or the hardware detects the line is stuck low */
        volatile bool timed_out = false;
        while ((HW_RIIC_StopPending(p_ctrl->p_reg)) && (!timed_out))
        {
            timed_out = HW_RIIC_TimedOut(p_ctrl->p_reg);
        }

        if (timed_out)
        {
            SSP_ERROR_LOG(SSP_ERR_TIMEOUT, g_module_name, g_riic_master_version);
        }
    }

    /* Do I2C internal reset to clear all the transaction states and release the bus */
    HW_RIIC_Reset(p_ctrl->p_reg, true);

    /* Make sure there are no retained interrupt requests, as per manual */
    HW_RIIC_ClearIrqs(p_ctrl->p_reg);
    R_BSP_IrqStatusClear(p_ctrl->eri_irq);
    NVIC_ClearPendingIRQ(p_ctrl->eri_irq);
    R_BSP_IrqStatusClear(p_ctrl->rxi_irq);
    NVIC_ClearPendingIRQ(p_ctrl->rxi_irq);
    R_BSP_IrqStatusClear(p_ctrl->tei_irq);
    NVIC_ClearPendingIRQ(p_ctrl->tei_irq);
    R_BSP_IrqStatusClear(p_ctrl->txi_irq);
    NVIC_ClearPendingIRQ(p_ctrl->txi_irq);

    /* Release peripheral from internal reset */
    HW_RIIC_Reset(p_ctrl->p_reg, false);
}

/*******************************************************************************************************************//**
 * @brief   Enables the dtc transfer interface for the transmit operation
 *
 * @param[in]       p_ctrl  Pointer to transfer control block
***********************************************************************************************************************/
static void       riic_enable_transfer_support_tx    (riic_instance_ctrl_t  * const p_ctrl)
{
    const transfer_instance_t * p_transfer_tx = p_ctrl->info.p_transfer_tx;

    if((NULL != p_transfer_tx) && (!p_ctrl->read))
    {
        p_transfer_tx->p_api->reset(p_transfer_tx->p_ctrl, (void *) (p_ctrl->p_buff), NULL,
                (uint16_t) (p_ctrl->remain));
        p_ctrl->remain = 0U;
        p_ctrl->loaded = p_ctrl->total;
        p_ctrl->activation_on_txi = true;
    }
}

/*******************************************************************************************************************//**
 * @brief   Enables the dtc transfer interface for the receive operation
 *
 * @param[in]       p_ctrl  Pointer to transfer control block
***********************************************************************************************************************/
static void       riic_enable_transfer_support_rx    (riic_instance_ctrl_t  * const p_ctrl)
{
    const transfer_instance_t * p_transfer_rx = p_ctrl->info.p_transfer_rx;

    /* We can only enable transfer support for n - 3 bytes because of the WAIT insertion
     * and NACK, so we check whether the number of transfers is greater than 3
     */
    if((NULL != p_transfer_rx) && (p_ctrl->read) && (p_ctrl->total > 3U))
    {
        p_transfer_rx->p_api->reset(p_transfer_rx->p_ctrl, NULL, (void *) (p_ctrl->p_buff),
                (uint16_t) (p_ctrl->total - 3));
        p_ctrl->remain = 3U;
        p_ctrl->loaded = p_ctrl->total - 3U;
        p_ctrl->activation_on_rxi = true;
    }
}

/*******************************************************************************************************************//**
 * @brief   Performs the data transfer described by the parameters when operating as a master.
 *
 * @param[in]       p_ctrl  Pointer to control struct of specific device.
 *
 * @retval  SSP_SUCCESS       Data transfer success.
 * @retval  SSP_ERR_IN_USE    If data transfer is in progress.
 * @retval  SSP_ERR_ABORTED   If an error occurred while data transfer.
***********************************************************************************************************************/
static          ssp_err_t   riic_run_hw_master      (riic_instance_ctrl_t    * const p_ctrl)
{
    /* Initialize fields used during transfer */
    p_ctrl->addr_loaded 			= 0U;
    p_ctrl->loaded      			= 0U;
    p_ctrl->remain      			= p_ctrl->total;
    p_ctrl->addr_remain 			= p_ctrl->addr_total;
    p_ctrl->err         			= false;
    p_ctrl->dummy_read_completed 	= false;
    p_ctrl->activation_on_rxi 		= false;
    p_ctrl->activation_on_txi 		= false;
    p_ctrl->transaction_completed 	= false;
    p_ctrl->address_restarted       = false;

    /* Check if this is a new transaction or a continuation */
    if (!p_ctrl->restarted)
    {
        /* If bus is busy, return error */
        RIIC_ERROR_RETURN((!HW_RIIC_BusBusy(p_ctrl->p_reg)), SSP_ERR_IN_USE);

        /* Clear all the interrupts before the transfer */
        HW_RIIC_ClearIrqs(p_ctrl->p_reg);
        R_BSP_IrqStatusClear(p_ctrl->eri_irq);
        NVIC_ClearPendingIRQ(p_ctrl->eri_irq);
        R_BSP_IrqStatusClear(p_ctrl->rxi_irq);
        NVIC_ClearPendingIRQ(p_ctrl->rxi_irq);
        R_BSP_IrqStatusClear(p_ctrl->tei_irq);
        NVIC_ClearPendingIRQ(p_ctrl->tei_irq);
        R_BSP_IrqStatusClear(p_ctrl->txi_irq);
        NVIC_ClearPendingIRQ(p_ctrl->txi_irq);
    }

    /* Allow timeouts to be generated on the low value of SCL using short count mode */
    HW_RIIC_TimeoutCfg(p_ctrl->p_reg, RIIC_TIMEOUT_ON_SCL_LOW, true);

    /* Re-enable the interrupts  */
    riic_configure_interrupts_master(p_ctrl);

    /* If previous transaction did not end with restart, send a start condition */
    if(!p_ctrl->restarted)
    {
        HW_RIIC_SendStart(p_ctrl->p_reg);
    }
    else
    {
        p_ctrl->restarted = false;
    }

    /* Check if we must block until the transfer is done */
    if (NULL == p_ctrl->info.p_callback)
    {
        /* Note: There is a hardware timeout that will allow this loop to exit */
        while (false == p_ctrl->transaction_completed)
        {
            /* The transfer descriptor is updated during interrupt processing */
        }
        if (p_ctrl->err)
        {
            return SSP_ERR_ABORTED;
        }
    }

    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief         Sets interrupt priority and initializes vector info
 * @param[in]     p_feature  SSP feature
 * @param[in]     signal     SSP signal ID
 * @param[in]     ipl        Interrupt priority level
 * @param[in]     p_ctrl     Pointer to driver control block
 * @param[out]    p_irq      Pointer to IRQ for this signal, set here
 *
 * @retval        SSP_SUCCESS               Interrupt enabled
 * @retval        SSP_ERR_IRQ_BSP_DISABLED  Interrupt does not exist in the vector table
 * @return                          See @ref Common_Error_Codes for other possible return codes. This function calls
 *                                * fmi_api_t::eventInfoGet
 **********************************************************************************************************************/
static ssp_err_t r_riic_irq_cfg(ssp_feature_t * p_feature, ssp_signal_t signal, uint8_t ipl,
        void * p_ctrl, IRQn_Type * p_irq)
{
    fmi_event_info_t event_info = {(IRQn_Type) 0U};
    ssp_vector_info_t * p_vector_info;
    ssp_err_t err = g_fmi_on_fmi.eventInfoGet(p_feature, signal, &event_info);
    *p_irq = event_info.irq;
    if (SSP_SUCCESS == err)
    {
        NVIC_SetPriority(*p_irq, ipl);
        R_SSP_VectorInfoGet(*p_irq, &p_vector_info);
        *(p_vector_info->pp_ctrl) = p_ctrl;
    }

    return err;
}

/*******************************************************************************************************************//**
 * @brief   Handles the receive data full interrupt when operating as a master.
 *
 * @param[in]       p_ctrl     The target RIIC block's control block.
***********************************************************************************************************************/
static void riic_rxi_master (riic_instance_ctrl_t * p_ctrl)
{
    /* First receive interrupt: Handle the special case of 1 or 2 byte read here */
    if (false == p_ctrl->dummy_read_completed)
    {
        /* Enable WAIT for 1 or 2 byte read */
        if (2U <= p_ctrl->total)
        {
            HW_RIIC_SetWait(p_ctrl->p_reg);
        }

        /*Enable NACK for 1 byte read*/
        if (1U == p_ctrl->remain)
        {
            HW_RIIC_SetNACKTransmission(p_ctrl->p_reg);
        }
        else
        {
        	HW_RIIC_SetACKTransmission(p_ctrl->p_reg);
        }

        /* Enable transfer support if possible */
        riic_enable_transfer_support_rx(p_ctrl);

        /* do a dummy read to clock the data into the ICDRR */
        volatile uint8_t dummy = HW_RIIC_ReadByte(p_ctrl->p_reg);
        (void)dummy;

        /* Update the counter */
        p_ctrl->dummy_read_completed = true;

    }
    /* If this is the interrupt that got fired after DTC transfer,
     * ignore it as the DTC has already taken care of the data transfer */
    else if (true == p_ctrl->activation_on_rxi)
    {
    	p_ctrl->activation_on_rxi = false;
    }
    /* ICDRR contain valid received data */
    else if (0U < p_ctrl->remain)
    {
        riic_rxi_read_data(p_ctrl);
    }
    else
    {
        /* do nothing */
    }
}

/*******************************************************************************************************************//**
 * @brief   Handles the transmit data empty interrupt when operating as a master.
 *
 * @param[in]       p_ctrl     The target RIIC block's control block.
***********************************************************************************************************************/
static void riic_txi_master (riic_instance_ctrl_t * p_ctrl)
{
    /* Check if we are issuing the slave address */
    if (0U < p_ctrl->addr_remain)
    {
        riic_txi_send_address(p_ctrl);
    }
    else if (!p_ctrl->read)
    {
    	/* If this is the interrupt that got fired after DTC transfer,
    	 * ignore it as the DTC has already taken care of the data transfer */
        if(true == p_ctrl->activation_on_txi)
        {
        	p_ctrl->activation_on_txi = false;
        }
        else if (p_ctrl->remain > 0U)
        {
            /* Write the data to ICDRT register */
            HW_RIIC_WriteByte(p_ctrl->p_reg, p_ctrl->p_buff[p_ctrl->loaded]);

            /* Update the number of bytes remaining for next pass */
            p_ctrl->loaded++;
            p_ctrl->remain--;
        }
        else
        {
            /* Do nothing */
        }

        /* We are done loading ICDRT, wait for TEND to send a stop/restart */
        if (0U == p_ctrl->remain)
        {
            /* Disable the TXI interrupt as we are done with transmit buffer
             * NOTE:Only disable in NVIC, disabling in I2C would cause the
             * restart condition to fail because we are using the buffered
             * interrupt to start the next sequence */
            NVIC_DisableIRQ(p_ctrl->txi_irq);

            /* Enable the transmit end IRQ, to issue a STOP or RESTART */
            HW_RIIC_EnableTENDIRQ(p_ctrl->p_reg, p_ctrl->tei_irq);
        }
    }
    else
    {
        /* do nothing */
    }
}

/*******************************************************************************************************************//**
 * @brief   Handles the transmit end interrupt when operating as a master.
 * @note This interrupt is configured to be generated at the end of last byte of the requested transfer.
 *
 * @param[in]       p_ctrl     The target RIIC block's control block.
***********************************************************************************************************************/
static void riic_tei_master (riic_instance_ctrl_t * p_ctrl)
{
    /* This is a 10 bit address read, issue a restart prior to the last address byte transmission  */
    if ((p_ctrl->read) && (p_ctrl->addr_remain == 1U) && (false == p_ctrl->address_restarted))
    {
        HW_RIIC_SendRestart(p_ctrl->p_reg);
        p_ctrl->address_restarted = true;
    }
    /* we are done with the transfer, send STOP or RESTART */
    else if (0U == p_ctrl->remain)
    {
        /* send RESTART */
        if (p_ctrl->restart)
        {
            /* Make sure that there are no pending txi interrupts prior to issuing restart
             * to prevent the interrupts from getting buffered */
            R_BSP_IrqStatusClear(p_ctrl->txi_irq);
            NVIC_ClearPendingIRQ(p_ctrl->txi_irq);

            HW_RIIC_SendRestart(p_ctrl->p_reg);

            /* Disable timeout */
            HW_RIIC_TimeoutCfg(p_ctrl->p_reg, RIIC_TIMEOUT_OFF, false);

            /* Remember that we issued a restart for the next transfer */
            p_ctrl->restarted = true;
        }
        /* Send STOP */
        else
        {
            HW_RIIC_SendStop(p_ctrl->p_reg);
        }
    }
    else
    {
        /* do nothing */
    }

    /* Clear the flag as described in the HW manual */
    HW_RIIC_ClearTxEnd(p_ctrl->p_reg);

    /* Disable the interrupt as we are done with the transfer */
    HW_RIIC_TxEndIntEn(p_ctrl->p_reg, p_ctrl->tei_irq, false);
}

/*******************************************************************************************************************//**
 * @brief   Handles the error interrupts when operating as a master.
 *
 * @param[in]       p_ctrl  Pointer to transfer control block
***********************************************************************************************************************/
static void riic_err_master (riic_instance_ctrl_t * p_ctrl)
{
    /* Clear the error flags in the peripheral */
    uint8_t errs_events = HW_RIIC_GetAndClearErrsEvents(p_ctrl->p_reg);

    /* If the event was an error event, then handle it */
    if (errs_events & (uint8_t) ((uint8_t) (RIIC_ERR_EVENT_TIMEOUT) | (uint8_t) (RIIC_ERR_EVENT_ARBITRATION_LOSS) |
                                 (uint8_t) (RIIC_ERR_EVENT_NACK)))
    {
         /* Abort an in-progress transfer with the current device */
         riic_abort_seq_master(p_ctrl);
    }

    /* This is a STOP, START or RESTART event.We only need to process these events only at the
     * end of the requisite transfers.
     * NOTE: Do not use p_transfer->loaded or p_transfer->remain to check whether the transfer is
     * completed, since using them would lead to a race condition between txi and eri interrupts in case
     * of one byte transfer which will result in BUS arbitration loss error */
    else if ((errs_events & (uint8_t) RIIC_ERR_EVENT_STOP) ||
            ((p_ctrl->restarted) && (errs_events & (uint8_t) RIIC_ERR_EVENT_START)))
    {
        /* HM: Clear Stop and Nackf bits for processing for next transfer operation */
        HW_RIIC_ClrStopNackf(p_ctrl->p_reg);

        /* Get the correct event to notify the user */
        i2c_event_t event = (p_ctrl->read) ? I2C_EVENT_RX_COMPLETE : I2C_EVENT_TX_COMPLETE;

        /* Notify anyone waiting that the transfer is finished */
        riic_notify(p_ctrl, event);
    }
    else
    {
        /* do nothing */
    }
}

/***********************************************************************************************************************
Interrupt Vectors
***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief   Receive data full interrupt routine.
 *
 * This function implements the RIIC Receive buffer full ISR routine.
 *
***********************************************************************************************************************/
void iic_rxi_isr  (void);
void iic_rxi_isr  (void)
{
    /* Save context if RTOS is used */
    SF_CONTEXT_SAVE

    /* Clear the IR flag */
    R_BSP_IrqStatusClear (R_SSP_CurrentIrqGet());

    ssp_vector_info_t * p_vector_info = NULL;
    R_SSP_VectorInfoGet(R_SSP_CurrentIrqGet(), &p_vector_info);
    riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) *(p_vector_info->pp_ctrl);

    riic_rxi_master(p_ctrl);

    /* Restore context if RTOS is used */
    SF_CONTEXT_RESTORE
}

/*******************************************************************************************************************//**
 * @brief   Transmit data empty interrupt routine.
 *
 * This function implements the Transmit buffer empty ISR routine.
 *
***********************************************************************************************************************/
void iic_txi_isr  (void);
void iic_txi_isr  (void)
{
    /* Save context if RTOS is used */
    SF_CONTEXT_SAVE

    /* Clear the IR flag */
    R_BSP_IrqStatusClear (R_SSP_CurrentIrqGet());

    ssp_vector_info_t * p_vector_info = NULL;
    R_SSP_VectorInfoGet(R_SSP_CurrentIrqGet(), &p_vector_info);
    riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) *(p_vector_info->pp_ctrl);

    riic_txi_master(p_ctrl);

    /* Restore context if RTOS is used */
    SF_CONTEXT_RESTORE
}

/*******************************************************************************************************************//**
 * @brief   Transmit end interrupt routine.
 *
 * This function implements the RIIC Transmission End ISR routine.
 *
***********************************************************************************************************************/
void iic_tei_isr  (void);
void iic_tei_isr  (void)
{
    /* Save context if RTOS is used */
    SF_CONTEXT_SAVE

    /* Clear the IR flag */
    R_BSP_IrqStatusClear (R_SSP_CurrentIrqGet());

    ssp_vector_info_t * p_vector_info = NULL;
    R_SSP_VectorInfoGet(R_SSP_CurrentIrqGet(), &p_vector_info);
    riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) *(p_vector_info->pp_ctrl);

    riic_tei_master(p_ctrl);

    /* Restore context if RTOS is used */
    SF_CONTEXT_RESTORE
}

/*******************************************************************************************************************//**
 * @brief   Error and event interrupt routine.
 *
 * This function implements the RIIC Event/Error.
 *
***********************************************************************************************************************/
void iic_eri_isr (void);
void iic_eri_isr (void)
{
    /* Save context if RTOS is used */
    SF_CONTEXT_SAVE

    /* Clear the IR flag */
    R_BSP_IrqStatusClear (R_SSP_CurrentIrqGet());

    ssp_vector_info_t * p_vector_info = NULL;
    R_SSP_VectorInfoGet(R_SSP_CurrentIrqGet(), &p_vector_info);
    riic_instance_ctrl_t * p_ctrl = (riic_instance_ctrl_t *) *(p_vector_info->pp_ctrl);

    riic_err_master(p_ctrl);

    /* Restore context if RTOS is used */
    SF_CONTEXT_RESTORE
}

/*******************************************************************************************************************//**
 * @brief   Check valid receive data and set WAIT, NACK and STOP/RESTART bit in RXI handler.
 *
 * @param[in]       p_ctrl  Pointer to transfer control block
***********************************************************************************************************************/
static  void     riic_rxi_read_data       (riic_instance_ctrl_t  * const p_ctrl)
{
    /* if next data = (final byte - 2), enable WAIT */
    if (3U == p_ctrl->remain)
    {
        HW_RIIC_SetWait(p_ctrl->p_reg);
    }

    /* if next data = (final byte - 1), enable NACK  */
    else if (2U == p_ctrl->remain)
    {
        HW_RIIC_SetNACKTransmission(p_ctrl->p_reg);
    }

    /* if next data = final byte, send STOP or RESTART */
    else if (1U == p_ctrl->remain)
    {
        if (p_ctrl->restart)
        {
            HW_RIIC_SendRestart(p_ctrl->p_reg);

            /* Remember that we issued a restart when doing the next transfer */
            p_ctrl->restarted = true;
        }
        else
        {
            /* Clear STOP flag and set SP */
            HW_RIIC_ClrStop(p_ctrl->p_reg);
            HW_RIIC_SendStop(p_ctrl->p_reg);
        }
    }
    else
    {
        /* do nothing */
    }

    /* Read the data into the buffer */
    p_ctrl->p_buff[p_ctrl->loaded] = HW_RIIC_ReadByte(p_ctrl->p_reg);

    /* if we are done with the transmission, clear the WAIT bit */
    if (1U == p_ctrl->remain)
    {
        HW_RIIC_ClrWait(p_ctrl->p_reg);
    }

    /* Update the counter values */
    p_ctrl->loaded++;
    p_ctrl->remain--;
}

/*******************************************************************************************************************//**
 * @brief   Write the address byte to the riic bus
 *
 * @param[in]       p_ctrl  Pointer to transfer control block
***********************************************************************************************************************/
static  void     riic_txi_send_address         (riic_instance_ctrl_t  * const p_ctrl)
{
    /* This is a 10 bit read and we have transmitted the low byte, next is restart */
    if((3U == p_ctrl->addr_total) && (2U == p_ctrl->addr_loaded) && (false == p_ctrl->address_restarted))
    {
        /* Enable the transmit end IRQ, so that we can generate a RESTART condition */
        HW_RIIC_EnableTENDIRQ(p_ctrl->p_reg, p_ctrl->tei_irq);
    }
    else
    {
        /* address low byte, this could either be a 7 bit address or low byte of 10 bit address */
        uint8_t address_byte = p_ctrl->addr_low;

        /* 10 bit address, handle accordingly */
        if (p_ctrl->addr_total > 1U)
        {
            /* MSB transfer, send address high byte with with R/W set to 0 */
            if(0U == p_ctrl->addr_loaded)
            {
                address_byte = p_ctrl->addr_high;
            }
            /* MSB transfer after restart of 10 bit read, send high byte with R/W set to 1 */
            else if((2U == p_ctrl->addr_loaded) && (3U == p_ctrl->addr_total))
            {
                address_byte = p_ctrl->addr_high | (uint8_t) I2C_CODE_READ;;
            }
            /* low byte transfer */
            else
            {
                address_byte = p_ctrl->addr_low;
            }
        }

        /* if this is the last address byte, enable transfer */
        if (1U == p_ctrl->addr_remain)
        {
            riic_enable_transfer_support_tx(p_ctrl);
        }

        /* Write the address byte */
        HW_RIIC_WriteByte(p_ctrl->p_reg, address_byte);

        /* Update the number of address bytes loaded for next pass */
        p_ctrl->addr_loaded++;
        p_ctrl->addr_remain--;
    }
}

/*******************************************************************************************************************//**
 * @brief   Set valid interrupts priority
 *
 * @param[in]       p_ctrl                    Pointer to control struct of specific device
 * @param[in]       p_cfg                     Pointer to IIC specific configuration structure
 *
 * @retval          SSP_SUCCESS               Interrupts enabled.
 * @retval          SSP_ERR_IRQ_BSP_DISABLED  Interrupt does not exist in the vector table.
 * @return                          See @ref Common_Error_Codes for other possible return codes. This function calls
 *                                * fmi_api_t::productFeatureGet
***********************************************************************************************************************/
static ssp_err_t riic_set_valid_interrupts_priority   (riic_instance_ctrl_t * p_ctrl, i2c_cfg_t const * const p_cfg)
{
    ssp_err_t err;

    ssp_feature_t ssp_feature = {{(ssp_ip_t) 0U}};
    ssp_feature.channel = p_cfg->channel;
    ssp_feature.unit = 0U;
    ssp_feature.id = SSP_IP_IIC;
    fmi_feature_info_t info = {0U};
    err = g_fmi_on_fmi.productFeatureGet(&ssp_feature, &info);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);

    err = r_riic_irq_cfg(&ssp_feature, SSP_SIGNAL_IIC_RXI, p_cfg->rxi_ipl, p_ctrl, &p_ctrl->rxi_irq);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);
    err = r_riic_irq_cfg(&ssp_feature, SSP_SIGNAL_IIC_TXI, p_cfg->txi_ipl, p_ctrl, &p_ctrl->txi_irq);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);
    err = r_riic_irq_cfg(&ssp_feature, SSP_SIGNAL_IIC_TEI, p_cfg->tei_ipl, p_ctrl, &p_ctrl->tei_irq);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);
    err = r_riic_irq_cfg(&ssp_feature, SSP_SIGNAL_IIC_ERI, p_cfg->eri_ipl, p_ctrl, &p_ctrl->eri_irq);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);

    return err;
}

/*******************************************************************************************************************//**
 * @brief Configures  RIIC RX related transfer.
 * @param[in]     p_ctrl                     Pointer to IIC specific control structure
 * @param[in]     p_cfg                      Pointer to IIC specific configuration structure
 *
 * @retval        SSP_SUCCESS                rx transfer interface in configured with valid parameters.
 * @retval        SSP_ERR_ASSERTION          Pointer to transfer instance for I2C receive in p_cfg is NULL.
 * @retval        SSP_ERR_IRQ_BSP_DISABLED   Interrupt does not exist in the vector table.
 * @return                          See @ref Common_Error_Codes for other possible return codes. This function calls
 *                                * fmi_api_t::productFeatureGet
 **********************************************************************************************************************/
static ssp_err_t riic_transfer_configure_rx   (riic_instance_ctrl_t * p_ctrl, i2c_cfg_t const * const p_cfg)
{
    ssp_err_t err;
    uint8_t * p_buffer = NULL;

    fmi_event_info_t event_info = {(IRQn_Type) 0U};
    ssp_feature_t p_feature = {{(ssp_ip_t) 0U}};
    p_feature.channel = p_cfg->channel;
    p_feature.unit = 0U;
    p_feature.id = SSP_IP_IIC;
    fmi_feature_info_t info = {0U};
    err = g_fmi_on_fmi.productFeatureGet(&p_feature, &info);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);

    /** Set default transfer info and open receive transfer module, if enabled. */
#if (RIIC_CFG_PARAM_CHECKING_ENABLE)
    SSP_ASSERT(NULL != p_cfg->p_transfer_rx->p_api);
    SSP_ASSERT(NULL != p_cfg->p_transfer_rx->p_ctrl);
    SSP_ASSERT(NULL != p_cfg->p_transfer_rx->p_cfg);
    SSP_ASSERT(NULL != p_cfg->p_transfer_rx->p_cfg->p_info);

    IRQn_Type       irq = p_ctrl->rxi_irq;
    SSP_ASSERT(SSP_INVALID_VECTOR != irq);
#endif
    transfer_info_t * p_info = p_cfg->p_transfer_rx->p_cfg->p_info;

    err = g_fmi_on_fmi.eventInfoGet(&p_feature, SSP_SIGNAL_IIC_RXI, &event_info);
    RIIC_ERROR_RETURN((SSP_SUCCESS == err), err);

    p_buffer = (uint8_t *) HW_RIIC_ReadAddrGet(p_ctrl->p_reg);
    p_info->mode           = TRANSFER_MODE_NORMAL;
    p_info->dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED;
    p_info->src_addr_mode  = TRANSFER_ADDR_MODE_FIXED;
    p_info->p_src          = (void *) p_buffer;
    p_info->irq            = TRANSFER_IRQ_END;
    p_info->size           = TRANSFER_SIZE_1_BYTE;

    transfer_cfg_t cfg     = *(p_cfg->p_transfer_rx->p_cfg);
    cfg.activation_source  = event_info.event;
    cfg.auto_enable        = false;
    cfg.p_callback         = NULL;

    err                = p_cfg->p_transfer_rx->p_api->open(p_cfg->p_transfer_rx->p_ctrl, &cfg);
    RIIC_ERROR_RETURN((SSP_SUCCESS == err), err);
    return SSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Configures  RIIC TX related transfer.
 * @param[in]     p_ctrl                     Pointer to IIC specific control structure
 * @param[in]     p_cfg                      Pointer to IIC specific configuration structure
 *
 * @retval        SSP_SUCCESS                tx transfer interface in configured with valid parameters.
 * @retval        SSP_ERR_ASSERTION          Pointer to transfer instance for I2C transmit in p_cfg is NULL.
 * @retval        SSP_ERR_IRQ_BSP_DISABLED   Interrupt does not exist in the vector table.
 * @return                          See @ref Common_Error_Codes for other possible return codes. This function calls
 *                                * fmi_api_t::productFeatureGet
 **********************************************************************************************************************/
static ssp_err_t riic_transfer_configure_tx   (riic_instance_ctrl_t * p_ctrl, i2c_cfg_t const * const p_cfg)
{
    ssp_err_t err;
    uint8_t * p_buffer = NULL;

    fmi_event_info_t event_info = {(IRQn_Type) 0U};
    ssp_feature_t p_feature = {{(ssp_ip_t) 0U}};
    p_feature.channel = p_cfg->channel;
    p_feature.unit = 0U;
    p_feature.id = SSP_IP_IIC;
    fmi_feature_info_t info = {0U};
    err = g_fmi_on_fmi.productFeatureGet(&p_feature, &info);
    RIIC_ERROR_RETURN(SSP_SUCCESS == err, err);

    /** Set default transfer info and open transmit transfer module, if enabled. */
#if (RIIC_CFG_PARAM_CHECKING_ENABLE)
    SSP_ASSERT(NULL != p_cfg->p_transfer_tx->p_api);
    SSP_ASSERT(NULL != p_cfg->p_transfer_tx->p_ctrl);
    SSP_ASSERT(NULL != p_cfg->p_transfer_tx->p_cfg);
    SSP_ASSERT(NULL != p_cfg->p_transfer_tx->p_cfg->p_info);

    IRQn_Type       irq = p_ctrl->txi_irq;
    SSP_ASSERT(SSP_INVALID_VECTOR != irq);
#endif
    transfer_info_t * p_info = p_cfg->p_transfer_tx->p_cfg->p_info;
    err = g_fmi_on_fmi.eventInfoGet(&p_feature, SSP_SIGNAL_IIC_TXI, &event_info);
    RIIC_ERROR_RETURN((SSP_SUCCESS == err), err);

    p_buffer = (uint8_t *) HW_RIIC_WriteAddrGet(p_ctrl->p_reg);
    p_info->mode           = TRANSFER_MODE_NORMAL;
    p_info->dest_addr_mode = TRANSFER_ADDR_MODE_FIXED;
    p_info->p_dest         = (void *) p_buffer;
    p_info->src_addr_mode  = TRANSFER_ADDR_MODE_INCREMENTED;
    p_info->irq            = TRANSFER_IRQ_END;
    p_info->size           = TRANSFER_SIZE_1_BYTE;

    transfer_cfg_t cfg     = *(p_cfg->p_transfer_tx->p_cfg);
    cfg.activation_source  = event_info.event;
    cfg.auto_enable        = false;
    cfg.p_callback         = NULL;
    err                    = p_cfg->p_transfer_tx->p_api->open(p_cfg->p_transfer_tx->p_ctrl, &cfg);
    RIIC_ERROR_RETURN((SSP_SUCCESS == err), err);
    return err;
}

/*******************************************************************************************************************//**
 * @brief  Parameter check.
 * @param[in]     p_ctrl                 Pointer to IIC specific control structure
 * @param[in]     p_cfg                  Pointer to IIC specific configuration structure
 *
 * @retval        SSP_SUCCESS            Provided parameters not NULL.
 * @retval        SSP_ERR_ASSERTION      The parameter p_api_ctrl or p_cfg is NULL or clock rate is greater than 1MHz.
 *                                       or p_cfg->p_extend not equal to NULL.
 **********************************************************************************************************************/
static ssp_err_t riic_param_check(riic_instance_ctrl_t * const p_ctrl, i2c_cfg_t const * const p_cfg)
{
    SSP_ASSERT(p_ctrl != NULL);
    SSP_ASSERT(p_cfg != NULL);
    SSP_ASSERT(p_cfg->rate <= I2C_RATE_FASTPLUS);

    /** p_extend not supported currently*/
    SSP_ASSERT(p_cfg->p_extend == NULL);
    return SSP_SUCCESS;
}

/* End of file */
