/***********************************************************************************************************************
 * Copyright [2015] Renesas Electronics Corporation and/or its licensors. All Rights Reserved.
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
 * File Name    : touch_panel_i2c_sx8654.c
 * Description  : I2C touch panel framework chip specific implementation for the SX8654 touch panel chip.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "sf_touch_panel_i2c.h"

/*******************************************************************************************************************//**
 * @ingroup SF_TOUCH_PANEL_I2C
 * @{
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @}
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** Macro for error logger. */
#ifndef SF_TOUCH_PANEL_ERROR_RETURN
/*LDRA_INSPECTED 77 S This macro does not work when surrounded by parentheses. */
#define SF_TOUCH_PANEL_ERROR_RETURN(a, err) SSP_ERROR_RETURN((a), (err), &g_module_name[0], &g_version)
#endif

#define extract_x(t) ((int16_t) (((t).x_msb << 8) | ((t).x_lsb)))
#define extract_y(t) ((int16_t) (((t).y_msb << 8) | ((t).y_lsb)))

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** Driver-specific touch point register mapping */
typedef struct st_SX8654_touch
{
    uint8_t  x_msb  : 4;
    uint8_t  x_chan : 3;
    uint8_t         : 1;
    uint8_t  x_lsb;

    uint8_t  y_msb  : 4;
    uint8_t  y_chan : 3;
    uint8_t         : 1;
    uint8_t  y_lsb  : 8;
} SX8654_touch_t;

/* Touch controller register addresses */
#define SX8654_REGTOUCH0    (0x00)
#define SX8654_REGCHANMSK   (0x04)
#define SX8654_REGPROX0     (0x0B)
#define SX8654_REGIRQMSK    (0x22)
#define SX8654_REGIRQSRC    (0x23)

/* Commands */
#define SX8654_CMD_PENTRG   (0xe0)
#define SX8654_CMD_READ_REG (0x40)

/* Bits for RegTouch0 (Address 0x00) */
#define SX8654_REG_TOUCH0_TOUCHRATE_200CPS  (0x7 << 4)
#define SX8654_REG_TOUCH0_POWDLY_8_9US      (0x4 << 0)

/* Bits for RegTouch1 (Address 0x01) */
#define SX8654_REG_TOUCH1_RESERVED          (0x1 << 5)
#define SX8654_REG_TOUCH1_RPNDT_228KOHM     (0x1 << 2)
#define SX8654_REG_TOUCH1_FILT_NFILT3       (0x3 << 0)

/* Bits for RegTouch2 (Address 0x02) */
#define SX8654_REG_TOUCH2_SETDLY_8_9US      (0x4 << 0)

/* Bits for RegChanMsk (Address 0x4) */
#define SX8654_REGCHANMSK_XCONV             (0x1 << 7)
#define SX8654_REGCHANMSK_YCONV             (0x1 << 6)

/* Bits for RegProx0 (Address 0x0B) */
#define SX8654_REGPROX0_PROXSCANPERIOD_OFF  (0x0 << 0)

/* Bits for RegIrqMsk (Address 0x22) */
#define SX8654_REGIRQMSK_PENRELEASE         (0x1 << 2)
#define SX8654_REGIRQMSK_PENTOUCH_TOUCHCONVDONE (0x1 << 3)

/* Bits for RegIrqSrc (Address 0x23) */
#define SX8654_REGIRQSRC_PENRELEASEIRQ      (0x1 << 2)

/* ADC max output value (12-bit resolution) */
#define SX8654_MAX_ADC_OUTPUT_VALUE         (4095L)

/* I2C communication retry times */
#define SX8654_I2C_RETRY_TIMES              (10)

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/

static ssp_err_t SX8654_payload_get (sf_touch_panel_ctrl_t * const    p_ctrl,
                                     sf_touch_panel_payload_t * const p_payload);

static ssp_err_t SX8654_reset (sf_touch_panel_ctrl_t * const p_ctrl);

static ssp_err_t SX8654_i2c_read (i2c_api_master_t const * const p_i2c_api, i2c_ctrl_t * const p_i2c_ctrl,
                                      uint8_t * const p_dest, uint32_t const bytes);

static ssp_err_t SX8654_i2c_write (i2c_api_master_t const * const p_i2c_api, i2c_ctrl_t * const p_i2c_ctrl,
                                      uint8_t * const p_src, uint32_t const bytes, bool const restart );

static ssp_err_t SX8654_i2c_write_followed_by_read (i2c_api_master_t const * const p_i2c_api,
                                      i2c_ctrl_t * const p_i2c_ctrl, uint8_t * const p_data, uint32_t const bytes);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/

/** Name of module used by error logger macro */
#if BSP_CFG_ERROR_LOG != 0
static const char g_module_name[] = "sf_touch_panel_i2c_SX8654";
#endif


const sf_touch_panel_i2c_chip_t g_sf_touch_panel_i2c_chip_sx8654 =
{
    .payloadGet = SX8654_payload_get,
    .reset      = SX8654_reset
};

/** Version data structure used by error logger macro. */
#if BSP_CFG_ERROR_LOG != 0
#if defined(__GNUC__)
/* This structure is affected by warnings from the GCC compiler bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60784
 * This pragma suppresses the warnings in this structure only, and will be removed when the SSP compiler is updated to
 * v5.3.*/
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
static const ssp_version_t g_version =
{
    .api_version_minor  = SF_TOUCH_PANEL_API_VERSION_MINOR,
    .api_version_major  = SF_TOUCH_PANEL_API_VERSION_MAJOR,
    .code_version_major = SF_TOUCH_PANEL_I2C_CODE_VERSION_MAJOR,
    .code_version_minor = SF_TOUCH_PANEL_I2C_CODE_VERSION_MINOR
};
#if defined(__GNUC__)
/* Restore warning settings for 'missing-field-initializers' to as specified on command line. */
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic pop
#endif
#endif


/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief   Reads data from the touch controller through the I2C-BUS interface.
 **********************************************************************************************************************/
static ssp_err_t SX8654_i2c_read (i2c_api_master_t const * const p_i2c_api, i2c_ctrl_t * const p_i2c_ctrl,
                                      uint8_t * const p_dest, uint32_t const bytes)
{
    ssp_err_t err;

    /** Performs I2C read operation. Retry some time if failed in the communication. */
    for (int i = 0; i < SX8654_I2C_RETRY_TIMES; i++)
    {
        err = p_i2c_api->read(p_i2c_ctrl, p_dest, bytes, false);

        tx_thread_sleep(2);

        if (SSP_SUCCESS == err)
        {
            break;
        }
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief   Writes data to the touch controller through the I2C-BUS interface.
 **********************************************************************************************************************/
static ssp_err_t SX8654_i2c_write (i2c_api_master_t const * const p_i2c_api, i2c_ctrl_t * const p_i2c_ctrl,
                                       uint8_t * const p_src, uint32_t const bytes, bool const restart)
{
    ssp_err_t err;

    /** Performs I2C write operation. Retry some time if failed in the communication. */
    for (int i = 0; i < SX8654_I2C_RETRY_TIMES; i++)
    {
        err = p_i2c_api->write(p_i2c_ctrl, p_src, bytes, restart);

        tx_thread_sleep(2);

        if (SSP_SUCCESS == err)
        {
            break;
        }
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief   Write data to and read data from the touch controller through the I2C-BUS interface.
 **********************************************************************************************************************/
static ssp_err_t SX8654_i2c_write_followed_by_read (i2c_api_master_t const * const p_i2c_api,
                     i2c_ctrl_t * const p_i2c_ctrl, uint8_t * const p_data, uint32_t const bytes)
{
    ssp_err_t err;

    /** Performs I2C write followed by read. Retry some time if failed in the communication. */
    for (int i = 0; i < SX8654_I2C_RETRY_TIMES; i++)
    {
        /** Performs I2C write operation with requesting restart condition. */
        err = p_i2c_api->write(p_i2c_ctrl, p_data, bytes, true);

        tx_thread_sleep(2);

        if (SSP_SUCCESS == err)
        {
            /** Performs I2C read operation. This starts from the restart condition. */
            err = p_i2c_api->read(p_i2c_ctrl, p_data, bytes, false);

            tx_thread_sleep(2);

            if (SSP_SUCCESS == err)
            {
                break;
            }
        }
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief   Reads the touch event data from the touch controller.  Implements sf_touch_panel_i2c_chip_t::payloadGet.
 **********************************************************************************************************************/
ssp_err_t SX8654_payload_get (sf_touch_panel_ctrl_t * const p_api_ctrl, sf_touch_panel_payload_t * const p_payload)
{
    sf_touch_panel_i2c_instance_ctrl_t * p_ctrl = (sf_touch_panel_i2c_instance_ctrl_t *) p_api_ctrl;

    i2c_ctrl_t                     * p_i2c_ctrl    = p_ctrl->p_lower_lvl_i2c->p_ctrl;
    sf_external_irq_ctrl_t         * p_irq_ctrl    = p_ctrl->p_lower_lvl_irq->p_ctrl;
    i2c_api_master_t         const * p_i2c_api     = p_ctrl->p_lower_lvl_i2c->p_api;
    sf_external_irq_api_t    const * p_irq_api     = p_ctrl->p_lower_lvl_irq->p_api;
    ssp_err_t err;


    /** Wait pin interrupt from touch controller for 1 count in OS timer. */
    err = p_irq_api->wait(p_irq_ctrl, TX_WAIT_FOREVER);
    if (SSP_SUCCESS != err)
    {
        SF_TOUCH_PANEL_ERROR_RETURN(false, err);
    }

    /** Gets interrupt from a touch panel, which means the touch panel is pressed. */
    SX8654_touch_t touch[1];
    uint8_t reg;

    /* Initializes all return parameters */
    p_payload->x          = 0;
    p_payload->y          = 0;
    p_payload->event_type = SF_TOUCH_PANEL_EVENT_DOWN;

    /* Read RegIrqSrc to determine if Released or Touching */
    reg = SX8654_CMD_READ_REG | SX8654_REGIRQSRC;

    /** Gets the released/touching status */
    err = SX8654_i2c_write_followed_by_read(p_i2c_api, p_i2c_ctrl, &reg, sizeof(reg));
    if (SSP_SUCCESS != err)
    {
        p_i2c_api->reset(p_i2c_ctrl);
        SF_TOUCH_PANEL_ERROR_RETURN(false, SSP_ERR_INTERNAL);
    }

    if (reg & SX8654_REGIRQSRC_PENRELEASEIRQ)
    {
        p_payload->event_type = SF_TOUCH_PANEL_EVENT_UP;
    }

    /** Gets X/Y coordinate data */
    err = SX8654_i2c_read(p_i2c_api, p_i2c_ctrl, (uint8_t *) &touch[0], sizeof(touch));
    if (SSP_SUCCESS != err)
    {
        p_i2c_api->reset(p_i2c_ctrl);
        SF_TOUCH_PANEL_ERROR_RETURN(false, SSP_ERR_INTERNAL);
    }

    /** Processes the raw data for the touch point(s) into useful data */
    p_payload->x = (int16_t) (((int32_t) p_ctrl->hsize_pixels * (int32_t) extract_x(touch[0])) / SX8654_MAX_ADC_OUTPUT_VALUE);
    p_payload->y = (int16_t) (((int32_t) p_ctrl->vsize_pixels * (int32_t) extract_y(touch[0])) / SX8654_MAX_ADC_OUTPUT_VALUE);

    if(SF_TOUCH_PANEL_EVENT_HOLD == p_ctrl->last_payload.event_type ||
       SF_TOUCH_PANEL_EVENT_MOVE == p_ctrl->last_payload.event_type)
    {
        p_ctrl->last_payload.event_type = SF_TOUCH_PANEL_EVENT_DOWN;
    }

    if(SF_TOUCH_PANEL_EVENT_DOWN == p_ctrl->last_payload.event_type &&
       SF_TOUCH_PANEL_EVENT_DOWN == p_payload->event_type)
    {
        if ((p_ctrl->last_payload.x != p_payload->x) || (p_ctrl->last_payload.y != p_payload->y))
        {
            p_payload->event_type = SF_TOUCH_PANEL_EVENT_MOVE;
        }
        else
        {
            p_payload->event_type = SF_TOUCH_PANEL_EVENT_HOLD;
        }
    }

    p_ctrl->last_payload.event_type = p_payload->event_type;

    if(p_payload->event_type == SF_TOUCH_PANEL_EVENT_UP)
    {
        p_payload->x = p_ctrl->last_payload.x;  /* SX8654 returns coordinates with the max value if PEN UP event happens. */
        p_payload->y = p_ctrl->last_payload.y;  /* Use the coordinates obtained at last PEN DOWN and do not save the one got this time. */
    }
    else
    {
        p_ctrl->last_payload.x = p_payload->x;
        p_ctrl->last_payload.y = p_payload->y;
    }

    return err;
}

/*******************************************************************************************************************//**
 * @brief   Resets the touch chip.  Implements sf_touch_panel_i2c_chip_t::reset.
 *
 * @param[in]  p_ctrl   Pointer to control block from touch panel framework.
 **********************************************************************************************************************/
static ssp_err_t SX8654_reset (sf_touch_panel_ctrl_t * const p_api_ctrl)
{
    sf_touch_panel_i2c_instance_ctrl_t * p_ctrl = (sf_touch_panel_i2c_instance_ctrl_t *) p_api_ctrl;

    /* Parameter checking done in touch panel framework. */

    i2c_api_master_t const * const p_i2c_api       = p_ctrl->p_lower_lvl_i2c->p_api;
    i2c_ctrl_t * const             p_i2c_ctrl      = p_ctrl->p_lower_lvl_i2c->p_ctrl;
    uint8_t                        command[4];

    /** Resets touch chip by setting GPIO reset pin low. */
    g_ioport_on_ioport.pinWrite(p_ctrl->pin, IOPORT_LEVEL_LOW);

    /** Waits for a while (keep the reset signal low longer than 1ms) */
    tx_thread_sleep(2);

    /** Resets the I2C peripheral. */
    ssp_err_t err = p_i2c_api->reset(p_i2c_ctrl);

    /** Releases touch chip from reset */
    g_ioport_on_ioport.pinWrite(p_ctrl->pin, IOPORT_LEVEL_HIGH);

    /** Waits just for a while before accessing touch chip */
    tx_thread_sleep(2);

    /** Writes a complete configuration generated by the SX8654 evaluation software */
    command[0] = SX8654_REGTOUCH0;
    command[1] = SX8654_REG_TOUCH0_TOUCHRATE_200CPS|SX8654_REG_TOUCH0_POWDLY_8_9US;
    command[2] = SX8654_REG_TOUCH1_RESERVED|SX8654_REG_TOUCH1_RPNDT_228KOHM|SX8654_REG_TOUCH1_FILT_NFILT3;
    command[3] = SX8654_REG_TOUCH2_SETDLY_8_9US;
    err        = SX8654_i2c_write(p_i2c_api, p_i2c_ctrl, command, 4, false);
    if (SSP_SUCCESS != err)
    {
        p_i2c_api->reset(p_i2c_ctrl);
        SF_TOUCH_PANEL_ERROR_RETURN(false, SSP_ERR_INTERNAL);
    }

    /** The generated configuration enables too many conversion channels, ensure only
     * channels X and Y are enabled
     */
    command[0] = SX8654_REGCHANMSK;
    command[1] = SX8654_REGCHANMSK_XCONV | SX8654_REGCHANMSK_YCONV;
    err        = SX8654_i2c_write(p_i2c_api, p_i2c_ctrl, command, 2, false);
    if (SSP_SUCCESS != err)
    {
        p_i2c_api->reset(p_i2c_ctrl);
        SF_TOUCH_PANEL_ERROR_RETURN(false, SSP_ERR_INTERNAL);
    }

    /** Enables the PenTouch/TouchConvDone and PenRelease interrupts */
    command[0] = SX8654_REGIRQMSK;
    command[1] = SX8654_REGIRQMSK_PENTOUCH_TOUCHCONVDONE | SX8654_REGIRQMSK_PENRELEASE;
    err        = SX8654_i2c_write(p_i2c_api, p_i2c_ctrl, command, 2, false);
    if (SSP_SUCCESS != err)
    {
        p_i2c_api->reset(p_i2c_ctrl);
        SF_TOUCH_PANEL_ERROR_RETURN(false, SSP_ERR_INTERNAL);
    }

    /** Defines the proximity scan period - Turn off proximity as we don't currently use it */
    command[0] = SX8654_REGPROX0;
    command[1] = SX8654_REGPROX0_PROXSCANPERIOD_OFF;
    err        = SX8654_i2c_write(p_i2c_api, p_i2c_ctrl, command, 2, false);
    if (SSP_SUCCESS != err)
    {
        p_i2c_api->reset(p_i2c_ctrl);
        SF_TOUCH_PANEL_ERROR_RETURN(false, SSP_ERR_INTERNAL);
    }

    /** Enables pen trigger mode */
    command[0] = SX8654_CMD_PENTRG;
    err        = SX8654_i2c_write(p_i2c_api, p_i2c_ctrl, command, 1, false);
    if (SSP_SUCCESS != err)
    {
        p_i2c_api->reset(p_i2c_ctrl);
        SF_TOUCH_PANEL_ERROR_RETURN(false, SSP_ERR_INTERNAL);
    }

    /** Initializes the last touch event info. */
    p_ctrl->last_payload.event_type = SF_TOUCH_PANEL_EVENT_NONE;

    return SSP_SUCCESS;
}
