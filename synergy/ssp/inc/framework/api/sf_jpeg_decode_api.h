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
 * File Name    : sf_jpeg_decode_api.h
 * Description  : Framework JPEG Decode Interface
 **********************************************************************************************************************/


/*******************************************************************************************************************//**
 * @ingroup SF_Interface_Library
 * @defgroup SF_JPEG_DECODE_API JPEG Decode Framework Interface
 * @brief RTOS-integrated JPEG Decode Framework Interface.
 *
 * @section SF_JPEG_DECODE_API_SUMMARY Summary
 * This is a ThreadX aware generic JPEG decoding framework for run-time JPEG decode applications.
 * It can be implemented by either hardware or software. For Synergy parts, the interface is implemented by the on-chip
 * JPEG decoding engine.  The connection to the HAL layer is established by passing in a driver structure
 * in  SF_JPEG_Decode_Open.
 *
 * Related SSP architecture topics:
 *  - @ref ssp-interfaces
 *  - @ref ssp-predefined-layers
 *  - @ref using-ssp-modules
 *
 * Framework JPEG Decode Interface description: @ref SFJPEGDecodeInterface
 *
 * @{
 **********************************************************************************************************************/

#ifndef SF_JPEG_DECODE_API_H
#define SF_JPEG_DECODE_API_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "bsp_api.h"
/* Include driver API and ThreadX API */
#include "r_jpeg_decode_api.h"
#include "tx_api.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** Version of the API defined in this file */
#define SF_JPEG_DECODE_API_VERSION_MAJOR (1U)
#define SF_JPEG_DECODE_API_VERSION_MINOR (2U)

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** JPEG decode framework control block.  Allocate an instance specific control block to pass into the
 * JPEG decode framework API calls.
 * @par Implemented as
 * - sf_jpeg_decode_instance_ctrl_t
 */
typedef void sf_jpeg_decode_ctrl_t;

/** Configuration for RTOS integrated JPEG driver */
typedef struct st_sf_jpeg_decode_cfg
{
    /** Pointer to a driver structure that implements this interface. Pre-configured driver
     *  structures are located in r_jpeg_decode.c and extern'ed in r_jpeg_decode.h. */
    jpeg_decode_instance_t    const  * p_lower_lvl_jpeg_decode;
} sf_jpeg_decode_cfg_t;

/** JPEG Decode API structure.  Implementations will use the following API. */
typedef struct st_sf_jpeg_decode_api
{
    /** Acquire mutex, then handle driver initialization at the HAL layer.  This function releases mutex before
     * it returns to the caller.
     * @param[in,out] p_ctrl   Pointer to a structure allocated by user. Elements initialized here.
     * @param[in]     p_cfg    Pointer to configuration structure. All elements of the structure must be set by user.
     */
    ssp_err_t (* open)(sf_jpeg_decode_ctrl_t      * const p_ctrl,
                       sf_jpeg_decode_cfg_t const * const p_cfg);

    /** Feed data into JPEG codec module.
     * @param[in]     p_ctrl       Pointer to the control block initialized in  SF_JPEG_Decode_Open().
     * @param[in]     p_buffer     Buffer contains data to be processed by the JPEG codec module.  The buffer starting
     *                             address must be 8-byte aligned
     * @param[in]     buffer_size  Size of the data buffer, must be multiple of 8 bytes
     */
    ssp_err_t (* inputBufferSet)(sf_jpeg_decode_ctrl_t * const p_ctrl,
                                 void                  * const p_buffer,
                                 uint32_t        const         buffer_size);

    /** Read processed data from JPEG codec module.
     * @param[in]     p_ctrl       Pointer to the control block initialized in  SF_JPEG_Decode_Open().
     * @param[in]     p_buffer     User-supplied buffer space to hold output from JPEG codec module.  The buffer 
     *                             starting address must be 8-byte aligned
     * @param[in]     buffer_size  Size of the output data buffer
     */
    ssp_err_t (* outputBufferSet)(sf_jpeg_decode_ctrl_t * const p_ctrl,
                                  void                  *       p_buffer,
                                  uint32_t                      buffer_size);    

    /** Obtain number of lines JPEG codec decoded.
     * @param[in]     p_ctrl       Pointer to the control block initialized in  SF_JPEG_Decode_Open().
     * @param[out]    p_lines      Number of lines decoded into the output buffer.
     */
    ssp_err_t (* linesDecodedGet)(sf_jpeg_decode_ctrl_t * const p_ctrl,
                                  uint32_t              * const p_lines);    

    /** Configure the horizontal stride value.
     * @param[in]     p_ctrl              Pointer to the control block initialized in  SF_JPEG_Decode_Open().
     * @param[out]    horizontal_stride   Set the horizontal stride value, in pixels
     */
    ssp_err_t (* horizontalStrideSet)(sf_jpeg_decode_ctrl_t * const p_ctrl,
                                      uint32_t                      horizontal_stride);    

    /** Configure the horizontal and vertical subsample values.  This allows an application to reduce the size of the 
     * decoded image.
     * @param[in]     p_ctrl               Pointer to the control block initialized in  SF_JPEG_Decode_Open().
     * @param[in]     horizontal_subsample Set the horizontal subsample value
     * @param[in]     vertical_subsample   Set the vertical subsample value
     */
    ssp_err_t (* imageSubsampleSet)(sf_jpeg_decode_ctrl_t   * const p_ctrl,
                                    jpeg_decode_subsample_t         horizontal_subsample,
                                    jpeg_decode_subsample_t         vertical_subsample);

    /** Wait for current JPEG codec operation to finish
     * @param[in]     p_ctrl       Pointer to the control block initialized in  SF_JPEG_Decode_Open().
     * @param[out]    p_status     Status of current JPEG codec module
     * @param[out]    timeout      Amount of time (in ThreadX ticks) to wait
     */
    ssp_err_t (* wait)(sf_jpeg_decode_ctrl_t * const p_ctrl,
                       jpeg_decode_status_t  * const p_status,
                       uint32_t                      timeout);                                    

    /** Obtain JPEG codec status
     * @param[in]     p_ctrl       Pointer to the control block initialized in  SF_JPEG_Decode_Open().
     * @param[out]    p_status     Status of current JPEG codec module
     */
    ssp_err_t (* statusGet)(sf_jpeg_decode_ctrl_t * const p_ctrl,
                            jpeg_decode_status_t  * const p_status);    

    /** Obtain the size of the image.  This function is only useful for decoding a JPEG image.
     * @param[in]     p_ctrl             Pointer to the control block initialized in  SF_JPEG_Decode_Open().
     * @param[out]    p_horizontal_size  Width of the image, in pixels
     * @param[out]    p_vertical_size    Height of the image, in pixels
     */
    ssp_err_t (* imageSizeGet)(sf_jpeg_decode_ctrl_t * const p_ctrl,
                               uint16_t              *       p_horizontal_size,
                               uint16_t              *       p_vertical_size);    

    /** Obtain the pixel format of the image. This function is only useful for decoding a JPEG image.
     * @param[in]     p_ctrl             Pointer to the control block initialized in SF_JPEG_Decode_Open().
     * @param[out]    p_color_space      Color space of the image
     */
    ssp_err_t (* pixelFormatGet)(sf_jpeg_decode_ctrl_t     * const p_ctrl,
                                 jpeg_decode_color_space_t * const p_color_space);

    /** Closes JPEG codec device.  Un-finished codec operation is interrupted, and output data are discarded.
     * @param[in]     p_ctrl   Pointer to the control block set in  SF_JPEG_Decode_Open().
     */
    ssp_err_t (* close)(sf_jpeg_decode_ctrl_t * const p_ctrl);

    /** Gets version and stores it in provided pointer p_version.
     * @param[out]  p_version  Code and API version used.
     */
    ssp_err_t (* versionGet)(ssp_version_t * const p_version);    
} sf_jpeg_decode_api_t;

/** This structure encompasses everything that is needed to use an instance of this interface. */
typedef struct st_sf_jpeg_decode_instance
{
    sf_jpeg_decode_ctrl_t      * p_ctrl;    ///< Pointer to the control structure for this instance
    sf_jpeg_decode_cfg_t const * p_cfg;     ///< Pointer to the configuration structure for this instance
    sf_jpeg_decode_api_t const * p_api;     ///< Pointer to the API structure for this instance
} sf_jpeg_decode_instance_t;

/*******************************************************************************************************************//**
 * @} (end defgroup SF_JPEG_DECODE_API)
 **********************************************************************************************************************/

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /** SF_JPEG_DECODE_API_H */
