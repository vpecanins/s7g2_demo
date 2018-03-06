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
 * File Name    : sf_el_gx_api.h
 * Description  : Interface definition for SSP GUIX adaptation framework.
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @ingroup SF_Interface_Library
 * @defgroup SF_EL_GX_API GUIX Interface
 * @brief Interface definition for Adapting Express Logic GUIX for Synergy graphics drivers.
 *
 * @section SF_EL_GX_API_SUMMARY Summary
 * This is the Interface of SF_EL_GX Framework module which ties Synergy graphics device drivers to GUIX.
 * The interface provides following driver adaptation for GUIX:
 * - DISPLAY driver for displaying image on LCD(e.g. GLCDC)
 * - Dave/2d driver for image rendering by 2DG engine
 * - JPEG driver for the image rendering by JPEG engine
 * - Software image rendering with no hardware acceleration
 *
 * Implemented by:
 * @ref SF_EL_GX
 *
 * Related SSP architecture topics:
 *  - @ref ssp-interfaces
 *  - @ref ssp-predefined-layers
 *  - @ref using-ssp-modules
 *
 * GUIX Interface description: @ref FrameworkGUIX
 * @{
 **********************************************************************************************************************/

#ifndef SF_EL_GX_API_H
#define SF_EL_GX_API_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "gx_api.h"
#include "bsp_api.h"
#include "r_display_api.h"
#include "gx_display.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** The API version of GUIX integrated driver framework */
#define SF_EL_GX_API_VERSION_MAJOR       (1U)
#define SF_EL_GX_API_VERSION_MINOR       (2U)

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** State codes for the SSP GUIX adaptation framework */
typedef enum e_sf_el_gx_state
{
    SF_EL_GX_CLOSED          = 0,
    SF_EL_GX_OPENED          = 1,
    SF_EL_GX_CONFIGURED      = 2
} sf_el_gx_state_t;

/** Low level device code for the GUIX */
typedef enum e_sf_el_gx_device
{
    SF_EL_GX_DEVICE_NONE     = 0,              ///< Non hardware
    SF_EL_GX_DEVICE_DISPLAY  = 1,              ///< Display device
    SF_EL_GX_DEVICE_DRW      = 2,              ///< 2D Graphics Engine
    SF_EL_GX_DEVICE_JPEG     = 3,              ///< JPEG Decoder
} sf_el_gx_device_t;

/** Display event codes */
typedef enum e_sf_el_gx_event
{
    SF_EL_GX_EVENT_ERROR             = 1,     ///< Low level driver error occurs
    SF_EL_GX_EVENT_DISPLAY_VSYNC     = 2,     ///< Display interface VSYNC
    SF_EL_GX_EVENT_UNDERFLOW         = 3,     ///< Display interface underflow
} sf_el_gx_event_t;

/** Callback arguments for the SSP GUIX adaptation framework */
typedef struct st_sf_el_gx_callback_args
{
    sf_el_gx_device_t   device;                ///< Device code
    sf_el_gx_event_t    event;                 ///< Event code of the low level hardware
    uint32_t            error;                 ///< Error code if SF_EL_GX_EVENT_ERROR
} sf_el_gx_callback_args_t;

/** GUIX adaptation framework control block.  Allocate an instance specific control block to pass into the
 * GUIX adaptation framework API calls.
 * @par Implemented as
 * - sf_el_gx_instance_ctrl_t
 */
typedef void sf_el_gx_ctrl_t;

/** Configuration structure for the SSP GUIX adaptation framework */
typedef struct st_sf_el_gx_cfg
{
    display_instance_t    * p_display_instance;    ///< Pointer to a display instance
    display_runtime_cfg_t * p_display_runtime_cfg; ///< Pointer to a runtime display configuration
    void                  * p_canvas;              ///< Pointer to a canvas(reserved)
    void                  * p_framebuffer_a;       ///< Pointer to a frame buffer(A)
    void                  * p_framebuffer_b;       ///< Pointer to a frame buffer(B)
    void (* p_callback)(sf_el_gx_callback_args_t * p_args); ///< Pointer to callback function
    void                  * p_context;             ///< Pointer to a context
    void                  * p_jpegbuffer;          ///< Pointer to a JPEG work buffer
    uint32_t                jpegbuffer_size;       ///< Size of a JPEG work buffer
    uint16_t                rotation_angle;        ///< Screen rotation angle(0/90/270)
} sf_el_gx_cfg_t;

/** Shared Interface definition for the SSP GUIX adaptation framework */
typedef struct st_sf_el_gx_api
{
    /** Open SSP GUIX adaptation framework.
     * @par Implemented as
     * - SF_EL_GX_Open()
     * @param[in,out]  p_ctrl    Pointer to SF_EL_GX control block structure. Must be declared by user.
     *                               Value set here.
     * @param[in]      p_cfg     Pointer to SF_EL_GX configuration structure. All elements of this
     *                               structure must be set by user.
     */
    ssp_err_t (* open)(sf_el_gx_ctrl_t * const p_ctrl, sf_el_gx_cfg_t const * const p_cfg);

    /** Close SSP GUIX adaptation framework.
     * @par Implemented as
     * - SF_EL_GX_Close()
     * @param[in,out]  p_ctrl    Pointer to SF_EL_GX control block structure.
     */
    ssp_err_t (* close)(sf_el_gx_ctrl_t * const p_ctrl);

     /** Get version.
     * @par Implemented as
     * - SF_EL_GX_VersionGet()
     * @param[in]      p_version Pointer to the memory to store the version information.
     */
    ssp_err_t (* versionGet)(ssp_version_t * p_version);

    /** Express Logic GUIX Driver setup entry.
     * @par Implemented as
     * - SF_EL_GX_Setup()
     * @param[in]      p_display Pointer to GUIX display driver setup function.
     */
   UINT (* setup)(GX_DISPLAY * p_display);

   /** Canvas initialization. Set the memory address of the initial canvas.
    * @par Implemented as
    * - SF_EL_GX_CanvasInit()
    * @param[in,out]   p_ctrl         Pointer to SF_EL_GX control block structure.
    * @param[in]       p_window_root  Pointer to GUIX root window context.
    */
  ssp_err_t (* canvasInit)(sf_el_gx_ctrl_t * const p_ctrl, GX_WINDOW_ROOT * p_window_root);

} sf_el_gx_api_t;

/** This structure encompasses everything that is needed to use an instance of this interface. */
typedef struct sf_st_el_gx_driver_instance
{
    sf_el_gx_ctrl_t      * p_ctrl;    ///< Pointer to the control structure for this instance
    sf_el_gx_cfg_t const * p_cfg;     ///< Pointer to the configuration structure for this instance
    sf_el_gx_api_t const * p_api;     ///< Pointer to the API structure for this instance
} sf_el_gx_instance_t;

/** @} (end defgroup SF_EL_GX_API) */

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /* SF_EL_GX_API_H */
