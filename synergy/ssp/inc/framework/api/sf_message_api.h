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
 * File Name    : sf_message_api.h
 * Description  : SSP messaging framework interface
 **********************************************************************************************************************/

#ifndef SF_MESSAGE_API_H
#define SF_MESSAGE_API_H

/*******************************************************************************************************************//**
 * @ingroup SF_Interface_Library
 * @defgroup SF_MESSAGING_FRAMEWORK_API Messaging Framework Interface
 * @brief RTOS-integrated Messaging Framework Interface.
 *
 * @section SF_MESSAGING_FRAMEWORK_API_SUMMARY Summary
 * This module is a ThreadX-aware Messaging Framework. This Interface is implemented by @ref SF_MESSAGING.
 *
 *
 * Related SSP architecture topics:
 *  - @ref ssp-interfaces
 *  - @ref ssp-predefined-layers
 *  - @ref using-ssp-modules
 *
 * Messaging Interface description: @ref FrameworkMessagingModule
 *
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "bsp_api.h"
/*LDRA_NOANALYSIS tx_api.h is not maintained by Renesas, so LDRA analysis is skipped for this file only. */
#include "tx_api.h"
/*LDRA_ANALYSIS */
#include "sf_message_port.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/* This is to support applications using SSP 1.1.x or earlier versions where class was used instead of class_code.
 * Define SF_MESSAGE_CLASS in the pre-processor setting of the project for existing projects created using SSP 1.1.x or earlier.
 * This should be removed when ISDE supports this migration. */
#ifdef SF_MESSAGE_CLASS
#define class_code class
#endif

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** Version of the API defined in this file */
#define SF_MESSAGE_API_VERSION_MAJOR (1U)
#define SF_MESSAGE_API_VERSION_MINOR (2U)

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** Messaging framework state */
typedef enum
{
    SF_MESSAGE_STATE_CLOSED = 0,        ///< Messaging framework is closed
    SF_MESSAGE_STATE_OPENED,            ///< Messaging framework is opened
} sf_message_state_t;

/** Messaging callback response */
typedef enum
{
    SF_MESSAGE_CALLBACK_EVENT_ACK = 0,  ///< ACK response
    SF_MESSAGE_CALLBACK_EVENT_NAK,      ///< NAK response
} sf_message_callback_event_t;

/** Messaging framework state */
typedef enum
{
    SF_MESSAGE_PRIORITY_NORMAL = 0,     ///< Gives a message to be sent normal priority
    SF_MESSAGE_PRIORITY_HIGH,           ///< Gives a message to be sent higher priority than previous ones.
} sf_message_priority_t;

/** Messaging option */
typedef enum
{
    SF_MESSAGE_RELEASE_OPTION_NONE           = 0, ///< No option
    SF_MESSAGE_RELEASE_OPTION_FORCED_RELEASE = 1, ///< Buffer forced release option
    SF_MESSAGE_RELEASE_OPTION_ACK            = 2, ///< ACK response (note if both ACK and NAK are set at same time, NAK
                                                  // is taken)
    SF_MESSAGE_RELEASE_OPTION_NAK            = 4  ///< NAK response
} sf_message_release_option_t;

/** Message header definition  */
typedef struct st_sf_message_header
{
    /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
    union
    {
        uint32_t  event;
        /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
        struct
        {
            uint32_t  class_code     : 8;      ///< Event class code
            uint32_t  class_instance : 8;      ///< Event class instance number
            uint32_t  code           : 16;     ///< Event code
        }  event_b;
    };
} sf_message_header_t;

/** Subscriber lists definitions  */
typedef struct st_sf_message_instance_range
{
    uint8_t   start;                       ///< Start of the event class instance range
    uint8_t   end;                         ///< End of the event class instance range
} sf_message_instance_range_t;

/** Message subscriber */
typedef struct st_sf_message_subscriber
{
    TX_QUEUE  * p_queue;                            ///< Pointer to the message queue for subscriber thread
    sf_message_instance_range_t instance_range;     ///< Range of the event class instance to receive message
} sf_message_subscriber_t;

/** Message subscriber list */
typedef struct st_sf_message_subscriber_list
{
    sf_message_event_class_t  event_class;               ///< Event class code
    uint16_t                  number_of_nodes;           ///< Number of nodes in the subscriber group
    sf_message_subscriber_t   ** pp_subscriber_group;    ///< Subscriber group for the event class
} sf_message_subscriber_list_t;

/** Message framework callback parameters */
typedef struct st_sf_message_callback_args
{
    sf_message_callback_event_t  event;             ///< Event code
    void const                 * p_context;         ///< Context provided to user during callback
} sf_message_callback_args_t;

/** Post error information structure */
typedef struct st_sf_message_post_err
{
    TX_QUEUE  * p_queue;							///< Queue
} sf_message_post_err_t;

/** Buffer control block structure */
typedef struct st_sf_message_buffer_ctrl_t
{
    void (* p_callback)(sf_message_callback_args_t *); ///< Optional user callback function
    void const * p_context;                            ///< Context provided to user during callback
    /*LDRA_INSPECTED 381 S Anonymous structures and unions are allowed in SSP code. */
    struct st_buffer_ctrl_flag                         ///< Flags
    {
        uint32_t  semaphore    : 16;                   ///< Counting semaphore to prevent a buffer from being released
        uint32_t  buffer_keep  : 1;                    ///< Buffer keep request
        uint32_t  nak_response : 1;                    ///< NAK (ORed logic for multiple subscribers)
        uint32_t  reserved     : 5;                    ///< Reserved bits
        uint32_t  in_use       : 1;                    ///< Buffer in-use
    }  flag_b;
} sf_message_buffer_ctrl_t;

/** Message framework control block.  Allocate an instance specific control block to pass into the
 * message framework API calls.
 * @par Implemented as
 * - sf_message_instance_ctrl_t
 */
typedef void sf_message_ctrl_t;

/** Messaging framework configuration structure definition */
typedef struct st_sf_message_cfg
{
    void                          * p_work_memory_start;        ///< Start address of the memory area
    uint32_t                      work_memory_size_bytes;       ///< Size of working memory area in bytes
    uint32_t                      buffer_size;                  ///< Bytes of the message block
    sf_message_subscriber_list_t  ** pp_subscriber_lists;       ///< Pointer array to the subscriber lists
    uint8_t                       * p_block_pool_name;          ///< Pointer to the block pool name
} sf_message_cfg_t;

/** Messaging framework Post API function configuration structure definition */
typedef struct st_sf_message_acquire_cfg
{
    bool  buffer_keep;                                          ///< Buffer keep option
} sf_message_acquire_cfg_t;

/** Messaging framework Acquire API function configuration structure definition */
typedef struct st_sf_message_post_cfg
{
    sf_message_priority_t  priority;                            ///< Message priority
    void (* p_callback)(sf_message_callback_args_t *);          ///< User callback function
    void const           * p_context;                           ///< Context provided to user during callback
} sf_message_post_cfg_t;

/** Messaging Framework API structure.  Implementations will use the following API. */
typedef struct st_sf_message_api
{
    /** Initialize message framework. Initiate the messaging framework control block, configure the work memory
     * corresponding to the configuration parameters.
     * @par Implemented as
     *  - SF_MESSAGE_Open()
     *
     * @param[in,out]   p_ctrl       Pointer to the messaging control block
     * @param[in]       p_cfg        Pointer to configuration structure
     */
    ssp_err_t (* open)(sf_message_ctrl_t * const p_ctrl, sf_message_cfg_t const * const p_cfg);

    /** Finalize message framework.
     * @par Implemented as
     *  - SF_MESSAGE_Close()
     * @param[in,out]   p_ctrl       Pointer to the messaging control block
     */
    ssp_err_t (* close)(sf_message_ctrl_t * const p_ctrl);

    /** Acquire buffer for message passing from the block.
     * @par Implemented as
     *  - SF_MESSAGE_BufferAcquire()
     * @param[in]       p_ctrl        Pointer to the messaging control block
     * @param[in,out]   pp_buffer     Pointer to the pointer to the allocated buffer memory
     * @param[in]       p_acquire_cfg Pointer to the buffer acquisition configuration
     * @param[in]       wait_option   Wait option (TX_NO_WAIT, TX_WAIT_FOREVER or numerical values)
     */
    ssp_err_t (* bufferAcquire)(sf_message_ctrl_t        const *  const p_ctrl,
                                sf_message_header_t            **       pp_buffer,
                                sf_message_acquire_cfg_t const *  const p_acquire_cfg,
                                uint32_t                 const          wait_option);    

    /** Release buffer obtained from SF_MESSAGE_BufferAcquire().
     * @par Implemented as
     *  - SF_MESSAGE_BufferRelease()
     * @param[in]       p_ctrl        Pointer to the messaging control block
     * @param[in]       p_buffer      Pointer to the buffer allocated by SF_MESSAGE_BufferAcquire()
     * @param[in]       option        Buffer release option (SF_MESSAGE_RELEASE_OPTION_NONE, 
     *                                SF_MESSAGE_RELEASE_OPTION_ACK, SF_MESSAGE_RELEASE_OPTION_NAK, 
     *                                SF_MESSAGE_RELEASE_OPTION_FORCED_RELEASE)
     */
    ssp_err_t (* bufferRelease) (sf_message_ctrl_t                 * const p_ctrl,
                                 sf_message_header_t               * const p_buffer,
                                 sf_message_release_option_t const         option);    

    /** Post message to the subscribers.
     * @par Implemented as
     *  - SF_MESSAGE_Post()
     * @param[in]       p_ctrl        Pointer to the messaging control block
     * @param[in]       p_buffer      Pointer to the buffer allocated by SF_MESSAGE_BufferAcquire()
     * @param[in]       p_post_cfg    Pointer to the message post configuration
     * @param[in]       wait_option   Wait option (TX_NO_WAIT, TX_WAIT_FOREVER or numerical values)
     */
    ssp_err_t (* post)(sf_message_ctrl_t           * const p_ctrl,
                       sf_message_header_t   const * const p_buffer,
                       sf_message_post_cfg_t const * const p_post_cfg,
                       sf_message_post_err_t       * const p_post_err,
                       uint32_t                      const wait_option);    

    /** Pend message.
     * @par Implemented as
     *  -  SF_MESSAGE_Pend()
     * @param[in]       p_ctrl         Pointer to the messaging control block
     * @param[in]       p_queue        Pointer to a threadX message queue object
     * @param[in,out]   pp_buffer      Pointer to the pointer to the buffer where message is stored.
     * @param[in]       wait_option    Wait option (TX_NO_WAIT, TX_WAIT_FOREVER or numerical values)
     */
    ssp_err_t (* pend)(sf_message_ctrl_t   const *  const p_ctrl,
                       TX_QUEUE            const *  const p_queue,
                       sf_message_header_t       **       pp_buffer,
                       uint32_t                     const wait_option);    

    /** Get the version of the messaging framework.
     * @par Implemented as
     *  -  SF_MESSAGE_VersionGet()
     * @param[in]       p_version    Pointer to the memory where to store the version number
     */
    ssp_err_t (* versionGet)(ssp_version_t * const p_version);    
} sf_message_api_t;

/** This structure encompasses everything that is needed to use an instance of this interface. */
typedef struct st_sf_message_instance
{
    sf_message_ctrl_t      * p_ctrl;    ///< Pointer to the control structure for this instance
    sf_message_cfg_t const * p_cfg;     ///< Pointer to the configuration structure for this instance
    sf_message_api_t const * p_api;     ///< Pointer to the API structure for this instance
} sf_message_instance_t;

/*******************************************************************************************************************//**
 * @} (end defgroup MESSAGING_FRAMEWORK_API)
 **********************************************************************************************************************/

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /* SF_MESSAGE_API_H */
