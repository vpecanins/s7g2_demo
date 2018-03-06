/**************************************************************************/
/*                                                                        */
/*            Copyright (c) 1996-2016 by Express Logic Inc.               */
/*                                                                        */
/*  This software is copyrighted by and is the sole property of Express   */
/*  Logic, Inc.  All rights, title, ownership, or other interests         */
/*  in the software remain the property of Express Logic, Inc.  This      */
/*  software may only be used in accordance with the corresponding        */
/*  license agreement.  Any unauthorized use, duplication, transmission,  */
/*  distribution, or disclosure of this software is expressly forbidden.  */
/*                                                                        */
/*  This Copyright notice may not be removed or modified without prior    */
/*  written consent of Express Logic, Inc.                                */
/*                                                                        */
/*  Express Logic, Inc. reserves the right to modify this software        */
/*  without notice.                                                       */
/*                                                                        */
/*  Express Logic, Inc.                     info@expresslogic.com         */
/*  11423 West Bernardo Court               http://www.expresslogic.com   */
/*  San Diego, CA  92127                                                  */
/*                                                                        */
/**************************************************************************/

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

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** GUIX Component                                                        */
/**                                                                       */
/**   Utility (Utility)                                                   */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/
/*                                                                        */
/*  COMPONENT DEFINITION                                   RELEASE        */
/*                                                                        */
/*    gx_utility.h                                        PORTABLE C      */
/*                                                           5.3          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Express Logic, Inc.                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the GUIX utility component,                       */
/*    including all data types and external references.  It is assumed    */
/*    that gx_api.h and gx_port.h have already been included.             */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  11-24-2014     William E. Lamie         Initial Version 5.2           */
/*  01-16-2015     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.2.1  */
/*  01-26-2015     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.2.2  */
/*  03-01-2015     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.2.3  */
/*  04-15-2015     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.2.4  */
/*  08-21-2015     William E. Lamie         Modified comment(s), added    */
/*                                            function prototype,         */
/*                                            resulting in version 5.2.5  */
/*  09-21-2015     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.2.6  */
/*  02-22-2016     William E. Lamie         Modified comment(s), and      */
/*                                            fixed compiler warnings,    */
/*                                            resulting in version 5.3    */
/*                                                                        */
/**************************************************************************/

#ifndef GX_UTILITY_H
#define GX_UTILITY_H

/** Common macro for SSP header files. There is also a corresponding SSP_FOOTER macro at the end of this file. */
SSP_HEADER

/* Define utility component function prototypes.  */

GX_BOOL _gx_pixelmap_transparent_detect(GX_RESOURCE_ID id);

VOID    _gx_utility_ltoa(LONG value, GX_CHAR *return_buffer, UINT return_buffer_size);
INT     _gx_utility_math_acos(INT x);
INT     _gx_utility_math_asin(INT x);
INT     _gx_utility_math_cos(INT angle);
INT     _gx_utility_math_sin(INT angle);
UINT    _gx_utility_math_sqrt(UINT n);
VOID    _gx_utility_rectangle_center(GX_RECTANGLE *rectangle, GX_RECTANGLE *within);
VOID    _gx_utility_rectangle_center_find(GX_RECTANGLE *rectangle, GX_POINT *return_center);
VOID    _gx_utility_rectangle_combine(GX_RECTANGLE *first_rectangle, GX_RECTANGLE *second_rectangle);
GX_BOOL _gx_utility_rectangle_compare(GX_RECTANGLE *first_rectangle, GX_RECTANGLE *second_rectangle);
VOID    _gx_utility_rectangle_define(GX_RECTANGLE *rectangle, GX_VALUE left, GX_VALUE top, GX_VALUE right, GX_VALUE bottom);
VOID    _gx_utility_rectangle_resize(GX_RECTANGLE *rectangle, GX_VALUE adjust);
GX_BOOL _gx_utility_rectangle_inside_detect(GX_RECTANGLE *outer, GX_RECTANGLE *inner);
GX_BOOL _gx_utility_rectangle_overlap_detect(GX_RECTANGLE *first_rectangle, GX_RECTANGLE *second_rectangle, GX_RECTANGLE *return_overlap_area);
GX_BOOL _gx_utility_rectangle_point_detect(GX_RECTANGLE *rectangle, GX_POINT point);
VOID    _gx_utility_rectangle_shift(GX_RECTANGLE *rectangle, GX_VALUE x_shift, GX_VALUE y_shift);
UINT    _gx_utility_pixelmap_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);
UINT    _gx_utility_32argb_pixelmap_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);
UINT    _gx_utility_565rgb_pixelmap_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);
UINT    _gx_utility_4444argb_pixelmap_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);
UINT    _gx_utility_8bpp_pixelmap_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);
UINT    _gx_utility_32argb_pixelmap_simple_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);
UINT    _gx_utility_565rgb_pixelmap_simple_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);
UINT    _gx_utility_4444argb_pixelmap_simple_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);
UINT    _gx_utility_8bpp_pixelmap_simple_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);
UINT    _gx_utility_pixelmap_simple_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);

UINT    _gx_utility_pixelmap_resize(GX_PIXELMAP *src, GX_PIXELMAP *destination, INT width, INT height);
UINT    _gx_utility_32argb_pixelmap_resize(GX_PIXELMAP *src, GX_PIXELMAP *destination, INT width, INT height);
UINT    _gx_utility_16bpp_pixelmap_resize(GX_PIXELMAP *src, GX_PIXELMAP *destination, INT width, INT height);
UINT    _gx_utility_8bpp_pixelmap_resize(GX_PIXELMAP *src, GX_PIXELMAP *destination, INT width, INT height);
#ifdef GX_UTF8_SUPPORT
UINT    _gx_utility_utf8_string_character_get(GX_CHAR **utf8_str, UINT byte_count, ULONG *glyph_value, UINT *glyph_len);
UINT    _gx_utility_unicode_to_utf8(ULONG unicode, GX_UBYTE *return_utf8_str, UINT *return_utf8_size);
UINT    _gx_utility_utf8_string_character_count_get(GX_CHAR *utf8_str,  UINT byte_count, UINT *character_count);
#endif /* GX_UTF8_SUPPORT */


/* Define error checking shells for API services.  These are only referenced by the
   application.  */
VOID    _gxe_utility_ltoa(LONG value, GX_CHAR *return_buffer, UINT return_buffer_size);
INT     _gxe_utility_math_asin(INT x);
INT     _gxe_utility_math_acos(INT x);
VOID    _gxe_utility_rectangle_center(GX_RECTANGLE *rectangle, GX_RECTANGLE *within);
VOID    _gxe_utility_rectangle_center_find(GX_RECTANGLE *rectangle, GX_POINT *return_center);
VOID    _gxe_utility_rectangle_combine(GX_RECTANGLE *first_rectangle, GX_RECTANGLE *second_rectangle);
GX_BOOL _gxe_utility_rectangle_compare(GX_RECTANGLE *first_rectangle, GX_RECTANGLE *second_rectangle);
VOID    _gxe_utility_rectangle_define(GX_RECTANGLE *rectangle, GX_VALUE left, GX_VALUE top, GX_VALUE right, GX_VALUE bottom);
VOID    _gxe_utility_rectangle_resize(GX_RECTANGLE *rectangle, GX_VALUE adjust);
GX_BOOL _gxe_utility_rectangle_overlap_detect(GX_RECTANGLE *first_rectangle, GX_RECTANGLE *second_rectangle, GX_RECTANGLE *return_overlap_area);
GX_BOOL _gxe_utility_rectangle_point_detect(GX_RECTANGLE *rectangle, GX_POINT point);
VOID    _gxe_utility_rectangle_shift(GX_RECTANGLE *rectangle, GX_VALUE x_shift, GX_VALUE y_shift);
UINT    _gxe_utility_pixelmap_resize(GX_PIXELMAP *src, GX_PIXELMAP *destination, INT width, INT height);
UINT    _gxe_utility_pixelmap_rotate(GX_PIXELMAP *src, INT angle, GX_PIXELMAP *destination, UINT *rot_cx, UINT *rot_cy);

/** Common macro for SSP header files. There is also a corresponding SSP_HEADER macro at the top of this file. */
SSP_FOOTER

#endif

