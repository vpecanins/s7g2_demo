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

#ifndef SF_MESSAGE_PRIVATE_API_H
#define SF_MESSAGE_PRIVATE_API_H

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/***********************************************************************************************************************
 * Private Instance API Functions. DO NOT USE! Use functions through Interface API structure instead.
 **********************************************************************************************************************/
ssp_err_t SF_MESSAGE_Open(sf_message_ctrl_t * const p_ctrl, sf_message_cfg_t const * const p_cfg);
ssp_err_t SF_MESSAGE_Close(sf_message_ctrl_t * const p_ctrl);
ssp_err_t SF_MESSAGE_BufferAcquire(sf_message_ctrl_t const * const p_ctrl,
                                   sf_message_header_t ** pp_buffer,
                                   sf_message_acquire_cfg_t const * const p_acquire_cfg,
                                   uint32_t const wait_option);
ssp_err_t SF_MESSAGE_BufferRelease(sf_message_ctrl_t * const p_ctrl,
                                   sf_message_header_t * const p_buffer,
                                   sf_message_release_option_t const option);
ssp_err_t SF_MESSAGE_Post(sf_message_ctrl_t * const p_ctrl,
                          sf_message_header_t const * const p_buffer,
                          sf_message_post_cfg_t const * const p_post_cfg,
                          sf_message_post_err_t * const p_post_err,
                          uint32_t const wait_option);
ssp_err_t SF_MESSAGE_Pend(sf_message_ctrl_t const * const p_ctrl,
                          TX_QUEUE const * const p_queue,
                          sf_message_header_t ** pp_buffer,
                          uint32_t const wait_option);
ssp_err_t SF_MESSAGE_VersionGet(ssp_version_t * const p_version);

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif /* SF_MESSAGE_PRIVATE_API_H */
