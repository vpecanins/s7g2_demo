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
 * File Name    : sf_message.h
 * Description  : SSP messaging framework header file
 **********************************************************************************************************************/

#ifndef SF_MESSAGE_H
#define SF_MESSAGE_H

/*******************************************************************************************************************//**
 * @ingroup SF_Library
 * @defgroup SF_MESSAGING Messaging Framework
 *
 * @brief RTOS-integrated Messaging Framework implementation.
 *
 * @{
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "sf_message_api.h"
#include "sf_message_cfg.h"

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
/** Version of code that implements the API defined in this file */
#define SF_MESSAGE_CODE_VERSION_MAJOR (1U)
#define SF_MESSAGE_CODE_VERSION_MINOR (3U)

/** The size of a message queue in words */
#define SF_MESSAGE_QUEUE_MESSAGE_WORDS (1)

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
/** Messaging framework instance control block structure */
typedef struct st_sf_message_instance_ctrl
{
    TX_BLOCK_POOL                 block_pool;                  ///< Pointer to the memory block pool control
    sf_message_subscriber_list_t  ** pp_subscriber_lists;      ///< Pointer array to the subscriber lists
    uint32_t                      buffer_size;                 ///< Bytes of the message buffer
    uint32_t                      number_of_buffers;           ///< The number of allocated buffers
    uint16_t                      number_of_subscriber_groups; ///< The number of subscriber groups
    sf_message_state_t            state;                       ///< Status of the message framework
} sf_message_instance_ctrl_t;

/**********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/
/** @cond INC_HEADER_DEFS_SEC */
/** Filled in Interface API structure for this Instance. */
extern const sf_message_api_t g_sf_message_on_sf_message;
/** @endcond */

/*******************************************************************************************************************//**
 * @} (end addtogroup SF_MESSAGING)
 **********************************************************************************************************************/

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /* SF_MESSAGE_H */
