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
* File Name    : sf_el_gx.c
* Description  : The definition of SSP GUIX adaptation framework functions
***********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright [2017] Renesas Electronics Corporation and/or its licensors. All Rights Reserved.
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
 * Includes
 **********************************************************************************************************************/
#include "sf_el_gx.h"
#include "sf_el_gx_cfg.h"

#include "sf_el_gx_private.h"
#include "sf_el_gx_private_api.h"

#if GX_USE_SYNERGY_DRW
#include "dave_driver.h"
#endif


/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** SSP Error macro */
#ifndef SF_EL_GX_ERROR_RETURN
/*LDRA_INSPECTED 77 S This macro does not work when surrounded by parentheses. */
#define SF_EL_GX_ERROR_RETURN(a, err) SSP_ERROR_RETURN((a), (ssp_err_t)(err), &g_module_name[0], &module_version)
#endif

/** Error macro for user callback to inform error in SSP modules */
#define SF_EL_GX_SSP_USER_ERROR_CALLBACK(pctrl, dev, ssp_err) \
        if((pctrl)->p_callback){                              \
            if(SSP_SUCCESS != (ssp_err)){                     \
                cb_args.device = (dev);                       \
                cb_args.event  = SF_EL_GX_EVENT_ERROR;        \
                cb_args.error  = (ssp_err);                   \
                (pctrl)->p_callback(&cb_args);                \
            }                                                 \
        }

/** Error macro for user callback to inform error in D/AVE 2D */
#define SF_EL_GX_D2_USER_ERROR_CALLBACK(pctrl, dev_err)       \
        if((pctrl)->p_callback){                              \
            if(D2_OK != (dev_err)){                           \
                cb_args.device = SF_EL_GX_DEVICE_DRW;         \
                cb_args.event  = SF_EL_GX_EVENT_ERROR;        \
                cb_args.error  = (uint32_t)(dev_err);         \
                (pctrl)->p_callback(&cb_args);                \
            }                                                 \
        }

/** Mutex timeout count */
#define MUTEX_WAIT_TIMER (300)

/** Display device timeout count */
#define SF_EL_GX_DISPLAY_HW_WAIT_COUNT_MAX (5000)

#if GX_USE_SYNERGY_DRW
/** Color format conversion from RGB565 to RGB888 */
#define FROM565TO888(_a) ((((_a) & 0xf800U) << 8) | (((_a) & 0x7e0U) << 5) | (((_a) & 0x1fU) << 3))
#endif

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static ssp_err_t sf_el_gx_driver_setup (GX_DISPLAY * p_display, sf_el_gx_instance_ctrl_t * const p_ctrl);

static ssp_err_t sf_el_gx_display_open (GX_DISPLAY * p_display, sf_el_gx_instance_ctrl_t * const p_ctrl);

static ssp_err_t sf_el_gx_display_close (sf_el_gx_instance_ctrl_t * const p_ctrl);

static void      sf_el_gx_canvas_clear (GX_DISPLAY * p_display, sf_el_gx_instance_ctrl_t * const p_ctrl);

static void      sf_el_gx_callback (display_callback_args_t * p_args);

#if GX_USE_SYNERGY_DRW
static ssp_err_t sf_el_gx_d2_open (GX_DISPLAY * p_display, sf_el_gx_instance_ctrl_t * const p_ctrl);

static ssp_err_t sf_el_gx_d2_close (sf_el_gx_instance_ctrl_t * const p_ctrl);
#endif

/* GUIX common callback functions setup function */
extern UINT _gx_synergy_display_driver_setup(GX_DISPLAY *display);

/* GUIX Synergy Port callback functions */
void   sf_el_frame_pointers_get (ULONG _display_handle, GX_UBYTE ** pp_visible, GX_UBYTE ** pp_working);

void   sf_el_frame_toggle (ULONG _display_handle, GX_BYTE ** pp_visible_frame);

void * sf_el_jpeg_buffer_get (ULONG _display_handle, int *p_memory_size);

int    sf_el_display_rotation_get(ULONG handle);

void   sf_el_display_actual_size_get(ULONG _display_handle, int * p_width, int * p_height);

void   sf_el_display__gx_display_8bit_palette_assign(ULONG _display_handle);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/
/** Name of module used by error logger macro */
#if BSP_CFG_ERROR_LOG != 0
static const char g_module_name[] = "sf_el_gx";
#endif
static sf_el_gx_instance_ctrl_t * gp_temp_context = NULL;    ///< Temporary pointer to the GUIX driver control block.

static TX_MUTEX g_sf_el_gx_mutex;                   ///< Driver global mutex to protect the context.

/***********************************************************************************************************************
Functions
***********************************************************************************************************************/

#if defined(__GNUC__)
/* This structure is affected by warnings from the GCC compiler bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60784
 * This pragma suppresses the warnings in this structure only, and will be removed when the SSP compiler is updated to
 * v5.3.*/
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
/** SF_EL_GX Framework version data structure */
static const ssp_version_t module_version =
{
    .api_version_minor  = SF_EL_GX_API_VERSION_MINOR,
    .api_version_major  = SF_EL_GX_API_VERSION_MAJOR,
    .code_version_major = SF_EL_GX_CODE_VERSION_MAJOR,
    .code_version_minor = SF_EL_GX_CODE_VERSION_MINOR
};
#if defined(__GNUC__)
/* Restore warning settings for 'missing-field-initializers' to as specified on command line. */
/*LDRA_INSPECTED 69 S */
#pragma GCC diagnostic pop
#endif


/** SF_EL_GX Framework API function pointer list */
const sf_el_gx_api_t sf_el_gx_on_guix =
{
    .open        = SF_EL_GX_Open,
    .close       = SF_EL_GX_Close,
    .versionGet  = SF_EL_GX_VersionGet,
    .setup       = SF_EL_GX_Setup,
    .canvasInit  = SF_EL_GX_CanvasInit
};


/*******************************************************************************************************************//**
 * @addtogroup SF_EL_GX
 * @{
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief  GUIX adaptation framework for Synergy, Driver configuration.
 * This function calls:
 * - tx_mutex_create           Creates the mutex for lock the driver during the context update.
 * - tx_mutex_delete           Deletes the mutex if kernel service calls failed in the process.
 * - tx_semaphore_create       Creates the semaphore for rendering and displaying synchronization.
 * @retval  SSP_SUCCESS          Opened the module successfully.
 * @retval  SSP_ERR_ASSERTION    NULL pointer error happens.
 * @retval  SSP_ERR_IN_USE       SF_EL_GX is in-use.
 * @retval  SSP_ERR_INTERNAL     Error happen in Kernel service calls.
 **********************************************************************************************************************/
ssp_err_t SF_EL_GX_Open(sf_el_gx_ctrl_t * const p_api_ctrl, sf_el_gx_cfg_t const * const p_cfg)
{
    sf_el_gx_instance_ctrl_t * p_ctrl = (sf_el_gx_instance_ctrl_t *) p_api_ctrl;
    UINT status;

#if (SF_EL_GX_CFG_PARAM_CHECKING_ENABLE)
    SSP_ASSERT(p_ctrl);
    SSP_ASSERT(p_cfg);
    SSP_ASSERT(p_cfg->p_display_instance);
    SSP_ASSERT(p_cfg->p_display_runtime_cfg);
    SSP_ASSERT(p_cfg->p_framebuffer_a);
    SSP_ASSERT((0   == p_cfg->rotation_angle)|(90  == p_cfg->rotation_angle)
    		  |(180 == p_cfg->rotation_angle)|(270 == p_cfg->rotation_angle));
    if (p_cfg->p_canvas == p_cfg->p_framebuffer_a)  /* A canvas cannot be same with frame buffer */
    {
        SF_EL_GX_ERROR_RETURN(false, SSP_ERR_INVALID_ARGUMENT);
    }
    if ((NULL != p_cfg->p_framebuffer_b) && (p_cfg->p_canvas == p_cfg->p_framebuffer_b))
    {
        SF_EL_GX_ERROR_RETURN(false, SSP_ERR_INVALID_ARGUMENT);
    }
    if ((NULL == p_cfg->p_canvas) && (0 != p_cfg->rotation_angle))  /* A canvas required if rotating screen */
    {
        SF_EL_GX_ERROR_RETURN(false, SSP_ERR_INVALID_ARGUMENT);
    }
#endif
    SF_EL_GX_ERROR_RETURN((SF_EL_GX_CLOSED == p_ctrl->state), SSP_ERR_IN_USE);

    if (SF_EL_GX_OPENED != p_ctrl->state)
    {
        /** Creates global mutex for SF_EL_GX */
        status = tx_mutex_create (&g_sf_el_gx_mutex, (CHAR *)"sf_gx_drv_mtx", TX_INHERIT);
        if (GX_SUCCESS != status)
        {
            SF_EL_GX_ERROR_RETURN(false, SSP_ERR_INTERNAL);
        }
    }

    /** Locks the SF_EL_GX instance until driver setup is done by SF_EL_GX_Setup(). */
    status = tx_mutex_get (&g_sf_el_gx_mutex, MUTEX_WAIT_TIMER);
    if (GX_SUCCESS != status)
    {
        tx_mutex_delete (&g_sf_el_gx_mutex);
        SF_EL_GX_ERROR_RETURN(false, SSP_ERR_INTERNAL);
    }

    /** Creates a semaphore for frame buffer flip */
    status = tx_semaphore_create(&p_ctrl->semaphore, (CHAR *)"sf_gx_drv_sem", 0);
    if(TX_SUCCESS != status)
    {
        tx_mutex_delete (&g_sf_el_gx_mutex);
        SF_EL_GX_ERROR_RETURN(false, SSP_ERR_INTERNAL);
    }

    /** Initializes the SF_EL_GX control block */
    p_ctrl->p_display_instance    = p_cfg->p_display_instance;
    p_ctrl->p_callback            = p_cfg->p_callback;
    p_ctrl->p_display_runtime_cfg = p_cfg->p_display_runtime_cfg;
    p_ctrl->p_canvas              = p_cfg->p_canvas;
    p_ctrl->p_framebuffer_read    = p_cfg->p_framebuffer_a;
    if(NULL != p_cfg->p_framebuffer_b)
    {
        p_ctrl->p_framebuffer_write = p_cfg->p_framebuffer_b;
    }
    else
    {   /* If frame buffer B is NULL, specify frame buffer A instead. */
        p_ctrl->p_framebuffer_write = p_cfg->p_framebuffer_a;
    }
    p_ctrl->p_jpegbuffer          = p_cfg->p_jpegbuffer;
    p_ctrl->jpegbuffer_size       = p_cfg->jpegbuffer_size;
    p_ctrl->rendering_enable      = false;
    p_ctrl->rotation_angle        = p_cfg->rotation_angle;

    /** Saves the control block to the global pointer inside the module temporarily. Stored data will be used in
     *  sf_el_gx_driver_setup() which will be invoked by GUIX. This pointer will be valid at last but be protected
     *  until SF_EL_GX_Setup() is done.
     */
    gp_temp_context = p_ctrl;

    /** Changes the driver state */
    p_ctrl->state = SF_EL_GX_OPENED;

    return SSP_SUCCESS;
}  /* End of function SF_EL_GX_Open() */

/*******************************************************************************************************************//**
 * @brief  GUIX adaptation framework for Synergy, Close function.
 * This function calls:
 * - tx_mutex_get                  Gets the mutex to lock the driver while device access.
 * - tx_mutex_put                  Puts the mutex to unlock the driver while device access.
 * - tx_mutex_delete               Deletes the mutex if kernel service calls failed in the process.
 * - tx_semaphore_delete           Deletes the semaphore for rendering and displaying synchronization.
 * - sf_el_gx_d2_close  Finalizes 2D Drawing Engine hardware.
 * - sf_el_gx_display_close Finalizes display hardware.
 * @retval  SSP_SUCCESS               Closed the module successfully.
 * @retval  SSP_ERR_ASSERTION         NULL pointer error happens.
 * @retval  SSP_ERR_NOT_OPEN          SF_EL_GX is not opened.
 * @retval  SSP_ERR_INTERNAL          Error happen in Kernel service calls.
 * @retval  SSP_ERR_TIMEOUT           Error occured in display driver.
 * @retval  SSP_ERR_D2D_ERROR_DEINIT  Error occured in D/AVE 2D driver.
 * @note    This function is re-entrant.
 **********************************************************************************************************************/
ssp_err_t SF_EL_GX_Close(sf_el_gx_ctrl_t * const p_api_ctrl)
{
    sf_el_gx_instance_ctrl_t * p_ctrl = (sf_el_gx_instance_ctrl_t *) p_api_ctrl;

    UINT status;
    ssp_err_t error;

#if (SF_EL_GX_CFG_PARAM_CHECKING_ENABLE)
    SSP_ASSERT(p_ctrl);
#endif
    SF_EL_GX_ERROR_RETURN((SF_EL_GX_CLOSED != p_ctrl->state), SSP_ERR_NOT_OPEN);

    /** Locks the driver to update the context. */
    status = tx_mutex_get (&g_sf_el_gx_mutex, MUTEX_WAIT_TIMER);
    SF_EL_GX_ERROR_RETURN(GX_SUCCESS == status, SSP_ERR_INTERNAL);

    if (SF_EL_GX_CONFIGURED == p_ctrl->state)
    {
#if GX_USE_SYNERGY_DRW
        /** Finalizes 2D Drawing Engine hardware */
        error = sf_el_gx_d2_close(p_ctrl);
        if (SSP_SUCCESS != error)
        {
            tx_mutex_put (&g_sf_el_gx_mutex);
            SF_EL_GX_ERROR_RETURN(SSP_SUCCESS == error, error);
        }
#endif
        /** Finalizes display hardware */
        error = sf_el_gx_display_close(p_ctrl);
        if (SSP_SUCCESS != error)
        {
            tx_mutex_put (&g_sf_el_gx_mutex);
            SF_EL_GX_ERROR_RETURN(SSP_SUCCESS == error, error);
        }
    }

    /** Changes the driver state */
    p_ctrl->state = SF_EL_GX_CLOSED;

    /** Creates a semaphore for frame buffer flip */
    tx_semaphore_delete(&p_ctrl->semaphore);

    /** Unlocks the SF_EL_GX instance since driver setup is done */
    tx_mutex_put (&g_sf_el_gx_mutex);

    /** Deletes driver global mutex */
    tx_mutex_delete (&g_sf_el_gx_mutex);

    /** Clears the temporary storage for the pointer to a control block.
     *  This is done in SF_EL_GX_Setup() in the expected function call sequence,
     *  but clear it here as well for the case of a unexpected sequence happens,
     *  meant SF_EL_GX_Setup() being not called. */
    gp_temp_context = NULL;

    return SSP_SUCCESS;
}  /* End of function SF_EL_GX_Close() */

/*******************************************************************************************************************//**
 * @brief  GUIX adaptation framework for Synergy, Version get function
 * @param[in,out] p_version  The version number.
 * @retval  SSP_SUCCESS  This function returns always this value
 * @note    This function is re-entrant.
 **********************************************************************************************************************/
ssp_err_t SF_EL_GX_VersionGet (ssp_version_t * p_version)
{
    *p_version = module_version;
    return SSP_SUCCESS;
}  /* End of function SF_EL_GX_VersionGet() */


/*******************************************************************************************************************//**
 * @brief  GUIX adaptation framework for Synergy, Setup GUIX low level device drivers for Display and D/AVE 2D interface.
 * This function has to be passed to the GUIX Studio display driver setup function gx_studio_display_configure() to let
 * GUIX configure the GUIX low level device driver(s).
 * This function calls:
 * - tx_mutex_put                Puts the driver global mutex when the low level driver setup is done
 * - tx_mutex_delete             Deletes the mutex if kernel service calls failed in the process
 * - _gx_synergy_display_driver_565rgb_setup  Setups default GUIX callback functions for RGB565 in case of display
 *       format format is RGB565 format
 * - _gx_synergy_display_driver_24xrgb_setup  Setups default GUIX callback functions for RGB565 in case of display
 *       format format is RGB888, unpacked format
 * - _gx_display_driver_32argb_setup  Setups default GUIX callback functions for RGB565 in case of display
 *       format format is ARGB8888, unpacked format
 * - sf_el_gx_driver_setup  Setups low level device drivers and overrides the GUIX default callback functions
 *       with hardware accelerated functions.
 * @retval  GX_SUCCESS    Device driver setup is successfully done.
 * @retval  GX_FAILURE    Device driver setup failed.
 * @note    Make sure SF_EL_GX_Open() has been called when this function is called back by GUIX. The behavior is
 *          not defined if this function were not invoked by GUIX.
 **********************************************************************************************************************/
UINT SF_EL_GX_Setup (GX_DISPLAY * p_display)
{
    UINT          status;
    ssp_err_t     error;
    sf_el_gx_callback_args_t cb_args;

    sf_el_gx_instance_ctrl_t * ptempctrl = gp_temp_context;

    /* Returns if temporary driver context which has to be set is not initialized. */
    SF_EL_GX_ERROR_RETURN(NULL != ptempctrl, GX_FAILURE);

    switch (ptempctrl->p_display_instance->p_cfg->input[0].format)
    {
    case DISPLAY_IN_FORMAT_16BITS_RGB565:  ///< Setups generic callback functions for RGB565, 16 bits color format.
        p_display->gx_display_color_format = GX_COLOR_FORMAT_565RGB;
        break;

    case DISPLAY_IN_FORMAT_32BITS_RGB888:  ///< Setups generic callback functions for RGB888, 24 bits, unpacked format.
        p_display->gx_display_color_format = GX_COLOR_FORMAT_24XRGB;
        break;

    case DISPLAY_IN_FORMAT_32BITS_ARGB8888:  ///< Setups generic callback functions for RGB888, 24 bits, unpacked format.
        p_display->gx_display_color_format = GX_COLOR_FORMAT_32ARGB;
        break;

    case DISPLAY_IN_FORMAT_16BITS_ARGB4444:  ///< Setups generic callback functions for ARGB4444, 16 bits color format.
        p_display->gx_display_color_format = GX_COLOR_FORMAT_4444ARGB;
        break;

    case DISPLAY_IN_FORMAT_CLUT8:
        p_display->gx_display_color_format = GX_COLOR_FORMAT_8BIT_PALETTE;
        break;

    default:
        /* Informs user application the driver setup error */
        cb_args.device = SF_EL_GX_DEVICE_NONE;
        cb_args.event  = SF_EL_GX_EVENT_ERROR;
        cb_args.error  = SSP_ERR_INVALID_ARGUMENT;
        gp_temp_context->p_callback(&cb_args);
        SF_EL_GX_ERROR_RETURN(false, GX_FAILURE);
    }

    /** Copies the GX_DISPLAY context for later use. */
    p_display->gx_display_handle = 0;
    gp_temp_context->p_display = p_display;

    /** Setups GUIX low level device drivers */
    error = sf_el_gx_driver_setup (p_display, ptempctrl);
    if(SSP_SUCCESS != error)
    {
        cb_args.device = SF_EL_GX_DEVICE_NONE;
        cb_args.event  = SF_EL_GX_EVENT_ERROR;
        cb_args.error  = error;
        ptempctrl->p_callback(&cb_args);
        SF_EL_GX_ERROR_RETURN(false, GX_FAILURE);
    }

    /** Changes the driver state */
    ptempctrl->state = SF_EL_GX_CONFIGURED;

    /** Clears the temporary storage for the pointer to a control block. */
    ptempctrl = NULL;

    /** Unlocks the SF_EL_GX instance since driver setup is done */
    status = tx_mutex_put (&g_sf_el_gx_mutex);
    SF_EL_GX_ERROR_RETURN(GX_SUCCESS == status, GX_FAILURE);

    return GX_SUCCESS;
}  /* End of function SF_EL_GX_Setup() */

/*******************************************************************************************************************//**
 * @brief  GUIX adaptation framework for Synergy, Canvas initialization, setup the memory address of first canvas to be
 * rendered.
 * @retval  SSP_SUCCESS          Memory address is successfully configured to a canvas.
 * @retval  SSP_ERR_INVALID_CALL Function call was made when the driver is not in SF_EL_GX_CONFIGURED state.
 * @retval  SSP_ERR_INTERNAL     Mutex operation had an error.
 **********************************************************************************************************************/
ssp_err_t SF_EL_GX_CanvasInit (sf_el_gx_ctrl_t * const p_api_ctrl, GX_WINDOW_ROOT * p_window_root)
{
    sf_el_gx_instance_ctrl_t * p_ctrl = (sf_el_gx_instance_ctrl_t *) p_api_ctrl;

    UINT status;
#if (SF_EL_GX_CFG_PARAM_CHECKING_ENABLE)
    SSP_ASSERT(p_ctrl);
#endif

    SF_EL_GX_ERROR_RETURN(SF_EL_GX_CONFIGURED == p_ctrl->state, SSP_ERR_INVALID_CALL);

    /** Locks the driver to update the context. */
    status = tx_mutex_get (&g_sf_el_gx_mutex, MUTEX_WAIT_TIMER);
    SF_EL_GX_ERROR_RETURN(GX_SUCCESS == status, SSP_ERR_INTERNAL);

    /** Lets GUIX know the first canvas */
    if(NULL == p_ctrl->p_canvas)
    {   /* If a buffer is not given, set the one of frame buffers as a canvas */
        p_window_root->gx_window_root_canvas->gx_canvas_memory = (GX_COLOR *)p_ctrl->p_framebuffer_write;
    }
    else
    {   /* If a buffer for canvas is given, set it as a canvas */
        p_window_root->gx_window_root_canvas->gx_canvas_memory = (GX_COLOR *)p_ctrl->p_canvas;
    }

    /** Unlocks the driver. */
    status = tx_mutex_put (&g_sf_el_gx_mutex);
    SF_EL_GX_ERROR_RETURN(GX_SUCCESS == status, SSP_ERR_INTERNAL);

    return SSP_SUCCESS;
}  /* End of function SF_EL_GX_CanvasInit() */

/*******************************************************************************************************************//**
 * @} (end addtogroup SF_EL_GX)
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, get frame buffer address.
 * This function is called by GUIX
 * @param[in]     display_handle     Pointer to the SF_EL_GX control block
 * @param[in/out] pp_visible_frame   Pointer to a pointer visible to store visible frame buffer
 * @param[in/out] pp_visible_frame   Pointer to a pointer visible to store working frame buffer
 * @retval  none
 **********************************************************************************************************************/
void sf_el_frame_pointers_get(ULONG _display_handle, GX_UBYTE ** pp_visible, GX_UBYTE ** pp_working)
{
    /** Gets control block */
    sf_el_gx_instance_ctrl_t * pctrl = (sf_el_gx_instance_ctrl_t *)(_display_handle);

    *pp_visible = pctrl->p_framebuffer_read;
    *pp_working = pctrl->p_framebuffer_write;
}  /* End of function sf_el_frame_pointers_get() */

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, toggle frame buffer.
 * This function calls:
 * - Display interface [layerChange]  Flips the frame buffer
 * - tx_semaphore_get                 Wait until the semaphore for rendering and displaying synchronization is set
 * This function is called by GUIX when following functions are executed.
 * - _gx_canvas_drawing_complete
 * - _gx_system_canvas_refresh
 * @param[in]     display_handle     Pointer to the SF_EL_GX control block
 * @param[in/out] pp_visible_frame   Pointer to a pointer to visible frame buffer
 * @retval  none
 **********************************************************************************************************************/
void sf_el_frame_toggle(ULONG display_handle, GX_BYTE ** pp_visible_frame)
{

    /** Gets control block */
    sf_el_gx_instance_ctrl_t * pctrl = (sf_el_gx_instance_ctrl_t *)display_handle;

    /** Updates the frame buffer addresses */
    void * pnext_temp = pctrl->p_framebuffer_read;
    pctrl->p_display_runtime_cfg->input.p_base = pctrl->p_framebuffer_write;
    pctrl->p_framebuffer_read  = pctrl->p_framebuffer_write;
    pctrl->p_framebuffer_write = pnext_temp;

    /** Returns the address of visible frame buffer */
    *pp_visible_frame = (GX_BYTE *)pnext_temp;

    /** Requests display driver to toggle frame buffer */
    while (SSP_SUCCESS != pctrl->p_display_instance->p_api->layerChange(
                          pctrl->p_display_instance->p_ctrl,
                          pctrl->p_display_runtime_cfg,
                          DISPLAY_FRAME_LAYER_1))
    {
        tx_thread_sleep(1);
    }

    /** Sets rendering_enable flag to the display driver to synchronize the timing */
    pctrl->rendering_enable = true;

    /** Waits until the set of semaphore which is set when the display device going into blanking period */
    tx_semaphore_get(&pctrl->semaphore, TX_WAIT_FOREVER);

}  /* End of function sf_el_frame_toggle() */

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, Display interface setup function
 * This function calls:
 * - [display].open             Opens display driver
 * - [display].start            Starts display driver
 * This function is called by:
 * - sf_el_gx_driver_setup
 * @param[in]    p_display    Pointer to a GUIX display control block
 * @param[in]    p_ctrl       Pointer to a SF_EL_GX control block
 * @retval  See error codes for SSP display interface.
 * @note This function is only allowed to be called by sf_el_gx_driver_setup().
 **********************************************************************************************************************/
static ssp_err_t sf_el_gx_display_open (GX_DISPLAY * p_display, sf_el_gx_instance_ctrl_t * const p_ctrl)
{
    ssp_err_t error;
    sf_el_gx_callback_args_t cb_args;
    display_cfg_t   tmp_cfg = *(p_ctrl->p_display_instance->p_cfg);

    SSP_PARAMETER_NOT_USED(p_display);

    /** Registers the callback function for this module */
    tmp_cfg.p_callback = sf_el_gx_callback;

    /** Utilizes p_context to let callback function set the semaphore */
    tmp_cfg.p_context  = (void *)p_ctrl;

    /**  Display driver open */
    error = p_ctrl->p_display_instance->p_api->open(p_ctrl->p_display_instance->p_ctrl,
                                                            (display_cfg_t const *)(&tmp_cfg));
    if(SSP_SUCCESS != error)
    {
        SF_EL_GX_SSP_USER_ERROR_CALLBACK(p_ctrl, SF_EL_GX_DEVICE_DISPLAY, error);
        SF_EL_GX_ERROR_RETURN(SSP_SUCCESS == error, error);
    }

    /**  Display driver start */
    error = p_ctrl->p_display_instance->p_api->start(p_ctrl->p_display_instance->p_ctrl);
    if(SSP_SUCCESS != error)
    {
        SF_EL_GX_SSP_USER_ERROR_CALLBACK(p_ctrl, SF_EL_GX_DEVICE_DISPLAY, error);
        SF_EL_GX_ERROR_RETURN(SSP_SUCCESS == error, error);
    }

    return SSP_SUCCESS;
}  /* End of function sf_el_gx_display_open() */

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, Display interface setup function
 * This function calls:
 * - [display].stop             Stops display driver
 * - [display].close            Closes display driver
 * This function is called by:
 * - SF_EL_GX_Close
 * @param[in]    p_ctrl       Pointer to a SF_EL_GX control block
 * @retval  SSP_SUCCESS       Display device was closed successfully.
 * @retval  SSP_ERR_TIMEOUT   Display device did not stop or be finalized.
 * @note This function is only allowed to be called by SF_EL_GX_Close().
 **********************************************************************************************************************/
static ssp_err_t sf_el_gx_display_close (sf_el_gx_instance_ctrl_t * const p_ctrl)
{
    ssp_err_t error;
    uint32_t  retry_count = 0;

    while(1)
    {
        /**  Stops display driver */
        error = p_ctrl->p_display_instance->p_api->stop(p_ctrl->p_display_instance->p_ctrl);
        if(SSP_ERR_INVALID_UPDATE_TIMING == error)
        {    /* Wait until current display configuration updating done */
            tx_thread_sleep(1);
            retry_count++;
            if(SF_EL_GX_DISPLAY_HW_WAIT_COUNT_MAX < retry_count)
            {
                return SSP_ERR_TIMEOUT;
            }
        }
        else
        {
            break;
        }
    }
    retry_count = 0U;
    while(1)
    {
        /**  Closes display driver */
        error = p_ctrl->p_display_instance->p_api->close(p_ctrl->p_display_instance->p_ctrl);
        if(SSP_ERR_INVALID_UPDATE_TIMING == error)
        {    /* Wait until current display configuration updating done */
            tx_thread_sleep(1);
            retry_count++;
            if(SF_EL_GX_DISPLAY_HW_WAIT_COUNT_MAX < retry_count)
            {
                return SSP_ERR_TIMEOUT;
            }
        }
        else
        {
            break;
        }
    }
    return SSP_SUCCESS;
}  /* End of function sf_el_gx_display_close() */

/***********************************************************************************************************************
 * @brief  Setups GUIX display driver.
 * This function calls:
 * - Display interface [open]  Opens the display device
 * - Display interface [start] Starts displaying by the display device
 * This function is called by:
 * - SF_EL_GX_Setup
 * @param[in]    p_display     Pointer to the GUIX display control block
 * @param[in]    p_ctrl        Pointer to a SF_EL_GX control block
 * @retval  SSP_SUCCESS  The GUIX drivers are successfully configured.
 * @retval  The others   The GUIX drivers setup had a failure. For the detail of failure, see the error definition
 *                       of the display driver.
 **********************************************************************************************************************/
static ssp_err_t sf_el_gx_driver_setup (GX_DISPLAY * p_display, sf_el_gx_instance_ctrl_t * const p_ctrl)
{
    ssp_err_t error;

    /** Setups GUIX draw functions */
    _gx_synergy_display_driver_setup(p_display);

#if GX_USE_SYNERGY_DRW
    /** Setups D/AVE 2D */
    error = sf_el_gx_d2_open (p_display, p_ctrl);
    SF_EL_GX_ERROR_RETURN(error == SSP_SUCCESS, error);
#endif
    /** Clear canvas with zero */
    sf_el_gx_canvas_clear (p_display, p_ctrl);

    /** Setups Display interface */
    error = sf_el_gx_display_open(p_display, p_ctrl);
    SF_EL_GX_ERROR_RETURN(error == SSP_SUCCESS, error);

    /** Registers the SF_EL_GX context to GUIX display handle */
    p_display->gx_display_handle = (ULONG)p_ctrl;

    return SSP_SUCCESS;
}  /* End of function sf_el_gx_driver_setup() */

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, Canvas clear function, clear the frame buffers with zero.
 * This function is called by:
 * - sf_el_gx_driver_setup
 * @param[in]    p_display    Pointer to a GUIX display control block
 * @param[in]    p_ctrl       Pointer to a SF_EL_GX control block
 * @retval  none.
 * @note This function is designed to be called by sf_el_gx_driver_setup().
 **********************************************************************************************************************/
static void sf_el_gx_canvas_clear (GX_DISPLAY * p_display, sf_el_gx_instance_ctrl_t * const p_ctrl)
{
    int     divisor;
    ULONG * put;

    switch (p_display->gx_display_color_format)
    {
    case GX_COLOR_FORMAT_565RGB:  ///< RGB565, 16 bits
        /* No break intentionally */
    case GX_COLOR_FORMAT_4444ARGB:   ///< ARGB4444, 16 bits
        divisor = 2;              ///< 16-bit data needs a half times data copy
        break;

    case GX_COLOR_FORMAT_24XRGB:  ///< RGB888, 24 bits, unpacked
        /* No break intentionally */
    case GX_COLOR_FORMAT_32ARGB:  ///< ARGB8888, 32 bits
        divisor = 1;
        break;

    case GX_COLOR_FORMAT_8BIT_PALETTE:
        divisor = 4;
        break;

    default:
        divisor = 1;              ///< Should not come here but set the value to 1
        break;
    }

    /** Clears the frame buffers */
    put = (ULONG *) p_ctrl->p_framebuffer_read;
    for (int loop = 0; loop < ((p_display->gx_display_width * p_display->gx_display_height) / divisor); loop++)
    {
        *put = 0;
        ++put;
    }

    put = (ULONG *) p_ctrl->p_framebuffer_write;
    for (int loop = 0; loop < ((p_display->gx_display_width * p_display->gx_display_height) / divisor); loop++)
    {
        *put = 0;
        ++put;
    }
}  /* End of function sf_el_gx_canvas_clear() */

#if GX_USE_SYNERGY_DRW
/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy,D/AVE 2D(2D Drawing Engine) open function
 * This function calls:
 * - d2_opendevice             Creates a new device handle of the D/AVE 2D driver
 * - d2_inithw                 Initializes 2D Drawing Engine hardware
 * - d2_startframe             Mark the begin of a scene
 * - d2_framebuffer            Specifies the rendering target
 * This function is called by:
 * - sf_el_gx_driver_setup
 * @param[in]    p_display     Pointer to a GUIX display control block
 * @param[in]    p_ctrl        Pointer to a SF_EL_GX control block
 * @retval  SSP_SUCCESS             The D/AVE 2D driver is successfully opened.
 * @retval  SSP_ERR_D2D_ERROR_INIT      The D/AVE 2D returns error at the initialization.
 * @retval  SSP_ERR_D2D_ERROR_RENDERING The D/AVE 2D returns error at opening a display list buffer.
 * @retval  SSP_ERR_UNSUPPORTED         Specified color format is not supported.
 * @retval  SSP_ERR_INVALID_ARGUMENT    Specified display parameter is invalid.
 * @note This function is only allowed to be called by sf_el_gx_driver_setup().
 **********************************************************************************************************************/
static ssp_err_t sf_el_gx_d2_open (GX_DISPLAY * p_display, sf_el_gx_instance_ctrl_t * const p_ctrl)
{
    d2_s32 d2_err;
    sf_el_gx_callback_args_t cb_args;

    /** Creates a device handle */
    p_display->gx_display_accelerator = d2_opendevice(0);

    /** Initializes 2D Drawing Engine hardware */
    d2_err = d2_inithw(p_display->gx_display_accelerator, 0);
    SF_EL_GX_D2_USER_ERROR_CALLBACK(p_ctrl,  d2_err);
    SF_EL_GX_ERROR_RETURN(D2_OK == d2_err, SSP_ERR_D2D_ERROR_INIT);

    /** Opens a display list buffer for drawing commands */
    d2_err = d2_startframe(p_display->gx_display_accelerator);
    SF_EL_GX_D2_USER_ERROR_CALLBACK(p_ctrl,  d2_err);
    SF_EL_GX_ERROR_RETURN(D2_OK == d2_err, SSP_ERR_D2D_ERROR_RENDERING);

    /** Gets output color format of D/AVE 2D interface */
    d2_s32 format = d2_mode_rgb565;
    switch (p_display->gx_display_color_format)
    {
    case GX_COLOR_FORMAT_565RGB:  ///< RGB565, 16 bits
        /* Initial value applied */
        break;

    case GX_COLOR_FORMAT_4444ARGB:    ///< ARGB4444, 16 bits
        format = d2_mode_argb4444;
        break;

    case GX_COLOR_FORMAT_24XRGB:  ///< RGB888, 24 bits, unpacked
        format = d2_mode_rgb888;
        break;

    case GX_COLOR_FORMAT_32ARGB:  ///< ARGB8888, 32 bits
        format = d2_mode_argb8888;
        break;

    case GX_COLOR_FORMAT_8BIT_PALETTE:
        format = d2_mode_clut;
        break;


    default:
        /** The other formats are not supported by hardware. */
        SF_EL_GX_D2_USER_ERROR_CALLBACK(p_ctrl,  0);
        SF_EL_GX_ERROR_RETURN(false, SSP_ERR_UNSUPPORTED);
    }

    /** Defines the framebuffer memory area and layout */
    d2_err = d2_framebuffer(p_display->gx_display_accelerator,
                   p_ctrl->p_framebuffer_write,
                   (d2_u32)p_display->gx_display_width,
                   (d2_u32)p_display->gx_display_width,
                   (d2_u32)p_display->gx_display_height,
                   format);
    SF_EL_GX_D2_USER_ERROR_CALLBACK(p_ctrl,  d2_err);
    SF_EL_GX_ERROR_RETURN(D2_OK == d2_err, SSP_ERR_INVALID_ARGUMENT);

    return SSP_SUCCESS;
}  /* End of function sf_gx_display_dave2d_open() */

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, D/AVE 2D(2D Drawing Engine) close function
 * This function calls:
 * - d2_closedevice          Destroy a device handle
 * This function is called by:
 * - SF_EL_GX_Close
 * @param[in]    p_ctrl        Pointer to a SF_EL_GX control block
 * @retval  SSP_SUCCESS              The D/AVE 2D driver is successfully closed.
 * @retval  SSP_ERR_D2D_ERROR_DEINIT  D/AVE 2D has an error in the initialization.
 * @note This function is only allowed to be called by SF_EL_GX_Close().
 **********************************************************************************************************************/
static ssp_err_t sf_el_gx_d2_close (sf_el_gx_instance_ctrl_t * const p_ctrl)
{
    d2_s32 d2_err;

    /** Destroy a device handle */
    d2_err = d2_closedevice(p_ctrl->p_display->gx_display_accelerator);
    SF_EL_GX_ERROR_RETURN(D2_OK == d2_err, SSP_ERR_D2D_ERROR_DEINIT);

    return SSP_SUCCESS;
}  /* End of function sf_el_gx_d2_close() */
#endif

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, get screen rotation angle.
 * This function is called by GUIX
 * @param[in]     display_handle     Pointer to the SF_EL_GX control block
 * @retval  Screen rotation angle. Only either of {0, 90, 480, 270} is supposed to be returned.
 **********************************************************************************************************************/
int sf_el_display_rotation_get(ULONG _display_handle)
{
    sf_el_gx_instance_ctrl_t *p_ctrl = (sf_el_gx_instance_ctrl_t *)(_display_handle);
    return p_ctrl->rotation_angle;
}  /* End of function sf_el_display_rotation_get() */

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, get active video screen size.
 * This function is called by GUIX
 * @param[in]     display_handle     Pointer to the SF_EL_GX control block
 * @param[out]    p_width            Pointer to an int size memory to return screen width in pixels
 * @param[out]    p_height           Pointer to an int size memory to return screen height in pixels
 * @retval  None
 **********************************************************************************************************************/
void   sf_el_display_actual_size_get(ULONG _display_handle, int * p_width, int * p_height)
{
    sf_el_gx_instance_ctrl_t *p_ctrl = (sf_el_gx_instance_ctrl_t *)(_display_handle);
    *p_width  = p_ctrl->p_display_instance->p_cfg->output.htiming.display_cyc;
    *p_height = p_ctrl->p_display_instance->p_cfg->output.vtiming.display_cyc;
}  /* End of function sf_el_display_actual_size_get() */

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, get JPEG work buffer address.
 * This function is called by GUIX
 * @param[in]     display_handle     Pointer to the SF_EL_GX control block
 * @param[in/out] p_memory_size      Pointer to caller defined variable to store JPEG work buffer
 * @retval  JPEG work buffer address
 **********************************************************************************************************************/
void * sf_el_jpeg_buffer_get (ULONG _display_handle, int * p_memory_size)
{
#if GX_USE_SYNERGY_JPEG
    sf_el_gx_instance_ctrl_t * pctrl = (sf_el_gx_instance_ctrl_t *)(_display_handle);

    if(p_memory_size)
    {
        *p_memory_size = (int)pctrl->jpegbuffer_size;
    }
    return((void*)pctrl->p_jpegbuffer);
#else
    SSP_PARAMETER_NOT_USED(_display_handle);

    if(p_memory_size)
    {
        *p_memory_size = 0;
    }
    return(NULL);

#endif
}  /* End of function sf_el_jpeg_buffer_get() */

/***********************************************************************************************************************
 * @brief  GUIX adaptation framework for Synergy, set CLUT table in the display module.
 * This function is called by GUIX.
 * @param[in]     display_handle     Pointer to the SF_EL_GX control block
 * @retval  None
 **********************************************************************************************************************/
void   sf_el_display__gx_display_8bit_palette_assign(ULONG _display_handle)
{
    sf_el_gx_instance_ctrl_t *p_ctrl = (sf_el_gx_instance_ctrl_t *)(_display_handle);
    display_clut_cfg_t       clut_cfg;

    clut_cfg.p_base = (uint32_t *)p_ctrl->p_display->gx_display_palette;
    clut_cfg.start  = 0;
    clut_cfg.size   = (uint16_t)p_ctrl->p_display->gx_display_palette_size;

    p_ctrl->p_display_instance->p_api->clut(p_ctrl->p_display_instance->p_ctrl, &clut_cfg, DISPLAY_FRAME_LAYER_1);

}  /* End of function sf_el_display__gx_display_8bit_palette_assign() */

/***********************************************************************************************************************
 * @brief  Callback function for GUIX driver framework. This function is called back from a Display HAL driver module.
 * If DISPLAY_EVENT_LINE_DETECTION is returned from Display HAL driver module, it sets the semaphore for rendering and
 * displaying synchronization. This function invokes a user callback function if registered through SF_EL_GX_Open()
 * function.
 * @param[in]    p_args   Pointer to the Display interface callback argument.
 * @retval  none
 **********************************************************************************************************************/
static void sf_el_gx_callback (display_callback_args_t * p_args)
{
    sf_el_gx_callback_args_t cb_arg;
    sf_el_gx_instance_ctrl_t * pctrl = (sf_el_gx_instance_ctrl_t *)p_args->p_context;

    if (DISPLAY_EVENT_LINE_DETECTION == p_args->event)
    {
        if (pctrl->rendering_enable)
        {
            /** Let GUIX know the display been in blanking period */
            tx_semaphore_ceiling_put((TX_SEMAPHORE *)&pctrl->semaphore, 1);
            pctrl->rendering_enable = false;
        }
        cb_arg.event = SF_EL_GX_EVENT_DISPLAY_VSYNC;
    }
    else if (DISPLAY_EVENT_GR1_UNDERFLOW == p_args->event)
    {
        cb_arg.event = SF_EL_GX_EVENT_UNDERFLOW;
    }
    else
    {
        /* Do nothing */
    }

    /** Invoke a user callback function if registered */
    if (pctrl->p_callback)
    {
        cb_arg.device = SF_EL_GX_DEVICE_DISPLAY;
        cb_arg.error  = SSP_SUCCESS;
        pctrl->p_callback(&cb_arg);
    }
}  /* End of function sf_el_gx_callback() */
