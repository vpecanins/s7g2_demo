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
* File Name    : sf_el_gx.h
* Description  : SSP GUIX adaptation framework header file
***********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @ingroup SF_Library
 * @defgroup SF_EL_GX GUIX Framework
 *
 * @brief GUIX adaptation layer.
 *
 * @{
 **********************************************************************************************************************/

#ifndef SF_EL_GX_H
#define SF_EL_GX_H

/** GUIX adaptation layer for SSP. */

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "sf_el_gx_api.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** Instance control block for the SSP GUIX adaptation framework */
typedef struct st_sf_el_gx_instance_ctrl
{
    GX_DISPLAY            * p_display;             ///< Pointer to the GUIX display context
    display_instance_t    * p_display_instance;    ///< Pointer to a display instance
    display_runtime_cfg_t * p_display_runtime_cfg; ///< Pointer to a runtime display configuration
    void                  * p_canvas;              ///< Pointer to a canvas(reserved)
    void                  * p_framebuffer_read;    ///< Pointer to a frame buffer (for displaying)
    void                  * p_framebuffer_write;   ///< Pointer to a frame buffer (for rendering)
    void (* p_callback)(sf_el_gx_callback_args_t * p_args); ///< Pointer to callback function
    void                  * p_context;             ///< Pointer to a context
    TX_SEMAPHORE            semaphore;             ///< Semaphore for the frame buffer flip sync
    bool                    rendering_enable;      ///< Sync flag between Rendering and displaying
    bool                    display_list_flushed;  ///< Flag to show the display list is flushed
    sf_el_gx_state_t        state;                 ///< State of this module
    void                  * p_jpegbuffer;          ///< Pointer to a JPEG work buffer
    uint32_t                jpegbuffer_size;       ///< Size of a JPEG work buffer
    uint16_t                rotation_angle;        ///< Screen rotation angle(0/90/270)
} sf_el_gx_instance_ctrl_t;

/**********************************************************************************************************************
Exported global variables
***********************************************************************************************************************/
/** @cond INC_HEADER_DEFS_SEC */
/** Filled in Interface API structure for this Instance. */
extern const sf_el_gx_api_t sf_el_gx_on_guix;
/** @endcond */

/*******************************************************************************************************************//**
 * @} (end defgroup SF_EL_GX_DISPLAY)
 **********************************************************************************************************************/

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif //SF_EL_GX_H
