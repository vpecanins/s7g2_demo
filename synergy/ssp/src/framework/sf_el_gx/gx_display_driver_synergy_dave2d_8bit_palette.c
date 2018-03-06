/**************************************************************************/
/*                                                                        */
/*            Copyright (c) 1996-2015 by Express Logic Inc.               */
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
/** GUIX Display Driver component                                         */
/**************************************************************************/

#include    <stdio.h>
#include    <string.h>

#include    "gx_api.h"
#include    "gx_display.h"
#include    "gx_utility.h"

#ifndef GX_USE_SYNERGY_DRW
#define GX_USE_SYNERGY_DRW  1
#endif
#ifndef GX_USE_SYNERGY_JPEG
#define GX_USE_SYNERGY_JPEG 1
#endif


/** indicator for the number of visible frame buffer */
enum frame_buffers
{
    FRAME_BUFFER_A = 0,
    FRAME_BUFFER_B
};

static GX_UBYTE *visible_frame;
static GX_UBYTE *working_frame;

// functions provided by sf_el_gx.c :
extern void sf_el_frame_toggle (ULONG _display_handle,  GX_UBYTE **visible);
extern void sf_el_frame_pointers_get(ULONG display_handle, GX_UBYTE **visible, GX_UBYTE **working);
extern int  sf_el_display_rotation_get(ULONG display_handle);
extern void sf_el_display_actual_size_get(ULONG display_handle, int *width, int *height);
extern void sf_el_display__gx_display_8bit_palette_assign(ULONG _display_handle);

void _gx_rotate_canvas_to_working_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy, int angle);
void _gx_copy_visible_to_working_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy);
VOID _gx_synergy_buffer_toggle_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *dirty);

#if (GX_USE_SYNERGY_DRW == 1)
#include "dave_driver.h"

#ifndef GX_DISABLE_ERROR_CHECKING
#define LOG_DAVE_ERRORS
#endif

// space used to store int to fixed point polygon vertices
#define MAX_POLYGON_VERTICES GX_POLYGON_MAX_EDGE_NUM

#define DRAW_PIXEL if (glyph_data & mask) \
    {                                \
        *put = text_color;           \
    }                                \
    put++;                           \
    mask = (unsigned char)(mask << 1);

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#if defined(LOG_DAVE_ERRORS)
static d2_s32   dave_status;

/* macro to check for and log status code from Dave2D engine              */
#define CHECK_DAVE_STATUS(_a)               \
    dave_status = _a;                       \
    if (dave_status)                        \
    {                                       \
        _gx_log_dave_error(dave_status);    \
    }

#else
/* here is error logging is not enabled */
#define CHECK_DAVE_STATUS(_a) _a;
#endif

extern void _gx_log_dave_error(d2_s32 status);
int _gx_get_dave_error(int get_index);
extern void _gx_display_list_flush(GX_DISPLAY *display);
extern void _gx_display_list_open(GX_DISPLAY *display);
d2_device *_gx_dave2d_set_clip(GX_DRAW_CONTEXT *context);
VOID _gx_dave2d_drawing_initiate_8bpp(GX_DISPLAY *display, GX_CANVAS *canvas);
VOID _gx_dave2d_drawing_complete_8bpp(GX_DISPLAY *display, GX_CANVAS *canvas);
VOID _gx_dave2d_horizontal_line_8bpp(GX_DRAW_CONTEXT *context,
                               INT xstart, INT xend, INT ypos, INT width, GX_COLOR color);
VOID _gx_dave2d_vertical_line_8bpp(GX_DRAW_CONTEXT *context,
                             INT ystart, INT yend, INT xpos, INT width, GX_COLOR color);
VOID _gx_dave2d_simple_line_draw_8bpp(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID _gx_dave2d_simple_wide_line_8bpp(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID _gx_dave2d_horizontal_pattern_line_draw_8bpp(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos);
VOID _gx_dave2d_vertical_pattern_line_draw_8bpp(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos);
VOID _gx_dave2d_pixelmap_draw_8bpp(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap);
static VOID _gx_dave2d_set_texture(GX_DRAW_CONTEXT *context,d2_device *dave, int xpos, int ypos, GX_PIXELMAP *map);
VOID _gx_dave2d_polygon_draw_8bpp(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num);
VOID _gx_dave2d_polygon_fill_8bpp(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num);
VOID _gx_dave2d_pixel_write_8bpp(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR color);
VOID _gx_dave2d_block_move_8bpp(GX_DRAW_CONTEXT *context, GX_RECTANGLE *block, INT xshift, INT yshift);
static VOID _gx_SW_8bpp_glyph_1bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID _gx_dave2d_glyph_1bit_draw_8bpp(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
static VOID _gx_SW_8bpp_glyph_4bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID _gx_dave2d_glyph_4bit_draw_8bpp(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID _gx_dave2d_circle_draw_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_dave2d_circle_fill_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_dave2d_arc_draw_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_dave2d_arc_fill_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_dave2d_pie_fill_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_dave2d_ellipse_draw_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
VOID _gx_dave2d_wide_ellipse_draw_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
VOID _gx_dave2d_ellipse_fill_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
static void _gx_dave2d_copy_visible_to_working_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy);
VOID _gx_dave2d_buffer_toggle_8bpp (GX_CANVAS *canvas, GX_RECTANGLE *dirty);
VOID _gx_display_driver_8bit_palette_assign(GX_DISPLAY *display, GX_COLOR *palette, INT count);
void _gx_dave2d_rotate_canvas_to_working_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy, int rotation_angle);


/*****************************************************************************/
/* called by _gx_canvas_drawing_initiate. Close previous frame, set new      */
/* canvas drawing address.                                                   */
VOID _gx_dave2d_drawing_initiate_8bpp(GX_DISPLAY *display, GX_CANVAS *canvas)
{
    // make sure previous dlist is done executing
    CHECK_DAVE_STATUS(d2_endframe(display -> gx_display_accelerator))

    // trigger execution of previous display list, switch to new display list
    CHECK_DAVE_STATUS(d2_startframe(display -> gx_display_accelerator))
    CHECK_DAVE_STATUS(d2_framebuffer(display -> gx_display_accelerator, canvas -> gx_canvas_memory,
                   (d2_s32)(canvas -> gx_canvas_x_resolution), (d2_u32)(canvas -> gx_canvas_x_resolution),
                   (d2_u32)(canvas -> gx_canvas_y_resolution), d2_mode_alpha8))
}
/*****************************************************************************/
/* drawing complete- do nothing currently. The buffer toggle function takes  */
/* care of toggling the display lists.                                       */
VOID _gx_dave2d_drawing_complete_8bpp(GX_DISPLAY *display, GX_CANVAS *canvas)
{
    GX_PARAMETER_NOT_USED(canvas);
    d2_device *dave;
    dave = display->gx_display_accelerator;
    /* make sure that will not influence next draw*/
    CHECK_DAVE_STATUS(d2_setalpha(dave,0xff))
}


/*****************************************************************************/
/* draw a horizontal line using Dave2D                                       */

VOID _gx_dave2d_horizontal_line_8bpp(GX_DRAW_CONTEXT *context,
                               INT xstart, INT xend, INT ypos, INT width, GX_COLOR color)
{
    _gx_display_list_flush(context -> gx_draw_context_display);

    d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))
    /* Just for 8bpp condition.*/
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)color))
    CHECK_DAVE_STATUS(d2_renderbox(dave, (d2_point)(D2_FIX4(xstart)), (d2_point)(D2_FIX4(ypos)), (d2_width)(D2_FIX4(xend - xstart + 1)), (d2_width)(D2_FIX4(width))))
    _gx_display_list_open(context -> gx_draw_context_display);

}

/*****************************************************************************/
/* draw a vertical line using Dave2D                                         */
VOID _gx_dave2d_vertical_line_8bpp(GX_DRAW_CONTEXT *context,
                             INT ystart, INT yend, INT xpos, INT width, GX_COLOR color)
{

    d2_device *dave = _gx_dave2d_set_clip(context);
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))
    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
    /* Just for 8bpp condition.*/
    CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)color))
    CHECK_DAVE_STATUS(d2_renderbox(dave, (d2_point)(D2_FIX4(xpos)), (d2_point)(D2_FIX4(ystart)), (d2_width)(D2_FIX4(width)), (d2_width)(D2_FIX4(yend - ystart + 1))))

}

/*****************************************************************************/
/* Draw non-aliased simple line using Dave2D                                 */

VOID _gx_dave2d_simple_line_draw_8bpp(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend)
{
    d2_device *dave = _gx_dave2d_set_clip(context);
    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
    /*Just for 8bpp condition.*/
    CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)(context -> gx_draw_context_brush.gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_renderline(dave, (d2_point)(D2_FIX4(xstart)), (d2_point)(D2_FIX4(ystart)), (d2_point)(D2_FIX4(xend)), (d2_point)(D2_FIX4(yend)),
                      (d2_width)(D2_FIX4(context -> gx_draw_context_brush.gx_brush_width)), d2_le_exclude_none))
}

/*****************************************************************************/
/* Draw non-aliased wide line using Dave2D                                   */

VOID _gx_dave2d_simple_wide_line_8bpp(GX_DRAW_CONTEXT *context, INT xstart, INT ystart,
                                INT xend, INT yend)
{

    d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
     /*Just for 8bpp condition.*/
    CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)(context -> gx_draw_context_brush.gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_renderline(dave, (d2_point)(D2_FIX4(xstart)), (d2_point)(D2_FIX4(ystart)), (d2_point)(D2_FIX4(xend)), (d2_point)(D2_FIX4(yend)),
                      (d2_width)(D2_FIX4(context -> gx_draw_context_brush.gx_brush_width)), d2_le_exclude_none))

}


/*****************************************************************************/
/* draw horizontal pattern line using Dave2D                                 */

VOID _gx_dave2d_horizontal_pattern_line_draw_8bpp(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos)
{
    d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))
    CHECK_DAVE_STATUS(d2_setpatternsize(dave, 32))
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
    CHECK_DAVE_STATUS(d2_setpattern(dave, context->gx_draw_context_brush.gx_brush_line_pattern))
    /* Just for 8bpp condition.*/
    CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)(context -> gx_draw_context_brush.gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_renderline(dave, (d2_point)(D2_FIX4(xstart)), (d2_point)(D2_FIX4(ypos)), (d2_point)(D2_FIX4(xend)), (d2_point)(D2_FIX4(ypos)),
                      (d2_width)(D2_FIX4(context -> gx_draw_context_brush.gx_brush_width)), d2_le_exclude_none))

}

/*****************************************************************************/
/* Draw vertial pattern line using Dave2D                                    */

VOID _gx_dave2d_vertical_pattern_line_draw_8bpp(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos)
{
    d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))
    CHECK_DAVE_STATUS(d2_setpatternsize(dave, 32))
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
    CHECK_DAVE_STATUS(d2_setpattern(dave, context->gx_draw_context_brush.gx_brush_line_pattern))
    /* Just for 8bpp condition.*/
    CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)(context -> gx_draw_context_brush.gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_renderline(dave, (d2_point)(D2_FIX4(xpos)), (d2_point)(D2_FIX4(ystart)), (d2_point)(D2_FIX4(xpos)), (d2_point)(D2_FIX4(yend)),
                  (d2_width)(D2_FIX4(context -> gx_draw_context_brush.gx_brush_width)), d2_le_exclude_none))
}

/*****************************************************************************/
/* Draw pixelmap using Dave2D. Currently 8bpp, 16bpp, and 32 bpp source      */
/* formats are supported. Others may be added. Optional RLE compression      */
/* of all formats.                                                           */

VOID _gx_dave2d_pixelmap_draw_8bpp(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap)
{
    d2_u32 mode = d2_mode_alpha8;
    _gx_display_list_flush(context -> gx_draw_context_display);

    if (pixelmap -> gx_pixelmap_flags & GX_PIXELMAP_COMPRESSED)
    {
        mode |= d2_mode_rle;
    }

    d2_device *dave = _gx_dave2d_set_clip(context);

    d2_setalphablendmode(dave,d2_bm_one,d2_bm_zero);
    d2_setalpha(dave, 0xff);

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *)pixelmap -> gx_pixelmap_data,
                           pixelmap -> gx_pixelmap_width, pixelmap -> gx_pixelmap_width,
                           pixelmap -> gx_pixelmap_height, mode))

    mode = d2_bf_no_blitctxbackup;
    mode |= d2_bf_usealpha;

    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                         pixelmap -> gx_pixelmap_width,
                         pixelmap -> gx_pixelmap_height,
                         0, 0,
                         (d2_point)(D2_FIX4(pixelmap -> gx_pixelmap_width)),
                         (d2_point)(D2_FIX4(pixelmap -> gx_pixelmap_height)),
                         (d2_point)(D2_FIX4(xpos)), (d2_point)(D2_FIX4(ypos)), mode))
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
/* support function used to apply texture source for all shape drawing       */

static VOID _gx_dave2d_set_texture(GX_DRAW_CONTEXT *context,d2_device *dave, int xpos, int ypos, GX_PIXELMAP *map)
{
    GX_PARAMETER_NOT_USED(context);
    d2_u32 format = d2_mode_alpha8;

    if (map->gx_pixelmap_flags & GX_PIXELMAP_COMPRESSED)
    {
        format |= d2_mode_rle;
    }

    CHECK_DAVE_STATUS(d2_settexture(dave,
            (void *) map->gx_pixelmap_data,
            map->gx_pixelmap_width,
            map->gx_pixelmap_width,
            map->gx_pixelmap_height,
            format))

    CHECK_DAVE_STATUS(d2_settextureoperation(dave, d2_to_copy, d2_to_zero, d2_to_zero, d2_to_zero));//palette
    CHECK_DAVE_STATUS(d2_settexelcenter(dave, 0, 0))
    CHECK_DAVE_STATUS(d2_settexturemapping(dave,
            (d2_point)(xpos << 4), (d2_point)(ypos << 4),
            0, 0,
            1 << 16, 0,
            0, 1 << 16))
}
/*****************************************************************************/
/* GUIX polygon draw utilizing Dave2D                                        */

VOID _gx_dave2d_polygon_draw_8bpp(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num)
{
    int loop;
    int index;
    GX_VALUE val;
    d2_point data[MAX_POLYGON_VERTICES * 2];
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    d2_color brush_color = brush->gx_brush_line_color;
    UINT temp_style = 0;

    // if the polygon is also filled, the outline has already been drawn, just return
    if (brush->gx_brush_style & (GX_BRUSH_SOLID_FILL|GX_BRUSH_PIXELMAP_FILL))
    {
        temp_style = brush->gx_brush_style;
        brush->gx_brush_style = (UINT)(brush->gx_brush_style & (UINT)(~ (GX_BRUSH_SOLID_FILL|GX_BRUSH_PIXELMAP_FILL)));
    }

    /* convert incoming point data to d2_point type */
    _gx_display_list_flush(context -> gx_draw_context_display);
    index = 0;
    for (loop = 0; loop < num; loop++)
    {
        val = vertex[loop].gx_point_x;
        data[index++] = (d2_point)(D2_FIX4(val));
        val = vertex[loop].gx_point_y;
        data[index++] = (d2_point)(D2_FIX4(val));
    }

    d2_device *dave = _gx_dave2d_set_clip(context);

    /*make sure antiliasing is off*/
    CHECK_DAVE_STATUS(d2_setantialiasing(dave,0))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_outline))
    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_outlinewidth(dave, (d2_width)(D2_FIX4(brush->gx_brush_width))))

    CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)brush_color))

    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))

    if (brush->gx_brush_style & GX_BRUSH_ROUND)
    {
        CHECK_DAVE_STATUS(d2_setlinejoin(dave, d2_lj_round))
    }
    else
    {
        CHECK_DAVE_STATUS(d2_setlinejoin(dave, d2_lj_miter))
    }
    /*color doesn't work, so use alpha, set const alpha to target value, write it to framebuffer.*/
    /* Just for 8bpp condition.*/
    CHECK_DAVE_STATUS(d2_renderpolygon(dave, data, (d2_u32)num, 0))

    if(temp_style != 0)
    {
        brush->gx_brush_style = temp_style ;
    }
    _gx_display_list_open(context -> gx_draw_context_display);
}


/*****************************************************************************/
/* GUIX filled polygon rendering using Dave2D                                */

VOID _gx_dave2d_polygon_fill_8bpp(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num)
{
    _gx_display_list_flush(context -> gx_draw_context_display);

    int loop;
    int index;
    GX_VALUE val;
    d2_point data[MAX_POLYGON_VERTICES * 2];
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    GX_COLOR brush_color = brush->gx_brush_fill_color;

    /* convert incoming point data to d2_point type */
    index = 0;
    for (loop = 0; loop < num; loop++)
    {
        val = vertex[loop].gx_point_x;
        data[index++] = (d2_point)(D2_FIX4(val));
        val = vertex[loop].gx_point_y;
        data[index++] = (d2_point)(D2_FIX4(val));
    }

    d2_device *dave = _gx_dave2d_set_clip(context);
    _gx_display_list_flush(context -> gx_draw_context_display);

    GX_VALUE temp_width = brush->gx_brush_width;
    brush->gx_brush_width = 0;

    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))

    CHECK_DAVE_STATUS(d2_setantialiasing(dave,0))
    CHECK_DAVE_STATUS(d2_setalpha(dave, 0xff))

    if (brush->gx_brush_style & GX_BRUSH_PIXELMAP_FILL)
    {
        CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_texture))
        _gx_dave2d_set_texture(context,
                               dave,
                context->gx_draw_context_clip->gx_rectangle_left,
                context->gx_draw_context_clip->gx_rectangle_top,
                brush->gx_brush_pixelmap);
        /*consider alpha default mode is constant,value is 0xff
         * make sure for this before drawing a map */
    }
    else
    {
        CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
       /*color doesn't work, so use alpha, set const alpha to target value, write it to framebuffer.*/
        CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)brush_color))
    }
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
    CHECK_DAVE_STATUS(d2_renderpolygon(dave, data, (d2_u32)num, 0))

    brush->gx_brush_width = temp_width;
    _gx_display_list_open(context -> gx_draw_context_display);
}


/*****************************************************************************/
/* GUIX display driver pixel write. Must first flush the Dave2D display list */
/* to insure order of operation.                                             */
VOID _gx_dave2d_pixel_write_8bpp(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR color)
{

    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_8bpp_pixel_write(context, x, y, color);
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
/* Move a block of pixels within working canvas memory.                      */
/* Mainly used for fast scrolling.                                           */
VOID _gx_dave2d_block_move_8bpp(GX_DRAW_CONTEXT *context,
                          GX_RECTANGLE *block, INT xshift, INT yshift)
{

    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_8bpp_block_move(context, block, xshift, yshift);
    _gx_display_list_open(context -> gx_draw_context_display);

}

/*****************************************************************************/
static VOID _gx_SW_8bpp_glyph_1bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph)
{
GX_UBYTE *glyph_row;
GX_UBYTE *glyph_data_ptr;
UINT      row;
UINT      pixel_per_row;
UINT      pixel_in_first_byte;
UINT      pixel_in_last_byte;
GX_UBYTE  text_color;
UINT      y_height;
GX_UBYTE  glyph_data;
UINT      glyph_width;
GX_UBYTE *put;
UINT      num_bytes;
UINT      num_bits;
GX_UBYTE *line_start;
GX_UBYTE  mask, init_mask;
UINT      i;

    text_color = (GX_UBYTE)context->gx_draw_context_brush.gx_brush_line_color;
    pixel_per_row = (UINT)(draw_area->gx_rectangle_right - draw_area->gx_rectangle_left + 1);

    /* Find the width of the glyph, in terms of bytes */
    glyph_width = glyph->gx_glyph_width;

    /* Make it byte-aligned. */
    glyph_width = (glyph_width + 7) >> 3;

    /* Compute the number of useful bytes from the glyph this routine is going to use.
       Because of map_offset, the first byte may contain pixel bits we don't need to draw;
       And the width of the draw_area may produce part of the last byte in the row to be ignored. */
    num_bytes = (UINT)(((UINT)(map_offset->gx_point_x) + pixel_per_row + 7) >> 3);

    /* Take into account if map_offset specifies the number of bytes to ignore from the beginning of the row. */
    num_bytes = (UINT)(num_bytes -(UINT)((UINT)(map_offset->gx_point_x) >> 3));

    /* Compute the number of pixels to draw from the first byte of the glyph data. */
    pixel_in_first_byte = (UINT)(8 - ((map_offset->gx_point_x) & 0x7));
    init_mask = (GX_UBYTE)(0x80 >> (pixel_in_first_byte - 1));

    /* Compute the number of pixels to draw from the last byte, if there are more than one byte in a row. */
    if (num_bytes != 1)
    {
        pixel_in_last_byte = ((UINT)(map_offset->gx_point_x) + pixel_per_row) & 0x7;
        if (pixel_in_last_byte == 0)
        {
            pixel_in_last_byte = 8;
        }
    }
    else
    {
        if (((UINT)(map_offset->gx_point_x) + pixel_per_row) < 8)
        {
            pixel_in_first_byte = pixel_per_row;
        }
        else
        {
            pixel_in_last_byte = 0;
        }
    }

    glyph_row = (GX_UBYTE *)glyph->gx_glyph_map;

    if (map_offset->gx_point_y)
    {
        glyph_row = glyph_row + ((GX_VALUE)glyph_width * map_offset->gx_point_y);
    }

    glyph_row += (map_offset->gx_point_x >> 3);

    y_height = (UINT)(draw_area->gx_rectangle_bottom - draw_area->gx_rectangle_top + 1);

    line_start = (GX_UBYTE *)context->gx_draw_context_memory;
    line_start += context->gx_draw_context_pitch * (draw_area->gx_rectangle_top);
    line_start += draw_area->gx_rectangle_left;

    for (row = 0; row < y_height; row++)
    {
        glyph_data_ptr = glyph_row;
        glyph_data = *glyph_data_ptr;
        mask = init_mask;
        num_bits = pixel_in_first_byte;
        put = line_start;
        for (i = 0; i < num_bytes; i++)
        {
            if ((i == (num_bytes - 1)) && (num_bytes > 1))
            {
                num_bits = pixel_in_last_byte;
            }
            switch (num_bits)
            {
            case 8:
                DRAW_PIXEL;
            case 7:
                DRAW_PIXEL;
            case 6:
                DRAW_PIXEL;
            case 5:
                DRAW_PIXEL;
            case 4:
                DRAW_PIXEL;
            case 3:
                DRAW_PIXEL;
            case 2:
                DRAW_PIXEL;
            case 1:
                DRAW_PIXEL;
            }
            glyph_data_ptr++;
            glyph_data = *(glyph_data_ptr);
            num_bits = 8;
            mask = 0x01;
        }

        glyph_row += glyph_width;
        line_start += context->gx_draw_context_pitch;
    }
    return;
}


/*****************************************************************************/
VOID _gx_dave2d_glyph_1bit_draw_8bpp(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_SW_8bpp_glyph_1bit_draw(context, draw_area, map_offset, glyph);
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
static VOID _gx_SW_8bpp_glyph_4bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph)
{
GX_UBYTE *glyph_row;
GX_UBYTE *glyph_data_ptr;
UINT      row;
UINT      pixel_width = 0;
UINT      leading_pixel;
UINT      trailing_pixel;
GX_UBYTE  text_color;
UINT      y_height;
GX_UBYTE  glyph_data;
UINT      pitch;
UINT      index;
GX_UBYTE *put;
GX_UBYTE *draw_start;

    draw_start = (GX_UBYTE *)context->gx_draw_context_memory;
    draw_start += context->gx_draw_context_pitch * draw_area->gx_rectangle_top;
    draw_start += draw_area->gx_rectangle_left;

    text_color = (GX_UBYTE)(context->gx_draw_context_brush.gx_brush_line_color + 15);
    pixel_width = (UINT)(draw_area->gx_rectangle_right - draw_area->gx_rectangle_left + 1);

    /* Find the width of the glyph */
    pitch = glyph->gx_glyph_width;

    /* Make it byte-aligned. */
    pitch = (pitch + 1) >> 1;

    glyph_row = (GX_UBYTE *)glyph->gx_glyph_map;

    if (map_offset->gx_point_y)
    {
        glyph_row = (GX_UBYTE *)(glyph_row + ((GX_VALUE)pitch * map_offset->gx_point_y));
    }

    glyph_row += (map_offset->gx_point_x >> 1);
    y_height = (UINT)(draw_area->gx_rectangle_bottom - draw_area->gx_rectangle_top + 1);
    leading_pixel = (map_offset->gx_point_x & 1);
    pixel_width -= leading_pixel;
    trailing_pixel = pixel_width & 1;
    pixel_width = pixel_width >> 1;

    for (row = 0; row < y_height; row++)
    {
        glyph_data_ptr = glyph_row;
        put = draw_start;

        if (leading_pixel)
        {
            glyph_data = *glyph_data_ptr++;
            glyph_data >>= 4;
            *put = (GX_UBYTE)(text_color - glyph_data);
            put++;
        }
        for (index = 0; index < pixel_width; index++)
        {
            glyph_data = *glyph_data_ptr++;

            *put++ = (GX_UBYTE)(text_color - (glyph_data & 0x0F));
            *put++ = (GX_UBYTE)(text_color - (glyph_data >> 4));
        }

        if (trailing_pixel)
        {
            glyph_data = *glyph_data_ptr & 0x0F;
            *put = (GX_UBYTE)(text_color - glyph_data);
            put++;
        }
        glyph_row += pitch;
        draw_start += context->gx_draw_context_pitch;
    }
}
VOID _gx_dave2d_glyph_4bit_draw_8bpp(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_SW_8bpp_glyph_4bit_draw(context, draw_area, map_offset, glyph);
    _gx_display_list_open(context -> gx_draw_context_display);
}


#if defined(GX_ARC_DRAWING_SUPPORT)

/*****************************************************************************/
/* Render non-aliased circle outline using Dave2D                            */

VOID _gx_dave2d_circle_draw_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r)
{

    GX_BRUSH *brush = &context->gx_draw_context_brush;

    // if the polygon is also filled, the outline has already been drawn, just return
    if (brush->gx_brush_style & (GX_BRUSH_SOLID_FILL|GX_BRUSH_PIXELMAP_FILL))
    {
        return;
    }

    context->gx_draw_context_clip->gx_rectangle_top = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_top - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_left = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_left - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_right = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_right + brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_bottom = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_bottom + brush->gx_brush_width);


    d2_device *dave = _gx_dave2d_set_clip(context);
    /*make sure antiliasing is off*/
    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_outline))
    CHECK_DAVE_STATUS(d2_outlinewidth(dave, (d2_width)(D2_FIX4(brush->gx_brush_width))))
    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)(brush->gx_brush_line_color)))

    CHECK_DAVE_STATUS(d2_rendercircle(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_width)(D2_FIX4(r)), 0))

}

/*****************************************************************************/
/* Render filled circle using Dave2D                                         */

VOID _gx_dave2d_circle_fill_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r)
{
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    GX_COLOR brush_color = brush->gx_brush_fill_color;

    d2_device *dave = _gx_dave2d_set_clip(context);

    context->gx_draw_context_clip->gx_rectangle_top = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_top - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_left = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_left - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_right = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_right + brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_bottom = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_bottom + brush->gx_brush_width);

    /*make sure antiliasing is off*/
    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))

    /*consider alpha default mode is constant,value is 0xff
     * make sure for this before drawing a map */
    CHECK_DAVE_STATUS(d2_setalpha(dave, 0xff))

    GX_VALUE temp_width = brush->gx_brush_width;
    brush->gx_brush_width = 0;
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))


    if (brush->gx_brush_style & GX_BRUSH_PIXELMAP_FILL)
    {
        CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_texture))
        _gx_dave2d_set_texture(context,
                               dave,
                context->gx_draw_context_clip->gx_rectangle_left,
                context->gx_draw_context_clip->gx_rectangle_top,
                brush->gx_brush_pixelmap);
    }
    else
    {
        CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
        /*color doesn't work, so use alpha, set const alpha to target value, write it to framebuffer.*/
        CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)brush_color))
    }
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
    CHECK_DAVE_STATUS(d2_rendercircle(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_width)(D2_FIX4(r)), 0))

    brush->gx_brush_width = temp_width;
    if (brush->gx_brush_width > 0)
    {
        UINT brush_style =  brush->gx_brush_style;
        brush->gx_brush_style = (UINT)(brush->gx_brush_style & (UINT)(~(GX_BRUSH_PIXELMAP_FILL|GX_BRUSH_SOLID_FILL)));
        _gx_dave2d_circle_draw_8bpp(context, xcenter, ycenter, r);
        brush->gx_brush_style = brush_style;
    }
}

/*****************************************************************************/
/* Arc drawing using Dave2D                                                  */

VOID _gx_dave2d_arc_draw_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle)
{
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    INT sin1, cos1, sin2, cos2;
    d2_u32 flags;
    UINT temp_style = 0;

    if (brush->gx_brush_style & (GX_BRUSH_SOLID_FILL|GX_BRUSH_PIXELMAP_FILL))
    {
        temp_style = brush->gx_brush_style;
        brush->gx_brush_style = (UINT)(brush->gx_brush_style & (UINT)(~(GX_BRUSH_SOLID_FILL|GX_BRUSH_PIXELMAP_FILL)));
    }

    context->gx_draw_context_clip->gx_rectangle_top = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_top - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_left = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_left - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_right = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_right + brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_bottom = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_bottom + brush->gx_brush_width);

    INT s_angle =  - start_angle;
    INT e_angle =  - end_angle;

    sin1 = gx_utility_math_sin((s_angle - 90) * 256);
    cos1 = gx_utility_math_cos((s_angle - 90) * 256);

    sin2 = gx_utility_math_sin((e_angle + 90) * 256);
    cos2 = gx_utility_math_cos((e_angle + 90) * 256);

    /*if (end_angle - start_angle > 180)*/
    if ((s_angle - e_angle> 180) || (s_angle - e_angle < 0 ))
    {
        flags = d2_wf_concave;
    }
    else
    {
        flags = 0;
    }

    d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_outline))
    CHECK_DAVE_STATUS(d2_outlinewidth(dave, (d2_width)(D2_FIX4(brush->gx_brush_width))))
    /*color doesn't work, so use alpha, set const alpha to target value, write it to framebuffer.*/
    /* Just for 8bpp condition.*/
    CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)(brush->gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
    CHECK_DAVE_STATUS(d2_renderwedge(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_width)(D2_FIX4(r)),
        0, cos1 << 8, sin1 << 8, cos2 << 8, sin2 << 8, flags))

    if(0 != temp_style)
    {
        brush->gx_brush_style = temp_style;
    }
}

/*****************************************************************************/
VOID _gx_dave2d_arc_fill_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle)
{
    /* dave2d doesn't support chord (filled arc), so approximate with pie */
    _gx_display_list_flush(context -> gx_draw_context_display);

    GX_BRUSH *brush = &context->gx_draw_context_brush;
    GX_COLOR temp_color = brush->gx_brush_line_color;
    brush->gx_brush_line_color = brush->gx_brush_fill_color;

    _gx_display_driver_generic_arc_fill(context, xcenter, ycenter, r,start_angle, end_angle);
    brush->gx_brush_line_color = temp_color;
    _gx_display_list_open(context -> gx_draw_context_display);
}


/*****************************************************************************/
/* Render pie using Dave2D                                                   */

VOID _gx_dave2d_pie_fill_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle)
{
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    INT sin1, cos1, sin2, cos2;
    d2_u32 flags;
    GX_COLOR brush_color = brush->gx_brush_fill_color;
    d2_device *dave = _gx_dave2d_set_clip(context);


    context->gx_draw_context_clip->gx_rectangle_top = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_top - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_left = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_left - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_right = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_right + brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_bottom = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_bottom + brush->gx_brush_width);

    INT s_angle =  - start_angle;
    INT e_angle =  - end_angle;

    sin1 = gx_utility_math_sin((s_angle - 90) * 256);
    cos1 = gx_utility_math_cos((s_angle - 90) * 256);

    sin2 = gx_utility_math_sin((e_angle + 90) * 256);
    cos2 = gx_utility_math_cos((e_angle + 90) * 256);


    /* if (end_angle - start_angle > 180) */
    if ((s_angle - e_angle> 180) || (s_angle - e_angle < 0 ))
    {
        flags = d2_wf_concave;
    }
    else
    {
        flags = 0;
    }

    /* consider alpha default mode is constant,value is 0xff
       make sure for this before drawing a map */
    CHECK_DAVE_STATUS(d2_setalpha(dave, 0xff))
    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))///(dave, src, dst)

    GX_VALUE temp_width = brush->gx_brush_width;
    brush->gx_brush_width = 0;
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))


    if (brush->gx_brush_style & GX_BRUSH_PIXELMAP_FILL)
    {
        CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_texture))
        _gx_dave2d_set_texture(context,
                               dave,
                context->gx_draw_context_clip->gx_rectangle_left,
                context->gx_draw_context_clip->gx_rectangle_top,
                brush->gx_brush_pixelmap);
    }
    else
    {
        CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
        /*color doesn't work, so use alpha, set const alpha to target value, write it to
        framebuffer.*/
        CHECK_DAVE_STATUS(d2_setalpha(dave, (d2_alpha)(brush_color)))
    }
    CHECK_DAVE_STATUS(d2_setblendmode(dave,d2_bm_zero,d2_bm_zero))
    CHECK_DAVE_STATUS(d2_renderwedge(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_point)(D2_FIX4(r)),
        0, cos1 << 8, sin1 << 8, cos2 << 8, sin2 << 8, flags))
    brush->gx_brush_width = temp_width;
}


/*****************************************************************************/
/* GUIX non-aliased ellipse draw. Not supported by Dave2D so flush display   */
/* list and use standard software rendering                                  */

VOID _gx_dave2d_ellipse_draw_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_generic_ellipse_draw(context, xcenter, ycenter, a, b);
    _gx_display_list_open(context -> gx_draw_context_display);
}


/*****************************************************************************/
/* GUIX non-aliased ellipse draw. Not supported by Dave2D so flush display   */
/* list and use standard software rendering                                  */

VOID _gx_dave2d_wide_ellipse_draw_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_generic_wide_ellipse_draw(context, xcenter, ycenter, a, b);
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
/* GUIX ellipse fill. Not supported by Dave2D so flush display   */
/* list and use standard software rendering                                  */

VOID _gx_dave2d_ellipse_fill_8bpp(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_generic_ellipse_fill(context, xcenter, ycenter, a, b);
    _gx_display_list_open(context -> gx_draw_context_display);

}

#endif  /* is GUIX arc drawing support enabled? */

/*****************************************************************************/
/* canvas copy, performed after buffer toggle operation                      */
static void _gx_dave2d_copy_visible_to_working_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy)
{
    GX_RECTANGLE display_size;
    GX_RECTANGLE copy_clip;

    ULONG        *pGetRow;
    ULONG        *pPutRow;

    int          copy_width;
    int          copy_height;

    GX_DISPLAY *display = canvas->gx_canvas_display;
    d2_device *dave = (d2_device *) display->gx_display_accelerator;

    _gx_utility_rectangle_define(&display_size, 0, 0,
            (GX_VALUE)(display->gx_display_width - 1),
            (GX_VALUE)(display->gx_display_height - 1));
    copy_clip = *copy;

    copy_clip.gx_rectangle_left  = (GX_VALUE)(copy_clip.gx_rectangle_left & 0xfffe);
    copy_clip.gx_rectangle_top   = (GX_VALUE)(copy_clip.gx_rectangle_top & 0xfffe);
    copy_clip.gx_rectangle_right |= 1;
    copy_clip.gx_rectangle_bottom |= 1;

    // offset canvas within frame buffer
    _gx_utility_rectangle_shift(&copy_clip, canvas->gx_canvas_display_offset_x, canvas->gx_canvas_display_offset_y);

    _gx_utility_rectangle_overlap_detect(&copy_clip, &display_size, &copy_clip);
    copy_width  = (copy_clip.gx_rectangle_right - copy_clip.gx_rectangle_left + 1);
    copy_height = copy_clip.gx_rectangle_bottom - copy_clip.gx_rectangle_top + 1;

    if ((copy_width <= 0) ||
        (copy_height <= 0))
    {
        return;
    }

    pGetRow = (ULONG *) visible_frame; // copy into buffer 1
    pPutRow = (ULONG *) working_frame;

    d2_setalphablendmode(dave,d2_bm_one,d2_bm_zero);
    d2_setalpha(dave,0xff);

    CHECK_DAVE_STATUS(d2_framebuffer(dave, pPutRow,
                   (d2_u32)(canvas -> gx_canvas_x_resolution), (d2_u32)(canvas -> gx_canvas_x_resolution),
                   (d2_u32)(canvas -> gx_canvas_y_resolution), d2_mode_alpha8))

    CHECK_DAVE_STATUS(d2_cliprect(dave,
                copy_clip.gx_rectangle_left,
                copy_clip.gx_rectangle_top,
                copy_clip.gx_rectangle_right,
                copy_clip.gx_rectangle_bottom))

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *) pGetRow,
                  canvas->gx_canvas_x_resolution,
                  canvas->gx_canvas_x_resolution,
                  canvas->gx_canvas_y_resolution,
                  d2_mode_alpha8))

    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                                  copy_width, copy_height,
                                  (d2_blitpos)(copy_clip.gx_rectangle_left),
                                  (d2_blitpos)(copy_clip.gx_rectangle_top),
                                  (d2_width)(D2_FIX4(copy_width)),
                                  (d2_width)(D2_FIX4(copy_height)),
                                  (d2_point)(D2_FIX4(copy_clip.gx_rectangle_left)),
                                  (d2_point)(D2_FIX4(copy_clip.gx_rectangle_top)),
                                  d2_bf_no_blitctxbackup | d2_bf_usealpha))

    CHECK_DAVE_STATUS(d2_endframe(display -> gx_display_accelerator))
    CHECK_DAVE_STATUS(d2_startframe(display -> gx_display_accelerator))
}

/*****************************************************************************/
/* buffer toggle, done after sequence of canvas refresh drawing commands     */
VOID _gx_dave2d_buffer_toggle_8bpp (GX_CANVAS *canvas, GX_RECTANGLE *dirty)
{
    GX_PARAMETER_NOT_USED(dirty);
    GX_RECTANGLE Limit;
    GX_RECTANGLE Copy;
    GX_DISPLAY *display;
    int rotation_angle;

    display = canvas->gx_canvas_display;
    rotation_angle = sf_el_display_rotation_get(display->gx_display_handle);
    sf_el_frame_pointers_get(display->gx_display_handle, &visible_frame, &working_frame);

    _gx_utility_rectangle_define(&Limit, 0, 0,
                                 (GX_VALUE)(canvas->gx_canvas_x_resolution - 1),
                                 (GX_VALUE)(canvas->gx_canvas_y_resolution - 1));

    if (canvas->gx_canvas_memory != (GX_COLOR *)working_frame)
    {
        if (_gx_utility_rectangle_overlap_detect(&Limit, &canvas->gx_canvas_dirty_area, &Copy))
        {
            _gx_dave2d_rotate_canvas_to_working_8bpp(canvas, &Copy, rotation_angle);
        }
    }

    _gx_display_list_flush(display);
    _gx_display_list_open(display);

    sf_el_frame_toggle(canvas->gx_canvas_display->gx_display_handle, (GX_UBYTE **) &visible_frame);
    sf_el_frame_pointers_get(canvas->gx_canvas_display->gx_display_handle, &visible_frame, &working_frame);

    /* If canvas memory is pointing directly to frame buffer, toggle canvas memory */
    if (canvas->gx_canvas_memory == (GX_COLOR *) visible_frame)
    {
        canvas->gx_canvas_memory = (GX_COLOR *) working_frame;
    }

    if (_gx_utility_rectangle_overlap_detect(&Limit, &canvas->gx_canvas_dirty_area, &Copy))
    {
        if (canvas->gx_canvas_memory == (GX_COLOR *)working_frame)
        {
            /* Copies our canvas to the back buffer */
            _gx_dave2d_copy_visible_to_working_8bpp(canvas, &Copy);
        }
        else
        {
            _gx_dave2d_rotate_canvas_to_working_8bpp(canvas, &Copy, rotation_angle);
        }
    }
}
void _gx_dave2d_rotate_canvas_to_working_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy, int rotation_angle)
{
    GX_RECTANGLE display_size;
    GX_RECTANGLE copy_clip;
    d2_u32 mode;

    uint8_t   *pGetRow;

    int    copy_width;
    int    copy_height;
    int    copy_width_rotated;
    int    copy_height_rotated;

    GX_DISPLAY *display = canvas->gx_canvas_display;
    d2_device *dave = (d2_device *) display->gx_display_accelerator;

    d2_u8 fillmode_bkup = d2_getfillmode(dave);

    _gx_utility_rectangle_define(&display_size, 0, 0,
            (GX_VALUE)(display->gx_display_width - 1),
            (GX_VALUE)(display->gx_display_height - 1));

    copy_clip = *copy;

    // for copy region to align on 32-bit boundary

    /* for copy region to align on 32-bit boundry */
    copy_clip.gx_rectangle_left  = (GX_VALUE)(copy_clip.gx_rectangle_left & 0xfffe);
    copy_clip.gx_rectangle_top   = (GX_VALUE)(copy_clip.gx_rectangle_top & 0xfffe);
    copy_clip.gx_rectangle_right |= 1;
    copy_clip.gx_rectangle_bottom |= 1;
    mode = d2_mode_alpha8;

    d2_setalphablendmode(dave,d2_bm_one,d2_bm_zero);
    d2_setalpha(dave,0xff);

    // offset canvas within frame buffer
    _gx_utility_rectangle_shift(&copy_clip, canvas->gx_canvas_display_offset_x, canvas->gx_canvas_display_offset_y);
    _gx_utility_rectangle_overlap_detect(&copy_clip, &display_size, &copy_clip);
    copy_width  = (copy_clip.gx_rectangle_right - copy_clip.gx_rectangle_left + 1);
    copy_height = copy_clip.gx_rectangle_bottom - copy_clip.gx_rectangle_top + 1;

    if ((copy_width <= 0) ||
        (copy_height <= 0))
    {
        return;
    }

    pGetRow = (uint8_t *) canvas->gx_canvas_memory;
    pGetRow = pGetRow + ((canvas->gx_canvas_display_offset_y + copy_clip.gx_rectangle_top) * display->gx_display_width);
    pGetRow = pGetRow + (canvas->gx_canvas_display_offset_x + copy_clip.gx_rectangle_left);

    d2_border xmin = (d2_border)(copy_clip.gx_rectangle_left);
    d2_border ymin = (d2_border)(copy_clip.gx_rectangle_top);
    d2_border xmax = (d2_border)(copy_clip.gx_rectangle_right);
    d2_border ymax = (d2_border)(copy_clip.gx_rectangle_bottom);
    d2_point  x_texture_zero = (d2_point)(copy_clip.gx_rectangle_left);
    d2_point  y_texture_zero = (d2_point)(copy_clip.gx_rectangle_top);
    d2_s32    dxu  = (d2_s32)(D2_FIX16(1));
    d2_s32    dxv  = 0;
    d2_s32    dyu  = 0;
    d2_s32    dyv  = (d2_s32)(D2_FIX16(1));

    GX_VALUE    x_resolution;
    GX_VALUE    y_resolution;

    if (rotation_angle == 0)
    {
        xmin = (d2_border)(copy_clip.gx_rectangle_left);
        ymin = (d2_border)(copy_clip.gx_rectangle_top);
        xmax = (d2_border)(copy_clip.gx_rectangle_right);
        ymax = (d2_border)(copy_clip.gx_rectangle_bottom);
        x_texture_zero = (d2_point)(copy_clip.gx_rectangle_left);
        y_texture_zero = (d2_point)(copy_clip.gx_rectangle_top);
        dxu  = (d2_s32)(D2_FIX16(1));
        dxv  = 0;
        dyu  = 0;
        dyv  = (d2_s32)(D2_FIX16(1));
        copy_width_rotated  = copy_width;
        copy_height_rotated = copy_height;
        x_resolution   = canvas -> gx_canvas_x_resolution;
        y_resolution   = canvas -> gx_canvas_y_resolution;
    }
    else if (rotation_angle == 90)
    {
        int actual_video_width;
        int actual_video_height;

        sf_el_display_actual_size_get(display->gx_display_handle, &actual_video_width, &actual_video_height);
        if (copy_height > actual_video_width)
        {
            copy_height = actual_video_width;
        }
        if (copy_clip.gx_rectangle_bottom > actual_video_width)
        {
            copy_clip.gx_rectangle_bottom = (GX_VALUE)actual_video_width;
        }
        GX_VALUE clipped_display_height = display->gx_display_height;
        if (clipped_display_height > actual_video_width)
        {
            clipped_display_height = (GX_VALUE)actual_video_width;
        }

        xmin = (d2_border)(clipped_display_height - copy_clip.gx_rectangle_bottom - 1);
        ymin = (d2_border)(copy_clip.gx_rectangle_left);
        xmax = (d2_border)(clipped_display_height - copy_clip.gx_rectangle_top - 1);
        ymax = (d2_border)(copy_clip.gx_rectangle_right);
        x_texture_zero = (d2_point)(clipped_display_height - copy_clip.gx_rectangle_top -1);
        y_texture_zero = (d2_point)(copy_clip.gx_rectangle_left);
        dxu  = 0;
        dxv  = (d2_s32)(D2_FIX16(1));
        dyu  = (d2_s32)(-D2_FIX16(1));
        dyv  = 0;
        copy_width_rotated  = copy_height;
        copy_height_rotated = copy_width;
        x_resolution   = canvas -> gx_canvas_y_resolution;
        y_resolution   = canvas -> gx_canvas_x_resolution;
    }
    else if (rotation_angle == 180)
    {
        int         actual_video_width;
        int         actual_video_height;

        sf_el_display_actual_size_get(display->gx_display_handle, &actual_video_width, &actual_video_height);
        if (copy_width > actual_video_width)
        {
            copy_width = actual_video_width;
        }
        if (copy_clip.gx_rectangle_right > actual_video_width)
        {
            copy_clip.gx_rectangle_right = (GX_VALUE)actual_video_width;
        }
        GX_VALUE clipped_display_width = display->gx_display_width;
        if (clipped_display_width > actual_video_width)
        {
            clipped_display_width = (GX_VALUE)actual_video_width;
        }

        xmin = (d2_border)(clipped_display_width - copy_clip.gx_rectangle_right - 1);
        ymin = (d2_border)(display->gx_display_height - copy_clip.gx_rectangle_bottom -1);
        xmax = (d2_border)(clipped_display_width - copy_clip.gx_rectangle_left - 1);
        ymax = (d2_border)(display->gx_display_height - copy_clip.gx_rectangle_top - 1);
        x_texture_zero = (d2_point)(clipped_display_width - copy_clip.gx_rectangle_left -1);
        y_texture_zero = (d2_point)(display->gx_display_height - copy_clip.gx_rectangle_top - 1);
        dxu  = (d2_s32)(-D2_FIX16(1));
        dxv  = 0;
        dyu  = 0;
        dyv  = (d2_s32)(-D2_FIX16(1));
        copy_width_rotated  = copy_width;
        copy_height_rotated = copy_height;
        x_resolution   = canvas -> gx_canvas_x_resolution;
        y_resolution   = canvas -> gx_canvas_y_resolution;
    }
    else /* angle = 270 */
    {
        xmin = (d2_border)(copy_clip.gx_rectangle_top);
        ymin = (d2_border)(display->gx_display_width - copy_clip.gx_rectangle_right - 1);
        xmax = (d2_border)(copy_clip.gx_rectangle_bottom);
        ymax = (d2_border)(display->gx_display_width - copy_clip.gx_rectangle_left - 1);
        x_texture_zero = (d2_point)(copy_clip.gx_rectangle_top);
        y_texture_zero = (d2_point)(display->gx_display_width - copy_clip.gx_rectangle_left -1);
        dxu  = 0;
        dxv  = (d2_s32)(-D2_FIX16(1));
        dyu  = (d2_s32)(D2_FIX16(1));
        dyv  = 0;
        copy_width_rotated  = copy_height;
        copy_height_rotated = copy_width;
        x_resolution   = canvas -> gx_canvas_y_resolution;
        y_resolution   = canvas -> gx_canvas_x_resolution;
    }

    d2_s32 error;

    error = d2_framebuffer(dave, (uint16_t *) working_frame,
                   (d2_s32)x_resolution, (d2_u32)x_resolution, (d2_u32)y_resolution, (d2_s32)mode);

    error += d2_cliprect(dave, xmin, ymin, xmax, ymax);

    error += d2_setfillmode(dave, d2_fm_texture);

    error += d2_settexture(dave, pGetRow, display->gx_display_width, copy_width, copy_height, mode);

    error += d2_settexturemode(dave, 0);
    error += d2_settextureoperation(dave, d2_to_copy, d2_to_zero, d2_to_zero, d2_to_zero);
    error += d2_settexelcenter(dave, 0, 0);

    error = d2_settexturemapping(dave,
                         (d2_point)D2_FIX4(x_texture_zero),
                         (d2_point)D2_FIX4(y_texture_zero),
                         0, 0,
                         dxu, dxv, dyu, dyv);

    if ( (0 == rotation_angle) || (180 == rotation_angle))
    {
        error = d2_renderbox(dave,
                    (d2_point)D2_FIX4(xmin),
                    (d2_point)D2_FIX4(ymin),
                    (d2_width)D2_FIX4(copy_width_rotated),
                    (d2_width)D2_FIX4(copy_height_rotated));
    }
    else
    {
        // Render vertical stripes multiple times to reduce the texture cache miss for
        // 90 or 270 degree rotation.
        for(int x = xmin; x < (xmin + copy_width_rotated); x++)
        {
            error = d2_renderbox(dave,
                        (d2_point)D2_FIX4(x),
                        (d2_point)D2_FIX4(ymin),
                        (d2_width)D2_FIX4(1),
                        (d2_width)D2_FIX4(copy_height_rotated));
        }
    }
    
    d2_setfillmode(dave, fillmode_bkup);
}
#endif /* GX_USE_SYNERGY_DRW */

/*****************************************************************************/
/* 8-bit Palette setup for display hardware.                                 */
VOID _gx_display_driver_8bit_palette_assign(GX_DISPLAY *display, GX_COLOR *palette, INT count)
{
    display -> gx_display_palette = palette;
    display -> gx_display_palette_size = (UINT)count;

    sf_el_display__gx_display_8bit_palette_assign(display->gx_display_handle);
}

/*****************************************************************************/
/* software buffer toggle operation, not using Dave2D, rotate, 8bpp         */
void _gx_rotate_canvas_to_working_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy, int angle)
{
    GX_RECTANGLE display_size;
    GX_RECTANGLE copy_clip;

    GX_UBYTE       *pGetRow;
    GX_UBYTE       *pPutRow;
    GX_UBYTE       *pGet;
    GX_UBYTE       *pPut;

    int          copy_width;
    int          copy_height;
    int          canvas_stride;
    int          display_stride;
    int          row;
    int          col;

    GX_DISPLAY *display = canvas->gx_canvas_display;
    gx_utility_rectangle_define(&display_size, 0, 0,
                                (GX_VALUE)(display->gx_display_width - 1),
                                (GX_VALUE)(display->gx_display_height - 1));
    copy_clip = *copy;

    /* for copy region to align on 32-bit boundry */
    copy_clip.gx_rectangle_left  = (GX_VALUE)(copy_clip.gx_rectangle_left & 0xfffe);
    copy_clip.gx_rectangle_top   = (GX_VALUE)(copy_clip.gx_rectangle_top & 0xfffe);
    copy_clip.gx_rectangle_right |= 1;
    copy_clip.gx_rectangle_bottom |= 1;

    // offset canvas within frame buffer
    gx_utility_rectangle_shift(&copy_clip, canvas->gx_canvas_display_offset_x, canvas->gx_canvas_display_offset_y);

    gx_utility_rectangle_overlap_detect(&copy_clip, &display_size, &copy_clip);
    copy_width  = (copy_clip.gx_rectangle_right - copy_clip.gx_rectangle_left + 1);
    copy_height = copy_clip.gx_rectangle_bottom - copy_clip.gx_rectangle_top + 1;

    if ((copy_width <= 0) ||
        (copy_height <= 0))
    {
        return;
    }

    pGetRow = (GX_UBYTE *) canvas->gx_canvas_memory;
    pPutRow = (GX_UBYTE *) working_frame;

    display_stride = display->gx_display_height;
    canvas_stride = canvas->gx_canvas_x_resolution;

    /*Skip top lines.*/
    if(angle == 0)
    {
        pPutRow += copy_clip.gx_rectangle_top * canvas_stride;
        pPutRow += copy_clip.gx_rectangle_left;
    }
    else if (angle == 270)
    {
        pPutRow += (display->gx_display_width - 1 - copy_clip.gx_rectangle_left) * display_stride;
        pPutRow += copy_clip.gx_rectangle_top;
    }
    else if(angle == 90)
    {
        pPutRow += copy_clip.gx_rectangle_left * display_stride;
        pPutRow += (display_stride - 1);
        pPutRow -= copy_clip.gx_rectangle_top;
    }
    else/*angle = 180*/
    {
        pPutRow += (display->gx_display_height - copy_clip.gx_rectangle_top) * canvas_stride;
        pPutRow -= copy_clip.gx_rectangle_left + 1;
    }

    if(angle == 0 )
    {
        pGetRow += (canvas->gx_canvas_display_offset_y + copy_clip.gx_rectangle_top) * canvas_stride;
        pGetRow += (canvas->gx_canvas_display_offset_x + copy_clip.gx_rectangle_left) ;
        for (row = 0; row < copy_height; row++)
        {
            pGet = pGetRow;
            pPut = pPutRow;
            for (col = 0; col < copy_width; col++)
            {
                *pPut++ = *pGet++;
            }
            pGetRow += canvas_stride;
            pPutRow += canvas_stride;
        }
    }
    else if(angle == 180)
    {
        pGetRow += (canvas->gx_canvas_display_offset_y + copy_clip.gx_rectangle_top) * canvas_stride;
        pGetRow += (canvas->gx_canvas_display_offset_x + copy_clip.gx_rectangle_left) ;
        for (row = 0; row < copy_height; row++)
        {
            pGet = pGetRow;
            pPut = pPutRow;
            for (col = 0; col < copy_width; col++)
            {
                *pPut-- = *pGet++;
            }
            pGetRow += canvas_stride;
            pPutRow -= canvas_stride;
        }
    }
    else
    {
        pGetRow += (canvas->gx_canvas_display_offset_y + copy_clip.gx_rectangle_top) * canvas_stride;
        pGetRow += (canvas->gx_canvas_display_offset_x + copy_clip.gx_rectangle_left);
        for (row = 0; row < copy_height; row++)
        {
            pGet = pGetRow;
            pPut = pPutRow;

            for (col = 0; col < copy_width; col++)
            {
                *pPut = *pGet++;
                if (angle == 270)
                {
                    pPut -= display_stride;
                }
                else
                {
                    pPut += display_stride;
                }
            }

            if (angle == 270)
            {
                pPutRow++;
            }
            else
            {
                pPutRow--;
            }
            pGetRow += canvas_stride;
        }
    }
}

/*****************************************************************************/
/* canvas copy, performed after buffer toggle operation                      */
void _gx_copy_visible_to_working_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy)
{
    GX_RECTANGLE display_size;
    GX_RECTANGLE copy_clip;

    ULONG        *pGetRow;
    ULONG        *pPutRow;

    int          copy_width;
    int          copy_height;
    int          canvas_stride;
    int          display_stride;

    ULONG        *pGet;
    ULONG        *pPut;
    int          row;
    int          col;

    GX_DISPLAY *display = canvas->gx_canvas_display;

    _gx_utility_rectangle_define(&display_size, 0, 0,
            (GX_VALUE)(display->gx_display_width - 1),
            (GX_VALUE)(display->gx_display_height - 1));
    copy_clip = *copy;

    // for copy region to align on 32-bit boundry
    copy_clip.gx_rectangle_left  = (GX_VALUE)(copy_clip.gx_rectangle_left & 0xfffe);
    copy_clip.gx_rectangle_right |= 1;

    // offset canvas within frame buffer
    _gx_utility_rectangle_shift(&copy_clip, canvas->gx_canvas_display_offset_x, canvas->gx_canvas_display_offset_y);

    _gx_utility_rectangle_overlap_detect(&copy_clip, &display_size, &copy_clip);
    copy_width  = (copy_clip.gx_rectangle_right - copy_clip.gx_rectangle_left + 1);
    copy_height = copy_clip.gx_rectangle_bottom - copy_clip.gx_rectangle_top + 1;

    if ((copy_width <= 0) ||
        (copy_height <= 0))
    {
        return;
    }

    pGetRow = (ULONG *) visible_frame; // copy into buffer 1
    pPutRow = (ULONG *) working_frame;

    copy_width /= 4;
    canvas_stride = canvas->gx_canvas_x_resolution / 4;
    pPutRow += copy_clip.gx_rectangle_top * canvas_stride;
    pPutRow += copy_clip.gx_rectangle_left / 4;

    display_stride = display->gx_display_width / 4;
    pGetRow += (canvas->gx_canvas_display_offset_y + copy_clip.gx_rectangle_top) * (display_stride);
    pGetRow += (canvas->gx_canvas_display_offset_x + copy_clip.gx_rectangle_left) / 4;

    for (row = 0; row < copy_height; row++)
    {
        pGet = pGetRow;
        pPut = pPutRow;

        for (col = 0; col < copy_width; col++)
        {
            *pPut++ = *pGet++;
        }
        pPutRow += canvas_stride;
        pGetRow += display_stride;
    }
}

/*****************************************************************************/
/* buffer toggle, done after sequence of canvas refresh drawing commands     */
VOID _gx_synergy_buffer_toggle_8bpp(GX_CANVAS *canvas, GX_RECTANGLE *dirty)
{
    GX_RECTANGLE Limit;
    GX_RECTANGLE Copy;
    GX_DISPLAY *display;
    GX_PARAMETER_NOT_USED(dirty);
    int rotation_angle;

    display = canvas->gx_canvas_display;

    _gx_utility_rectangle_define(&Limit, 0, 0,
                                 (GX_VALUE)(canvas->gx_canvas_x_resolution - 1),
                                 (GX_VALUE)(canvas->gx_canvas_y_resolution - 1));

    rotation_angle = sf_el_display_rotation_get(display->gx_display_handle);
    sf_el_frame_pointers_get(display->gx_display_handle, &visible_frame, &working_frame);

    if(canvas->gx_canvas_memory != (GX_COLOR *)working_frame)
    {
        if (_gx_utility_rectangle_overlap_detect(&Limit, &canvas->gx_canvas_dirty_area, &Copy))
        {
            _gx_rotate_canvas_to_working_8bpp(canvas, &Copy, rotation_angle);
        }
    }

    sf_el_frame_toggle(canvas->gx_canvas_display->gx_display_handle, &visible_frame);
    sf_el_frame_pointers_get(canvas->gx_canvas_display->gx_display_handle, &visible_frame, &working_frame);

    if (canvas->gx_canvas_memory == (GX_COLOR *) visible_frame)
    {
        canvas->gx_canvas_memory = (GX_COLOR *) working_frame;
    }

    if (_gx_utility_rectangle_overlap_detect(&Limit, &canvas->gx_canvas_dirty_area, &Copy))
    {
        if(canvas->gx_canvas_memory == (GX_COLOR *) working_frame)
        {
            _gx_copy_visible_to_working_8bpp(canvas, &Copy);
        }
        else
        {
            _gx_rotate_canvas_to_working_8bpp(canvas, &Copy, rotation_angle);
        }
    }
}

