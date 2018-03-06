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
 * File Name    : sf_touch_panel_api.h
 * Description  : RTOS integrated touch panel framework API.
 **********************************************************************************************************************/

#ifndef SF_TOUCH_PANEL_API_H
#define SF_TOUCH_PANEL_API_H

/*******************************************************************************************************************//**
 * @ingroup SF_Interface_Library
 * @defgroup SF_TOUCH_PANEL_API Touch Panel Framework Interface
 * @brief RTOS-integrated Touch Panel Framework Interface.
 *
 * @section SF_TOUCH_PANEL_API_SUMMARY Summary
 * This module is a ThreadX-aware Touch Panel Framework which scans for touch events and posts them to the Messaging
 * Framework for distribution to touch event subscribers. This Interface is implemented by @ref SF_TOUCH_PANEL_I2C.
 *
 * Interfaces used:
 * - @ref SF_EXTERNAL_IRQ_API
 * - @ref I2C_API
 * - @ref SF_MESSAGING_FRAMEWORK_API
 *
 * Related SSP architecture topics:
 *  - @ref ssp-interfaces
 *  - @ref ssp-predefined-layers
 *  - @ref using-ssp-modules
 *
 * Touch Panel Framework Interface description: @ref FrameworkTouchPanelInterface
 *
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "bsp_api.h"
/* Include message API and ThreadX API */
#include "sf_message.h"
#include "tx_api.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define SF_TOUCH_PANEL_API_VERSION_MAJOR (1U)
#define SF_TOUCH_PANEL_API_VERSION_MINOR (2U)

/** Touch panel message size in 4 byte words, rounded up. */
#define SF_TOUCH_PANEL_MESSAGE_WORDS ((sizeof(sf_touch_panel_payload_t) + 3) / 4)

/** Maximum number of messages in touch panel message queue. */
#define SF_TOUCH_PANEL_MAX_MESSAGES (4)

/** Macro defining the number of bytes per word. */
#define SF_TOUCH_BYTES_PER_WORD (4)

/** Touch message queue memory size. */
#define SF_TOUCH_PANEL_MESSAGE_MEM_BYTES                    \
    (SF_TOUCH_PANEL_MESSAGE_WORDS * SF_TOUCH_BYTES_PER_WORD \
     * SF_TOUCH_PANEL_MAX_MESSAGES)

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** Touch event list. */
typedef enum st_sf_touch_panel_event
{
    SF_TOUCH_PANEL_EVENT_INVALID,         ///< Invalid touch data
    SF_TOUCH_PANEL_EVENT_HOLD,            ///< Touch has not moved since last touch event.
    SF_TOUCH_PANEL_EVENT_MOVE,            ///< Touch has moved since last touch event.
    SF_TOUCH_PANEL_EVENT_DOWN,            ///< New touch event reported.
    SF_TOUCH_PANEL_EVENT_UP,              ///< Touch released.
	SF_TOUCH_PANEL_EVENT_NONE             ///< No valid touch event happened.
} sf_touch_panel_event_t;

/** Touch data payload posted to the message queue. */
typedef struct st_sf_touch_panel_payload
{
	sf_message_header_t     header;     ///< Required header for messaging framework.
    int16_t                 x;          ///< X coordinate.
    int16_t                 y;          ///< Y coordinate.
    sf_touch_panel_event_t  event_type; ///< Touch event type.
} sf_touch_panel_payload_t;

/** Touch panel framework control block.  Allocate an instance specific control block to pass into the
 * touch panel framework API calls.
 * @par Implemented as
 * - sf_touch_panel_i2c_instance_ctrl_t
 */
typedef void sf_touch_panel_ctrl_t;

/** Configuration for RTOS integrated touch panel framework. */
typedef struct st_sf_touch_panel_cfg
{
    uint16_t           hsize_pixels;    ///< Horizontal size of screen in pixels.
    uint16_t           vsize_pixels;    ///< Vertical size of screen in pixels.
    UINT               priority;        ///< Priority of the touch panel thread.
    sf_message_instance_t const * p_message; ///< Pointer to messaging framework control block
    uint8_t            event_class_instance; ///< Event class instance number for posting touch event class messages.

    /** The frequency to report repeat (SF_TOUCH_PANEL_EVENT_DOWN or SF_TOUCH_PANEL_EVENT_HOLD) touch events in
     * Hertz. @note This will be converted to RTOS ticks in the driver and rounded up to the nearest integer
     * value of RTOS ticks. */
    uint16_t           update_hz;
    uint16_t           rotation_angle;  ///< Touch coordinate rotation angle(0/90/180/270)

    /** Pointer to hardware specific extension. See sf_touch_panel_<instance>.h. */
    void const  * p_extend;
} sf_touch_panel_cfg_t;

/** Calibration data passed to SF_TOUCH_PANEL_Calibrate. */
typedef struct st_sf_touch_panel_calibrate
{
    uint16_t  x;             ///< Expected x coordinate.
    uint16_t  y;             ///< Expected y coordinate.

    /** Acceptable linear deviation from the expected coordinates in pixels. */
    uint16_t  tolerance_pixels;

    /** Pointer to hardware specific extension.  See sf_touch_panel_<instance>_cfg_t in sf_touch_panel_<instance>.h. */
    void const  * p_extend;
} sf_touch_panel_calibrate_t;

/** Touch panel API structure. Touch panel implementations  use the following API. */
typedef struct st_sf_touch_panel_api
{
    /** Create required RTOS objects, call lower level module for hardware specific initialization, and create a
     * thread to post touch data to a message queue.
     * @par Implemented as
     *  - SF_TOUCH_PANEL_I2C_Open()
     *
     * @param[in,out] p_ctrl   Pointer to a structure allocated by user. This control structure is initialized in
     *                         this function.
     * @param[in]     p_cfg    Pointer to configuration structure. All elements of the structure must be set by user.
     */
    ssp_err_t (* open)(sf_touch_panel_ctrl_t      * const p_ctrl,
                       sf_touch_panel_cfg_t const * const p_cfg);

    /** Begin calibration routine based on provided expected coordinates.  Returns SSP_SUCCESS only if the tolerance is
     * longer than the distance from the expected touch point to the actual touch point (using the formula below):
     * p_calibrate->tolerance_pixels ^ 2 > (p_calibrate->x - x_measured) ^ 2 + (p_calibrate->y - y_measured) ^ 2
     * @par Implemented as
     *  - SF_TOUCH_PANEL_I2C_Calibrate()
     *
     * @param[in]   p_ctrl       Handle set in touch_panel_api_t::open.
     * @param[in]   p_expected  Expected coordinates and tolerance for passing.
     * @param[in]   p_actual      Pointer to message payload received from SF_MESSAGE_EVENT_CLASS_TOUCH event class.
     * @param[in]   timeout      ThreadX timeout. Select TX_NO_WAIT, a value in system clock counts between 1 and
     *                           0xFFFFFFFF, or TX_WAIT_FOREVER.
     */
    ssp_err_t (* calibrate)(sf_touch_panel_ctrl_t            * const p_ctrl,
                            sf_touch_panel_calibrate_t const * const p_expected,
                            sf_touch_panel_payload_t   const * const p_actual,
                            ULONG                                     timeout);

    /** @brief  Start scanning for touch events.
     * @par Implemented as
     *  - SF_TOUCH_PANEL_I2C_Start()
     *
     * @param[in]   p_ctrl       Handle set in touch_panel_api_t::open.
     */
    ssp_err_t (* start)(sf_touch_panel_ctrl_t       * const p_ctrl);

    /** @brief  Stop scanning for touch events.
     * @par Implemented as
     *  - SF_TOUCH_PANEL_I2C_Stop()
     *
     * @param[in]   p_ctrl       Handle set in touch_panel_api_t::open.
     */
    ssp_err_t (* stop)(sf_touch_panel_ctrl_t       * const p_ctrl);

    /** @brief  Reset touch controller if reset pin is provided, and resets the I2C bus.
     * @par Implemented as
     *  - SF_TOUCH_PANEL_I2C_Reset()
     *
     * @note This does not include calibration.  Use sf_touch_panel_api_t::calibrate from the application after
     * this function if calibration is required after reset.
     *
     * @param[in]   p_ctrl       Handle set in touch_panel_api_t::open.
     */
    ssp_err_t (* reset)(sf_touch_panel_ctrl_t       * const p_ctrl);

    /** @brief Terminate touch thread and close channel at HAL layer.
     * @par Implemented as
     *  - SF_TOUCH_PANEL_I2C_Close()
     *
     * @param[in]   p_ctrl       Handle set in touch_panel_api_t::open.
     */
    ssp_err_t (* close)(sf_touch_panel_ctrl_t      * const p_ctrl);

    /** @brief Gets version and stores it in provided pointer p_version.
     * @par Implemented as
     *  - SF_TOUCH_PANEL_I2C_VersionGet()
     *
     * @param[out]  p_version  Code and API version used stored here.
     */
    ssp_err_t (* versionGet)(ssp_version_t     * const p_version);
} sf_touch_panel_api_t;

/** This structure encompasses everything that is needed to use an instance of this interface. */
typedef struct st_sf_touch_panel_instance
{
    sf_touch_panel_ctrl_t      * p_ctrl;    ///< Pointer to the control structure for this instance
    sf_touch_panel_cfg_t const * p_cfg;     ///< Pointer to the configuration structure for this instance
    sf_touch_panel_api_t const * p_api;     ///< Pointer to the API structure for this instance
} sf_touch_panel_instance_t;


/*******************************************************************************************************************//**
 * @} (end defgroup SF_TOUCH_PANEL_API)
 **********************************************************************************************************************/

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /* SF_TOUCH_PANEL_API_H */
