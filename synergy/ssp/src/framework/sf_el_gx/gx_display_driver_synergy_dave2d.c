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
/** GUIX Display Driver component                                         */
/**************************************************************************/

#include    <stdio.h>
#include    <string.h>

#include    "gx_api.h"
#include    "gx_display.h"
#include    "gx_utility.h"

/** indicator for the number of visible frame buffer */
static GX_UBYTE             *visible_frame;
static GX_UBYTE             *working_frame;

// functions provided by sf_el_gx.c :
extern void sf_el_frame_toggle (ULONG _display_handle,  GX_UBYTE **visible);
extern void sf_el_frame_pointers_get(ULONG display_handle, GX_UBYTE **visible, GX_UBYTE **working);
extern int  sf_el_display_rotation_get(ULONG display_handle);
extern void sf_el_display_actual_size_get(ULONG display_handle, int *width, int *height);

void _gx_copy_visible_to_working(GX_CANVAS *canvas, GX_RECTANGLE *copy);
void _gx_rotate_canvas_to_working_16bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy, int angle);
void _gx_rotate_canvas_to_working_32bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy, int angle);
void _gx_synergy_buffer_toggle(GX_CANVAS *canvas, GX_RECTANGLE *dirty);

#if (GX_USE_SYNERGY_DRW == 1)
#include "dave_driver.h"

// max number of error values we will keep

#ifndef GX_DISABLE_ERROR_CHECKING
#define LOG_DAVE_ERRORS
#define DAVE_ERROR_LIST_SIZE 4
#endif

// space used to store int to fixed point polygon vertices
#define MAX_POLYGON_VERTICES GX_POLYGON_MAX_EDGE_NUM

// number of display list command issues to issue a display list flush
#define GX_SYNERGY_DRW_DL_COMMAND_COUNT_TO_REFRESH (85)

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

static GX_BYTE      last_font_bits = 0;
static GX_BYTE      dave_error_list_index = 0;
static GX_BYTE      dave_error_count = 0;

static GX_BOOL      display_list_flushed = GX_FALSE;
static d2_color     (*_gx_d2_color)(GX_COLOR);

// partial palettes used for drawing 1bpp and 4bpp fonts

const d2_color mono_palette[2] = {
        0x00ffffff,
        0xffffffff
};

const d2_color gray_palette[16] = {
        0x00ffffff,
        0x11ffffff,
        0x22ffffff,
        0x33ffffff,
        0x44ffffff,
        0x55ffffff,
        0x66ffffff,
        0x77ffffff,
        0x88ffffff,
        0x99ffffff,
        0xaaffffff,
        0xbbffffff,
        0xccffffff,
        0xddffffff,
        0xeeffffff,
        0xffffffff,
};

#if defined(LOG_DAVE_ERRORS)
static d2_s32   dave_error_list[DAVE_ERROR_LIST_SIZE];
static d2_s32   dave_status;

void _gx_log_dave_error(d2_s32 status);
int _gx_get_dave_error(int get_index);
void _gx_display_list_flush(GX_DISPLAY *display);
void _gx_display_list_open(GX_DISPLAY *display);
static d2_color _gx_rgb565_to_888(GX_COLOR color);
VOID _gx_dave2d_drawing_initiate(GX_DISPLAY *display, GX_CANVAS *canvas);
VOID _gx_dave2d_drawing_complete(GX_DISPLAY *display, GX_CANVAS *canvas);
d2_device *_gx_dave2d_set_clip(GX_DRAW_CONTEXT *context);
VOID _gx_dave2d_horizontal_line(GX_DRAW_CONTEXT *context,
                                INT xstart, INT xend, INT ypos, INT width, GX_COLOR color);
VOID _gx_dave2d_vertical_line(GX_DRAW_CONTEXT *context,
                              INT ystart, INT yend, INT xpos, INT width, GX_COLOR color);
VOID _gx_dave2d_canvas_copy(GX_CANVAS *canvas, GX_CANVAS *composite);
VOID _gx_dave2d_canvas_blend(GX_CANVAS *canvas, GX_CANVAS *composite);
VOID _gx_dave2d_simple_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID _gx_dave2d_simple_wide_line(GX_DRAW_CONTEXT *context, INT xstart, INT ystart,
                                 INT xend, INT yend);
VOID _gx_dave2d_aliased_line(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID _gx_dave2d_aliased_line(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID _gx_dave2d_horizontal_pattern_line_draw_565(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos);
VOID _gx_dave2d_horizontal_pattern_line_draw_888(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos);
VOID _gx_dave2d_vertical_pattern_line_draw_565(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos);
VOID _gx_dave2d_vertical_pattern_line_draw_888(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos);
VOID _gx_dave2d_aliased_wide_line(GX_DRAW_CONTEXT *context, INT xstart,
                                  INT ystart, INT xend, INT yend);
VOID _gx_dave2d_pixelmap_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap);
VOID _gx_dave2d_pixelmap_blend(GX_DRAW_CONTEXT *context, INT xpos, INT ypos,
                                          GX_PIXELMAP *pixelmap, GX_UBYTE alpha);
VOID _gx_dave2d_horizontal_pixelmap_line_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, INT xstart, INT xend, INT y, GX_PIXELMAP *pixelmap);
VOID _gx_dave2d_set_texture(GX_DRAW_CONTEXT *context,d2_device *dave, int xpos, int ypos, GX_PIXELMAP *map);
VOID _gx_dave2d_polygon_draw(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num);
VOID _gx_dave2d_polygon_fill(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num);
VOID _gx_dave2d_pixel_write_565(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR color);
VOID _gx_dave2d_pixel_write_888(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR color);
VOID _gx_dave2d_pixel_blend_565(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR fcolor, GX_UBYTE alpha);
VOID _gx_dave2d_pixel_blend_888(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR fcolor, GX_UBYTE alpha);
VOID _gx_dave2d_block_move(GX_DRAW_CONTEXT *context, GX_RECTANGLE *block, INT xshift, INT yshift);
VOID _gx_dave2d_alphamap_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap);
VOID _gx_dave2d_glyph_8bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID _gx_dave2d_glyph_4bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID _gx_dave2d_glyph_1bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID _gx_dave2d_aliased_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_dave2d_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_dave2d_circle_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_dave2d_pie_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_dave2d_aliased_arc_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_dave2d_arc_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_dave2d_arc_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_dave2d_aliased_ellipse_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
VOID _gx_dave2d_ellipse_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
VOID _gx_dave2d_ellipse_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
void _gx_dave2d_copy_visible_to_working(GX_CANVAS *canvas, GX_RECTANGLE *copy);
void _gx_dave2d_rotate_canvas_to_working(GX_CANVAS *canvas, GX_RECTANGLE *copy, int rotation_angle);
void _gx_dave2d_buffer_toggle(GX_CANVAS *canvas, GX_RECTANGLE *dirty);
VOID _gx_synergy_jpeg_draw (GX_DRAW_CONTEXT *p_context, INT x, INT y, GX_PIXELMAP *p_pixelmap);

/*****************************************************************************/
/* add error status code to FIFO list of recent errors                       */
void _gx_log_dave_error(d2_s32 status)
{
    GX_PARAMETER_NOT_USED(status);
    dave_error_list[dave_error_list_index] = dave_status;
    if (dave_error_count < DAVE_ERROR_LIST_SIZE)
    {
        dave_error_count++;
    }
    dave_error_list_index++;
    if (dave_error_list_index >= DAVE_ERROR_LIST_SIZE)
    {
        dave_error_list_index = 0;
    }
}

/*****************************************************************************/
/* retrieve Dave2D error code from FIFO list                                 */
int _gx_get_dave_error(int get_index)
{
    if (get_index > dave_error_count)
    {
        return 0;
    }

    int list_index = dave_error_list_index;
    while(get_index > 0)
    {
        list_index--;
        if (list_index < 0)
        {
            list_index = DAVE_ERROR_LIST_SIZE;
        }
        get_index--;
    }
    return dave_error_list[list_index];
}

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

/*****************************************************************************/
/* close and execute current display list, block until completed              */
void _gx_display_list_flush(GX_DISPLAY *display)
{
    if (!display_list_flushed && d2_commandspending(display->gx_display_accelerator))
    {
        CHECK_DAVE_STATUS(d2_endframe(display -> gx_display_accelerator))
        CHECK_DAVE_STATUS(d2_flushframe(display -> gx_display_accelerator))
        display_list_flushed = GX_TRUE;
    }
}

/*****************************************************************************/
/* open display list for drawing commands                                    */
void _gx_display_list_open(GX_DISPLAY *display)
{
    if (display_list_flushed)
    {
        CHECK_DAVE_STATUS(d2_startframe(display -> gx_display_accelerator))
        display_list_flushed = GX_FALSE;
    }
}

/*****************************************************************************/
static d2_color _gx_rgb565_to_888(GX_COLOR color)
{
    d2_color out_color;
    out_color = (((color & 0xf800) << 8) | ((color & 0x7e0) << 5) | ((color & 0x1f) << 3));
    return out_color;
}

/*****************************************************************************/
static d2_color _gx_xrgb_to_xrgb(GX_COLOR color)
{
    d2_color out_color = (d2_color) color;
    return out_color;
}

/*****************************************************************************/
/* called by _gx_canvas_drawing_initiate. Close previous frame, set new      */
/* canvas drawing address.                                                   */
VOID _gx_dave2d_drawing_initiate(GX_DISPLAY *display, GX_CANVAS *canvas)
{
    /* make sure previous dlist is done executing */
d2_u32 mode = d2_mode_rgb565;
    switch(display->gx_display_color_format)
    {
    case GX_COLOR_FORMAT_565RGB:
        mode = d2_mode_rgb565;
        _gx_d2_color = _gx_rgb565_to_888;
        break;

    case GX_COLOR_FORMAT_24XRGB:
    case GX_COLOR_FORMAT_32ARGB:
        mode = d2_mode_argb8888;
        _gx_d2_color = _gx_xrgb_to_xrgb;
        break;

    case GX_COLOR_FORMAT_8BIT_PALETTE:
        mode = d2_mode_alpha8;
        break;
    }

    CHECK_DAVE_STATUS(d2_endframe(display -> gx_display_accelerator))

    /* trigger execution of previous display list, switch to new display list */
    CHECK_DAVE_STATUS(d2_startframe(display -> gx_display_accelerator))
    CHECK_DAVE_STATUS(d2_framebuffer(display -> gx_display_accelerator, canvas -> gx_canvas_memory,
                                     (d2_s32)(canvas -> gx_canvas_x_resolution), (d2_u32)(canvas -> gx_canvas_x_resolution),
                                     (d2_u32)(canvas -> gx_canvas_y_resolution), (d2_s32)mode))
}

/*****************************************************************************/
/* drawing complete- do nothing currently. The buffer toggle function takes  */
/* care of toggling the display lists.                                       */
VOID _gx_dave2d_drawing_complete(GX_DISPLAY *display, GX_CANVAS *canvas)
{
    GX_PARAMETER_NOT_USED(display);  
    GX_PARAMETER_NOT_USED(canvas);
}

/*****************************************************************************/
/* Assign clipping rectangle based on GUIX drawing context information       */
d2_device *_gx_dave2d_set_clip(GX_DRAW_CONTEXT *context)
{
d2_device *dave = context -> gx_draw_context_display -> gx_display_accelerator;

    if (context -> gx_draw_context_clip)
    {
        CHECK_DAVE_STATUS(d2_cliprect(dave,
                    context -> gx_draw_context_clip -> gx_rectangle_left,
                    context -> gx_draw_context_clip -> gx_rectangle_top,
                    context -> gx_draw_context_clip -> gx_rectangle_right,
                    context -> gx_draw_context_clip -> gx_rectangle_bottom))
    }
    else
    {
        CHECK_DAVE_STATUS(d2_cliprect(dave, 0, 0,
                                      (d2_border)(context -> gx_draw_context_canvas -> gx_canvas_x_resolution - 1),
                                      (d2_border)(context -> gx_draw_context_canvas -> gx_canvas_y_resolution - 1)))
    }
    return dave;
}

/*****************************************************************************/
/* draw a horizontal line using Dave2D                                       */
VOID _gx_dave2d_horizontal_line(GX_DRAW_CONTEXT *context,
                               INT xstart, INT xend, INT ypos, INT width, GX_COLOR color)
{
d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(color)))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))
    CHECK_DAVE_STATUS(d2_renderbox(dave, (d2_point)(D2_FIX4(xstart)), (d2_point)(D2_FIX4(ypos)),
                                   (d2_point)(D2_FIX4(xend - xstart + 1)), (d2_point)(D2_FIX4(width))))
}

/*****************************************************************************/
/* draw a vertical line using Dave2D                                         */
VOID _gx_dave2d_vertical_line(GX_DRAW_CONTEXT *context,
                             INT ystart, INT yend, INT xpos, INT width, GX_COLOR color)
{
d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(color)))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_solid))
    CHECK_DAVE_STATUS(d2_renderbox(dave, (d2_point)(D2_FIX4(xpos)), (d2_point)(D2_FIX4(ystart)),
                                   (d2_point)(D2_FIX4(width)), (d2_point)(D2_FIX4(yend - ystart + 1))))
}

/*****************************************************************************/
/* copy canvas using Dave2D                                                  */
VOID _gx_dave2d_canvas_copy(GX_CANVAS *canvas, GX_CANVAS *composite)
{
    d2_u32 mode;
    GX_DISPLAY *display = canvas->gx_canvas_display;
    d2_device *dave = display -> gx_display_accelerator;

    CHECK_DAVE_STATUS(d2_cliprect(dave,
                                  composite->gx_canvas_dirty_area.gx_rectangle_left,
                                  composite->gx_canvas_dirty_area.gx_rectangle_top,
                                  composite->gx_canvas_dirty_area.gx_rectangle_right,
                                  composite->gx_canvas_dirty_area.gx_rectangle_bottom));

    switch (display->gx_display_color_format)
    {
    case GX_COLOR_FORMAT_565RGB:
        mode = d2_mode_rgb565;
        break;

    case GX_COLOR_FORMAT_24XRGB:
    case GX_COLOR_FORMAT_32ARGB:
        mode = d2_mode_argb8888;
        break;

    case GX_COLOR_FORMAT_8BIT_PALETTE:
        mode = d2_mode_alpha8;
    break;

    default:
        return;
    }

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *) canvas->gx_canvas_memory,
                           canvas->gx_canvas_x_resolution,
                           canvas->gx_canvas_x_resolution,
                           canvas->gx_canvas_y_resolution, mode))

    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                canvas->gx_canvas_x_resolution,
                canvas->gx_canvas_y_resolution,
                0, 0,
                (d2_width)(D2_FIX4(canvas->gx_canvas_x_resolution)),
                (d2_width)(D2_FIX4(canvas->gx_canvas_y_resolution)),
                (d2_point)(D2_FIX4(canvas->gx_canvas_display_offset_x)),
                (d2_point)(D2_FIX4(canvas->gx_canvas_display_offset_y)),
                d2_bf_no_blitctxbackup))
}

/*****************************************************************************/
/* blend canvas with background using Dave2D                                 */
VOID _gx_dave2d_canvas_blend(GX_CANVAS *canvas, GX_CANVAS *composite)
{
    d2_u32 mode;
    GX_DISPLAY *display = canvas->gx_canvas_display;
    d2_device *dave = display -> gx_display_accelerator;

    CHECK_DAVE_STATUS(d2_cliprect(dave,
                                  composite->gx_canvas_dirty_area.gx_rectangle_left,
                                  composite->gx_canvas_dirty_area.gx_rectangle_top,
                                  composite->gx_canvas_dirty_area.gx_rectangle_right,
                                  composite->gx_canvas_dirty_area.gx_rectangle_bottom));

    switch (display->gx_display_color_format)
    {
    case GX_COLOR_FORMAT_565RGB:
        mode = d2_mode_rgb565;
        break;

    case GX_COLOR_FORMAT_24XRGB:
    case GX_COLOR_FORMAT_32ARGB:
        mode = d2_mode_argb8888;
        break;

    case GX_COLOR_FORMAT_8BIT_PALETTE:
        mode = d2_mode_alpha8;
        break;

    default:
        return;
    }

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *) canvas->gx_canvas_memory,
                           canvas->gx_canvas_x_resolution,
                           canvas->gx_canvas_x_resolution,
                           canvas->gx_canvas_y_resolution, mode))

    // set the alpha blend value:
    CHECK_DAVE_STATUS(d2_setalpha(dave, canvas->gx_canvas_alpha))

    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                canvas->gx_canvas_x_resolution,
                canvas->gx_canvas_y_resolution,
                0, 0,
                (d2_width)(D2_FIX4(canvas->gx_canvas_x_resolution)),
                (d2_width)(D2_FIX4(canvas->gx_canvas_y_resolution)),
                (d2_point)(D2_FIX4(canvas->gx_canvas_display_offset_x)),
                (d2_point)(D2_FIX4(canvas->gx_canvas_display_offset_y)),
                d2_bf_no_blitctxbackup))

    // set the alpha blend value:
    CHECK_DAVE_STATUS(d2_setalpha(dave, 0xff))
}

/*****************************************************************************/
/* Draw non-aliased simple line using Dave2D                                 */
VOID _gx_dave2d_simple_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend)
{
d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(context->gx_draw_context_brush.gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_renderline(dave, (d2_point)(D2_FIX4(xstart)), (d2_point)(D2_FIX4(ystart)),
                                    (d2_point)(D2_FIX4(xend)), (d2_point)(D2_FIX4(yend)),
                                    (d2_width)(D2_FIX4(context -> gx_draw_context_brush.gx_brush_width)), d2_le_exclude_none))
}

/*****************************************************************************/
/* Draw non-aliased wide line using Dave2D                                   */
VOID _gx_dave2d_simple_wide_line(GX_DRAW_CONTEXT *context, INT xstart, INT ystart,
                                INT xend, INT yend)
{
d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(context -> gx_draw_context_brush.gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_renderline(dave, (d2_point)(D2_FIX4(xstart)), (d2_point)(D2_FIX4(ystart)),
                                    (d2_point)(D2_FIX4(xend)), (d2_point)(D2_FIX4(yend)),
                                    (d2_width)(D2_FIX4(context -> gx_draw_context_brush.gx_brush_width)), d2_le_exclude_none))
}

/*****************************************************************************/
/* Draw anti-aliased line using Dave2D                                       */
VOID _gx_dave2d_aliased_line(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend)
{
d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 1))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(context -> gx_draw_context_brush.gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_renderline(dave, (d2_point)(D2_FIX4(xstart)), (d2_point)(D2_FIX4(ystart)), 
                                    (d2_point)(D2_FIX4(xend)), (d2_point)(D2_FIX4(yend)),
                                    (d2_width)(D2_FIX4(context -> gx_draw_context_brush.gx_brush_width)), d2_le_exclude_none))
}

/*****************************************************************************/
/* draw horizontal pattern line using Dave2D                                 */
VOID _gx_dave2d_horizontal_pattern_line_draw_565(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_16bpp_horizontal_pattern_line_draw(context, xstart, xend, ypos);
    _gx_display_list_open(context -> gx_draw_context_display);
}


VOID _gx_dave2d_horizontal_pattern_line_draw_888(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_32bpp_horizontal_pattern_line_draw(context, xstart, xend, ypos);
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
/* Draw vertial pattern line using Dave2D                                    */
VOID _gx_dave2d_vertical_pattern_line_draw_565(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_16bpp_vertical_pattern_line_draw(context, ystart, yend, xpos);
    _gx_display_list_open(context -> gx_draw_context_display);
}

VOID _gx_dave2d_vertical_pattern_line_draw_888(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_32bpp_vertical_pattern_line_draw(context, ystart, yend, xpos);
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
/* Draw anti-aliased wide line using Dave2D                                  */
VOID _gx_dave2d_aliased_wide_line(GX_DRAW_CONTEXT *context, INT xstart,
                                        INT ystart, INT xend, INT yend)
{
d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 1))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(context -> gx_draw_context_brush.gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_renderline(dave, (d2_point)(D2_FIX4(xstart)), (d2_point)(D2_FIX4(ystart)),
                                    (d2_point)(D2_FIX4(xend)), (d2_point)(D2_FIX4(yend)),
                                    (d2_width)(D2_FIX4(context -> gx_draw_context_brush.gx_brush_width)), d2_le_exclude_none))
}

/*****************************************************************************/
/* Draw pixelmap using Dave2D. Currently 8bpp, 16bpp, and 32 bpp source      */
/* formats are supported. Others may be added. Optional RLE compression      */
/* of all formats.                                                           */
VOID _gx_dave2d_pixelmap_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap)
{
d2_u32  mode;

    switch (pixelmap -> gx_pixelmap_format)
    {
    case GX_COLOR_FORMAT_8BIT_PALETTE:
        mode = d2_mode_i8|d2_mode_clut;
        break;

    case GX_COLOR_FORMAT_565RGB:
        mode = d2_mode_rgb565;
        break;

    case GX_COLOR_FORMAT_24XRGB:
    case GX_COLOR_FORMAT_32ARGB:
        mode = d2_mode_argb8888;
        break;

    default:
        return;
    }

    if (pixelmap -> gx_pixelmap_flags & GX_PIXELMAP_COMPRESSED)
    {
        mode |= d2_mode_rle;
    }

    d2_device *dave = _gx_dave2d_set_clip(context);

    if ((mode & d2_mode_clut) == d2_mode_clut)
    {
        CHECK_DAVE_STATUS(d2_settexclut(dave, (d2_color *) pixelmap->gx_pixelmap_aux_data))
    }

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *)pixelmap -> gx_pixelmap_data,
                           pixelmap -> gx_pixelmap_width, pixelmap -> gx_pixelmap_width,
                           pixelmap -> gx_pixelmap_height, mode))

    mode = d2_bf_no_blitctxbackup;

    if (pixelmap -> gx_pixelmap_flags & GX_PIXELMAP_ALPHA)
    {
        mode |= d2_bf_usealpha;
    }

    if(pixelmap->gx_pixelmap_flags & GX_PIXELMAP_TRANSPARENT)
    {
        /*8bit palette must use alpha*/
        mode |= d2_bf_usealpha;
    }

    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                         pixelmap -> gx_pixelmap_width,
                         pixelmap -> gx_pixelmap_height,
                         0, 0,
                         (d2_width)(D2_FIX4(pixelmap -> gx_pixelmap_width)),
                         (d2_width)(D2_FIX4(pixelmap -> gx_pixelmap_height)),
                         (d2_point)(D2_FIX4(xpos)),
                         (d2_point)(D2_FIX4(ypos)), mode))
}

/*****************************************************************************/
/* alpha-blend pixelmap using Dave2D                                         */
VOID _gx_dave2d_pixelmap_blend(GX_DRAW_CONTEXT *context, INT xpos, INT ypos,
                                      GX_PIXELMAP *pixelmap, GX_UBYTE alpha)
{
d2_u32  mode = d2_mode_rgb565;

    switch (pixelmap -> gx_pixelmap_format)
    {
    case GX_COLOR_FORMAT_8BIT_PALETTE:
        mode = d2_mode_i8|d2_mode_clut;
        break;

    case GX_COLOR_FORMAT_565RGB:
        mode = d2_mode_rgb565;
        break;

    case GX_COLOR_FORMAT_24XRGB:
    case GX_COLOR_FORMAT_32ARGB:
        mode = d2_mode_argb8888;
        break;
    }

    if (pixelmap -> gx_pixelmap_flags & GX_PIXELMAP_COMPRESSED)
    {
        mode |= d2_mode_rle;
    }

    d2_device *dave = _gx_dave2d_set_clip(context);

    if ((mode & d2_mode_clut) == d2_mode_clut)
    {
        d2_settexclut(dave, (d2_color *) pixelmap->gx_pixelmap_aux_data);
    }

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *)pixelmap -> gx_pixelmap_data,
                           pixelmap -> gx_pixelmap_width, pixelmap -> gx_pixelmap_width,
                           pixelmap -> gx_pixelmap_height, mode))

    mode = d2_bf_no_blitctxbackup;

    if (pixelmap -> gx_pixelmap_flags & GX_PIXELMAP_ALPHA)
    {
        mode |= d2_bf_usealpha;
    }

    /* set the alpha blend value: */
    CHECK_DAVE_STATUS(d2_setalpha(dave, alpha))

    /* do the bitmap drawing: */
    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                         pixelmap -> gx_pixelmap_width,
                         pixelmap -> gx_pixelmap_height,
                         0, 0,
                         (d2_width)(D2_FIX4(pixelmap -> gx_pixelmap_width)),
                         (d2_width)(D2_FIX4(pixelmap -> gx_pixelmap_height)),
                         (d2_point)(D2_FIX4(xpos)), (d2_point)(D2_FIX4(ypos)), mode))

    /* reset the alpha value: */
    CHECK_DAVE_STATUS(d2_setalpha(dave, 0xff))
}

/*****************************************************************************/
/* Draw pixelmap using Dave2D.  Optional RLE compression                     */
/* of all formats.                                                           */
VOID _gx_dave2d_horizontal_pixelmap_line_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, INT xstart, INT xend, INT y, GX_PIXELMAP *pixelmap)
{
GX_RECTANGLE old_clip;

    old_clip = *context->gx_draw_context_clip;
    context->gx_draw_context_clip->gx_rectangle_left = (GX_VALUE)xstart;
    context->gx_draw_context_clip->gx_rectangle_right = (GX_VALUE)xend;
    context->gx_draw_context_clip->gx_rectangle_top = (GX_VALUE)y;
    context->gx_draw_context_clip->gx_rectangle_bottom = (GX_VALUE)y;

    _gx_dave2d_pixelmap_draw(context, xpos, ypos, pixelmap);

    *context->gx_draw_context_clip = old_clip;
}


/*****************************************************************************/
/* support function used to apply texture source for all shape drawing       */
VOID _gx_dave2d_set_texture(GX_DRAW_CONTEXT *context,d2_device *dave, int xpos, int ypos, GX_PIXELMAP *map)//GUOPENGJIE???
{
    GX_PARAMETER_NOT_USED(context);
    d2_u32 format = d2_mode_rgb565;

    switch(map->gx_pixelmap_format)
    {
    case GX_COLOR_FORMAT_565RGB:
        format = d2_mode_rgb565;
        break;

    case GX_COLOR_FORMAT_4444ARGB:
        format = d2_mode_argb4444;
        break;

    case GX_COLOR_FORMAT_24XRGB:
    case GX_COLOR_FORMAT_32ARGB:
        format = d2_mode_argb8888;
        break;

    case GX_COLOR_FORMAT_8BIT_ALPHAMAP:
        format = d2_mode_alpha8;
        break;

    case GX_COLOR_FORMAT_8BIT_PALETTE:
        format = d2_mode_i8|d2_mode_clut;
        CHECK_DAVE_STATUS(d2_settexclut(dave, (d2_color *) map->gx_pixelmap_aux_data))
        break;
    }

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

    CHECK_DAVE_STATUS(d2_settextureoperation(dave, d2_to_one, d2_to_copy, d2_to_copy, d2_to_copy))
    CHECK_DAVE_STATUS(d2_settexelcenter(dave, 0, 0))
    CHECK_DAVE_STATUS(d2_settexturemapping(dave,
                                           (d2_point)(xpos << 4),
                                           (d2_point)(ypos << 4),
                                           0, 0,
                                           1 << 16, 0,
                                           0, 1 << 16))
}
/*****************************************************************************/
/* GUIX polygon draw utilizing Dave2D                                        */
VOID _gx_dave2d_polygon_draw(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num)
{
    int loop;
    int index;
    GX_VALUE val;
    d2_point data[MAX_POLYGON_VERTICES * 2];
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    d2_color brush_color = brush->gx_brush_line_color;

    // if brush width = 0, return.
    if (brush->gx_brush_width < 1)
    {
        return;
    }

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
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_outline))
    CHECK_DAVE_STATUS(d2_outlinewidth(dave, (d2_width)(D2_FIX4(brush->gx_brush_width))))
    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))

    if (brush->gx_brush_style & GX_BRUSH_ROUND)
    {
        CHECK_DAVE_STATUS(d2_setlinejoin(dave, d2_lj_round))
    }
    else
    {
        CHECK_DAVE_STATUS(d2_setlinejoin(dave, d2_lj_miter))
    }
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(brush_color)))
    CHECK_DAVE_STATUS(d2_renderpolygon(dave, data, (d2_u32)num, 0))

}


/*****************************************************************************/
/* GUIX filled polygon rendering using Dave2D                                */
VOID _gx_dave2d_polygon_fill(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num)
{
    int loop;
    int index;
    GX_VALUE val;
    d2_point data[MAX_POLYGON_VERTICES * 2];
    GX_BRUSH *brush = &context->gx_draw_context_brush;

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
        CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(brush->gx_brush_fill_color)))
    }
    d2_renderpolygon(dave, data, (d2_u32)num, 0);

}


/*****************************************************************************/
/* GUIX display driver pixel write. Must first flush the Dave2D display list */
/* to insure order of operation.                                             */
VOID _gx_dave2d_pixel_write_565(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR color)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_16bpp_pixel_write(context, x, y, color);
    _gx_display_list_open(context -> gx_draw_context_display);
}

VOID _gx_dave2d_pixel_write_888(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR color)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_32bpp_pixel_write(context, x, y, color);
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
/* GUIX display driver pixel blend. Must first flush the Dave2D display list */
/* to insure order of operation.                                             */
VOID _gx_dave2d_pixel_blend_565(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR fcolor, GX_UBYTE alpha)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_565rgb_pixel_blend(context, x, y, fcolor, alpha);
    _gx_display_list_open(context -> gx_draw_context_display);
}


VOID _gx_dave2d_pixel_blend_888(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR fcolor, GX_UBYTE alpha)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_32argb_pixel_blend(context, x, y, fcolor, alpha);
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
/* Move a block of pixels within working canvas memory.                      */
/* Mainly used for fast scrolling.                                           */
VOID _gx_dave2d_block_move(GX_DRAW_CONTEXT *context,
                          GX_RECTANGLE *block, INT xshift, INT yshift)
{
    d2_device *dave = _gx_dave2d_set_clip(context);

    int width = block->gx_rectangle_right - block->gx_rectangle_left + 1;
    int height = block->gx_rectangle_bottom - block->gx_rectangle_top + 1;

    CHECK_DAVE_STATUS(d2_utility_fbblitcopy(dave, (d2_u16)width, (d2_u16)height,
                      (d2_blitpos)(block->gx_rectangle_left),
                      (d2_blitpos)(block->gx_rectangle_top),
                      (d2_blitpos)(block->gx_rectangle_left + xshift),
                      (d2_blitpos)(block->gx_rectangle_top + yshift),
                      d2_bf_no_blitctxbackup))
}

/*****************************************************************************/
/* Draw an 8bpp alpha-map using Dave2D                                       */
VOID _gx_dave2d_alphamap_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap)
{
d2_u32  mode;

    mode = d2_mode_alpha8;

    if (pixelmap -> gx_pixelmap_flags & GX_PIXELMAP_COMPRESSED)
    {
        mode |= d2_mode_rle;
    }

    d2_device *dave = _gx_dave2d_set_clip(context);

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *)pixelmap -> gx_pixelmap_data,
                           pixelmap -> gx_pixelmap_width, pixelmap -> gx_pixelmap_width,
                           pixelmap -> gx_pixelmap_height, mode))

    mode = d2_bf_no_blitctxbackup;

    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(context->gx_draw_context_brush.gx_brush_fill_color)))
    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                         pixelmap -> gx_pixelmap_width,
                         pixelmap -> gx_pixelmap_height,
                         0, 0,
                         (d2_width)(D2_FIX4(pixelmap -> gx_pixelmap_width)),
                         (d2_width)(D2_FIX4(pixelmap -> gx_pixelmap_height)),
                         (d2_point)(D2_FIX4(xpos)), (d2_point)(D2_FIX4(ypos)), d2_bf_usealpha|d2_bf_colorize))
}

/*****************************************************************************/
/* Render 8bpp glyph using Dave2D                                            */
VOID _gx_dave2d_glyph_8bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph)
{
d2_device  *dave;
GX_COLOR    text_color;
d2_u32      mode = 0;

    text_color =  context -> gx_draw_context_brush.gx_brush_line_color;

    /* pickup pointer to current display driver */

    dave = context -> gx_draw_context_display -> gx_display_accelerator;
    if (glyph->gx_glyph_map_size & 0x8000)
    {
        mode |= d2_mode_rle;
    }

    CHECK_DAVE_STATUS(d2_cliprect(dave, draw_area -> gx_rectangle_left, draw_area -> gx_rectangle_top,
                draw_area -> gx_rectangle_right, draw_area -> gx_rectangle_bottom))

    CHECK_DAVE_STATUS(d2_setcolor(dave, 1, _gx_d2_color(text_color)))

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *)glyph -> gx_glyph_map,
                           glyph -> gx_glyph_width, glyph -> gx_glyph_width,
                           glyph -> gx_glyph_height,
                           mode|d2_mode_alpha8))

    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                         glyph -> gx_glyph_width,
                         glyph -> gx_glyph_height,
                         (d2_blitpos)(map_offset -> gx_point_x),
                         (d2_blitpos)(map_offset -> gx_point_y),
                         (d2_width)(D2_FIX4(glyph -> gx_glyph_width)),
                         (d2_width)(D2_FIX4(glyph -> gx_glyph_height)),
                         (d2_point)(D2_FIX4(draw_area -> gx_rectangle_left)),
                         (d2_point)(D2_FIX4(draw_area -> gx_rectangle_top)),
                         d2_bf_colorize2 | d2_bf_usealpha | d2_bf_filter))

}

/*****************************************************************************/
/* Render 4bpp glyph using Dave2D                                            */
VOID _gx_dave2d_glyph_4bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph)
{
d2_device  *dave;
GX_COLOR    text_color;
d2_u32      mode = 0;

    text_color =  context -> gx_draw_context_brush.gx_brush_line_color;

    /* pickup pointer to current display driver */
    if(glyph->gx_glyph_map_size & 0x8000)
    {
        mode |= d2_mode_rle;
    }
    dave = context -> gx_draw_context_display -> gx_display_accelerator;

    CHECK_DAVE_STATUS(d2_cliprect(dave, (d2_border)(draw_area -> gx_rectangle_left),
                                 (d2_border)(draw_area -> gx_rectangle_top),
                                 draw_area -> gx_rectangle_right, draw_area -> gx_rectangle_bottom))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 1, _gx_d2_color(text_color)))

    if (last_font_bits != 4)
    {
        CHECK_DAVE_STATUS(d2_settexclut_part(dave, (d2_color *) gray_palette, 0, 16))
        last_font_bits = 4;
    }

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *)glyph -> gx_glyph_map,
                           (glyph -> gx_glyph_width + 1) & 0xfffe,
                           glyph -> gx_glyph_width,
                           glyph -> gx_glyph_height,
                           mode|d2_mode_i4|d2_mode_clut))

    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                             glyph -> gx_glyph_width,
                             glyph -> gx_glyph_height,
                             0,0,
                             (d2_width)(D2_FIX4(glyph -> gx_glyph_width)),
                             (d2_width)(D2_FIX4(glyph -> gx_glyph_height)),
                             (d2_point)(D2_FIX4(draw_area -> gx_rectangle_left - map_offset -> gx_point_x)),
                             (d2_point)(D2_FIX4(draw_area -> gx_rectangle_top - map_offset -> gx_point_y)),
                             d2_bf_colorize2 | d2_bf_usealpha))
}

/*****************************************************************************/
VOID _gx_dave2d_glyph_1bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph)
{
d2_device  *dave;
GX_COLOR    text_color;
d2_u32      mode = 0;

    text_color =  context -> gx_draw_context_brush.gx_brush_line_color;

    /* pickup pointer to current display driver */
    dave = context -> gx_draw_context_display -> gx_display_accelerator;
    CHECK_DAVE_STATUS(d2_setcolor(dave, 1, _gx_d2_color(text_color)))

    CHECK_DAVE_STATUS(d2_cliprect(dave, (d2_border)(draw_area -> gx_rectangle_left),
                                 (d2_border)(draw_area -> gx_rectangle_top),
                                 draw_area -> gx_rectangle_right, draw_area -> gx_rectangle_bottom))

    if (last_font_bits != 1)
    {
        CHECK_DAVE_STATUS(d2_settexclut_part(dave, (d2_color *) mono_palette, 0, 2))
        last_font_bits = 1;
    }

    if(glyph->gx_glyph_map_size & 0x8000)
    {
        mode |= d2_mode_rle;
    }

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *)glyph -> gx_glyph_map,
                           (glyph -> gx_glyph_width + 7) & 0xfff8,
                           glyph -> gx_glyph_width,
                           glyph -> gx_glyph_height,
                           mode|d2_mode_i1|d2_mode_clut))

    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                         glyph -> gx_glyph_width,
                         glyph -> gx_glyph_height,
                         0,0,
                         (d2_width)(D2_FIX4(glyph -> gx_glyph_width)),
                         (d2_width)(D2_FIX4(glyph -> gx_glyph_height)),
                         (d2_point)(D2_FIX4(draw_area -> gx_rectangle_left - map_offset->gx_point_x)),
                         (d2_point)(D2_FIX4(draw_area -> gx_rectangle_top - map_offset->gx_point_y)),
                         d2_bf_colorize2 | d2_bf_usealpha))
}

#if defined(GX_ARC_DRAWING_SUPPORT)

/*****************************************************************************/
/* Render anti-aliased circle outline using Dave2D                           */
VOID _gx_dave2d_aliased_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r)
{
    GX_BRUSH *brush = &context->gx_draw_context_brush;

    // If brush width = 0, return.
    if(brush->gx_brush_width < 1)
    {
        return;
    }

    if(r < (UINT) ((brush->gx_brush_width + 1)/2))
    {
        r = 0;
    }
    else
    {
        r = (UINT)(r - (UINT) (brush->gx_brush_width + 1)/2);
    }

    context->gx_draw_context_clip->gx_rectangle_top = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_top - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_left = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_left - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_right = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_right + brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_bottom = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_bottom + brush->gx_brush_width);

    d2_device *dave = _gx_dave2d_set_clip(context);
    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 1))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_outline))
    CHECK_DAVE_STATUS(d2_outlinewidth(dave, (d2_width)(D2_FIX4(brush->gx_brush_width))))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(brush->gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_rendercircle(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_width)(D2_FIX4(r)), 0))
}

/*****************************************************************************/
/* Render non-aliased circle outline using Dave2D                            */
VOID _gx_dave2d_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r)
{
GX_BRUSH *brush = &context->gx_draw_context_brush;
    
    // If brush width = 0, return.
    if(brush->gx_brush_width < 1)
    {
        return;
    }

    if(r < (UINT) ((brush->gx_brush_width + 1)/2))
    {
        r = 0;
    }
    else
    {
        r = (UINT)(r - (UINT) (brush->gx_brush_width + 1)/2);
    }

    context->gx_draw_context_clip->gx_rectangle_top = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_top - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_left = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_left - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_right = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_right + brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_bottom = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_bottom + brush->gx_brush_width);

    d2_device *dave = _gx_dave2d_set_clip(context);
    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_outline))
    CHECK_DAVE_STATUS(d2_outlinewidth(dave, (d2_width)(D2_FIX4(brush->gx_brush_width))))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(brush->gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_rendercircle(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_width)(D2_FIX4(r)), 0))
}

/*****************************************************************************/
/* Render filled circle using Dave2D                                         */

VOID _gx_dave2d_circle_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r)
{
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    GX_COLOR brush_color = brush->gx_brush_fill_color;

    d2_device *dave = _gx_dave2d_set_clip(context);

    context->gx_draw_context_clip->gx_rectangle_top = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_top - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_left = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_left - brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_right = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_right + brush->gx_brush_width);
    context->gx_draw_context_clip->gx_rectangle_bottom = (GX_VALUE)(context->gx_draw_context_clip->gx_rectangle_bottom + brush->gx_brush_width);

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 1))
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
        CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(brush_color)))
    }
    CHECK_DAVE_STATUS(d2_rendercircle(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_width)(D2_FIX4(r)), 0))
}

/*****************************************************************************/
/* Render pie using Dave2D                                                   */
VOID _gx_dave2d_pie_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle)
{
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    INT sin1, cos1, sin2, cos2;
    d2_u32 flags;
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

    /*if (end_angle - start_angle > 180)*/
    if ((s_angle - e_angle> 180) || (s_angle - e_angle < 0 ))
    {
        flags = d2_wf_concave;
    }
    else
    {
        flags = 0;
    }

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 1))
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
        CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(brush->gx_brush_fill_color)))
    }
    CHECK_DAVE_STATUS(d2_renderwedge(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_width)(D2_FIX4(r)),
        0, cos1 << 8, sin1 << 8, cos2 << 8, sin2 << 8, flags))
}

/*****************************************************************************/
VOID _gx_dave2d_aliased_arc_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle)
{
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    INT sin1, cos1, sin2, cos2;
    d2_u32 flags;

    if(brush->gx_brush_width < 1)
    {
        return;
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
    INT brush_width = (brush->gx_brush_width + 1) >> 1;

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 1))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_outline))
    CHECK_DAVE_STATUS(d2_outlinewidth(dave, (d2_width)(D2_FIX4(brush_width))))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(brush->gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_renderwedge(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_width)(D2_FIX4(r)),
        0, cos1 << 8, sin1 << 8, cos2 << 8, sin2 << 8, flags))

}

/*****************************************************************************/
/* Arc drawing using Dave2D                                                  */
VOID _gx_dave2d_arc_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle)
{
    GX_BRUSH *brush = &context->gx_draw_context_brush;
    INT sin1, cos1, sin2, cos2;
    d2_u32 flags;

    if(brush->gx_brush_width < 1)
    {
        return;
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
    INT brush_width = (brush->gx_brush_width + 1) >> 1;

    CHECK_DAVE_STATUS(d2_setantialiasing(dave, 0))
    CHECK_DAVE_STATUS(d2_selectrendermode(dave, d2_rm_outline))
    CHECK_DAVE_STATUS(d2_outlinewidth(dave, (d2_width)(D2_FIX4(brush_width))))
    CHECK_DAVE_STATUS(d2_setcolor(dave, 0, _gx_d2_color(brush->gx_brush_line_color)))
    CHECK_DAVE_STATUS(d2_setfillmode(dave, d2_fm_color))
    CHECK_DAVE_STATUS(d2_renderwedge(dave, (d2_point)(D2_FIX4(xcenter)), (d2_point)(D2_FIX4(ycenter)), (d2_width)(D2_FIX4(r)),
        0, cos1 << 8, sin1 << 8, cos2 << 8, sin2 << 8, flags))
}

/*****************************************************************************/
VOID _gx_dave2d_arc_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle)
{
    /* dave2d doesn't support chord (filled arc), so use software fill */

    // the fill will use all Dave2D drawing, so no need to end/begin display list
    _gx_display_driver_generic_arc_fill(context, xcenter, ycenter, r, start_angle, end_angle);
}

/*****************************************************************************/
/* GUIX anti-aliased ellipse draw. Not supported by Dave2D so use standard   */
/* software rendering                                                        */
VOID _gx_dave2d_aliased_ellipse_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_generic_aliased_ellipse_draw(context, xcenter, ycenter, a, b);
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
/* GUIX non-aliased ellipse draw. Not supported by Dave2D so flush display   */
/* list and use standard software rendering                                  */
VOID _gx_dave2d_ellipse_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b)
{
    _gx_display_list_flush(context -> gx_draw_context_display);
    _gx_display_driver_generic_ellipse_draw(context, xcenter, ycenter, a, b);
    _gx_display_list_open(context -> gx_draw_context_display);
}

/*****************************************************************************/
/* GUIX ellipse fill. Not supported by Dave2D so flush display   */
/* list and use standard software rendering                                  */
VOID _gx_dave2d_ellipse_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b)
{
     // the fill will use all Dave2D drawing, so no need to end/begin display list
    _gx_display_driver_generic_ellipse_fill(context, xcenter, ycenter, a, b);
}

#endif  /* is GUIX arc drawing support enabled? */


/*****************************************************************************/
/* canvas copy, performed after buffer toggle operation                      */
void _gx_dave2d_copy_visible_to_working(GX_CANVAS *canvas, GX_RECTANGLE *copy)
{
    GX_RECTANGLE display_size;
    GX_RECTANGLE copy_clip;
    d2_u32 mode;

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

    // for copy region to align on 32-bit boundary

    if (display->gx_display_color_format == GX_COLOR_FORMAT_565RGB)
    {
        copy_clip.gx_rectangle_left = (GX_VALUE)(copy_clip.gx_rectangle_left & 0xfffe);
        copy_clip.gx_rectangle_right |= 1;
        mode = d2_mode_rgb565;
    }
    else
    {
        mode = d2_mode_argb8888;
    }

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

    CHECK_DAVE_STATUS(d2_framebuffer(dave, pPutRow,
                                     (d2_s32)(canvas -> gx_canvas_x_resolution), (d2_u32)(canvas -> gx_canvas_x_resolution),
                                     (d2_u32)(canvas -> gx_canvas_y_resolution), (d2_s32)mode))

    CHECK_DAVE_STATUS(d2_cliprect(dave,
                copy_clip.gx_rectangle_left,
                copy_clip.gx_rectangle_top,
                copy_clip.gx_rectangle_right,
                copy_clip.gx_rectangle_bottom))

    CHECK_DAVE_STATUS(d2_setblitsrc(dave, (void *) pGetRow,
                  canvas->gx_canvas_x_resolution,
                  canvas->gx_canvas_x_resolution,
                  canvas->gx_canvas_y_resolution,
                  mode))

    CHECK_DAVE_STATUS(d2_blitcopy(dave,
                copy_width, copy_height,
                (d2_blitpos)(copy_clip.gx_rectangle_left),
                (d2_blitpos)(copy_clip.gx_rectangle_top),
                (d2_width)(D2_FIX4(copy_width)),
                (d2_width)(D2_FIX4(copy_height)),
                (d2_point)(D2_FIX4(copy_clip.gx_rectangle_left)),
                (d2_point)(D2_FIX4(copy_clip.gx_rectangle_top)),
                d2_bf_no_blitctxbackup))

    CHECK_DAVE_STATUS(d2_endframe(display -> gx_display_accelerator))
    CHECK_DAVE_STATUS(d2_startframe(display -> gx_display_accelerator))
}

/* copy canvas memory into frame buffer, rotating                    */
void _gx_dave2d_rotate_canvas_to_working(GX_CANVAS *canvas, GX_RECTANGLE *copy, int rotation_angle)
{
    GX_RECTANGLE display_size;
    GX_RECTANGLE copy_clip;
    d2_u32 mode;

    USHORT   *pGetRow16;
    uint32_t *pGetRow32;

    int    stripe_count;
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
    if (display->gx_display_color_format == GX_COLOR_FORMAT_565RGB)
    {
        copy_clip.gx_rectangle_left  = (GX_VALUE)(copy_clip.gx_rectangle_left & 0xfffe);
        copy_clip.gx_rectangle_right |= 1;
        mode = d2_mode_rgb565;
    }
    else
    {
        mode = d2_mode_argb8888;
    }

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

    if (mode == d2_mode_rgb565)
    {
        pGetRow16 = (uint16_t *) canvas->gx_canvas_memory;
        pGetRow16 = pGetRow16 + ((canvas->gx_canvas_display_offset_y + copy_clip.gx_rectangle_top) * display->gx_display_width);
        pGetRow16 = pGetRow16 + (canvas->gx_canvas_display_offset_x + copy_clip.gx_rectangle_left);
    }
    else
    {
        pGetRow32 = (uint32_t *) canvas->gx_canvas_memory;
        pGetRow32 = pGetRow32 + ((canvas->gx_canvas_display_offset_y + copy_clip.gx_rectangle_top) * display->gx_display_width);
        pGetRow32 = pGetRow32 + (canvas->gx_canvas_display_offset_x + copy_clip.gx_rectangle_left);
    }

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
    if (mode == d2_mode_rgb565)
    {
        error += d2_settexture(dave, pGetRow16, display->gx_display_width, copy_width, copy_height, mode);
    }
    else
    {
        error += d2_settexture(dave, pGetRow32, display->gx_display_width, copy_width, copy_height, mode);
    }

    error += d2_settexturemode(dave, 0);
    error += d2_settextureoperation(dave, d2_to_one, d2_to_copy, d2_to_copy, d2_to_copy);
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
        stripe_count = 0;

        for(int x = xmin; x < (xmin + copy_width_rotated); x++)
        {
            error = d2_renderbox(dave,
                        (d2_point)D2_FIX4(x),
                        (d2_point)D2_FIX4(ymin),
                        (d2_width)D2_FIX4(1),
                        (d2_width)D2_FIX4(copy_height_rotated));

            stripe_count++;
            if (stripe_count > GX_SYNERGY_DRW_DL_COMMAND_COUNT_TO_REFRESH)
            {
                _gx_display_list_flush(display);
                _gx_display_list_open(display);
                stripe_count = 0;
            }
        }
    }

    d2_setfillmode(dave, fillmode_bkup);
}

/*****************************************************************************/
/* buffer toggle, done after sequence of canvas refresh drawing commands     */
void _gx_dave2d_buffer_toggle(GX_CANVAS *canvas, GX_RECTANGLE *dirty)
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
            _gx_dave2d_rotate_canvas_to_working(canvas, &Copy, rotation_angle);
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
            _gx_dave2d_copy_visible_to_working(canvas, &Copy);
        }
        else
        {
            _gx_dave2d_rotate_canvas_to_working(canvas, &Copy, rotation_angle);
        }
    }
}

#endif  /* GX_USE_SYNERGY_DRW */



/*****************************************************************************/
/* software buffer toggle operation, not using Dave2D, no rotation           */
void _gx_copy_visible_to_working(GX_CANVAS *canvas, GX_RECTANGLE *copy)
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
    gx_utility_rectangle_define(&display_size, 0, 0,
                                (GX_VALUE)(display->gx_display_width - 1),
                                (GX_VALUE)(display->gx_display_height - 1));
    copy_clip = *copy;

    /* for copy region to align on 32-bit boundry */
    if (display->gx_display_color_format == GX_COLOR_FORMAT_565RGB)
    {
        copy_clip.gx_rectangle_left  = (GX_VALUE)(copy_clip.gx_rectangle_left & 0xfffe);
        copy_clip.gx_rectangle_right |= 1;
    }

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

    pGetRow = (ULONG *) visible_frame;
    pPutRow  = (ULONG *) working_frame;

    if (display->gx_display_color_format == GX_COLOR_FORMAT_565RGB)
    {
        copy_width /= 2;
        canvas_stride = canvas->gx_canvas_x_resolution / 2;
        pPutRow += copy_clip.gx_rectangle_top * canvas_stride;
        pPutRow += copy_clip.gx_rectangle_left / 2;

        display_stride = display->gx_display_width / 2;
        pGetRow += (canvas->gx_canvas_display_offset_y + copy_clip.gx_rectangle_top) * (display_stride);
        pGetRow += (canvas->gx_canvas_display_offset_x + copy_clip.gx_rectangle_left) / 2;
    }
    else
    {
        canvas_stride = canvas->gx_canvas_x_resolution;
        pPutRow += copy_clip.gx_rectangle_top * canvas_stride;
        pPutRow += copy_clip.gx_rectangle_left;

        display_stride = display->gx_display_width;
        pGetRow += (canvas->gx_canvas_display_offset_y + copy_clip.gx_rectangle_top) * display_stride;
        pGetRow += (canvas->gx_canvas_display_offset_x + copy_clip.gx_rectangle_left);
    }

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
/* software buffer toggle operation, not using Dave2D, rotate, 16bpp         */
void _gx_rotate_canvas_to_working_16bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy, int angle)
{
    GX_RECTANGLE display_size;
    GX_RECTANGLE copy_clip;

    USHORT       *pGetRow;
    USHORT       *pPutRow;
    USHORT       *pGet;
    USHORT       *pPut;

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

    pGetRow = (USHORT *) canvas->gx_canvas_memory;
    pPutRow = (USHORT *) working_frame;

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
/* software buffer toggle operation, not using Dave2D, rotate, 16bpp         */
void _gx_rotate_canvas_to_working_32bpp(GX_CANVAS *canvas, GX_RECTANGLE *copy, int angle)
{
    GX_RECTANGLE display_size;
    GX_RECTANGLE copy_clip;

    ULONG       *pGetRow;
    ULONG       *pPutRow;
    ULONG       *pGet;
    ULONG       *pPut;

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

    pGetRow = (ULONG *) canvas->gx_canvas_memory;
    pPutRow = (ULONG *) working_frame;

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
/* buffer toggle, done after sequence of canvas refresh drawing commands     */
void _gx_synergy_buffer_toggle(GX_CANVAS *canvas, GX_RECTANGLE *dirty)
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
            if (display->gx_display_color_format == GX_COLOR_FORMAT_565RGB)
            {
                _gx_rotate_canvas_to_working_16bpp(canvas, &Copy, rotation_angle);
            }
            else
            {
                _gx_rotate_canvas_to_working_32bpp(canvas, &Copy, rotation_angle);
            }
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
            _gx_copy_visible_to_working(canvas, &Copy);
        }
        else
        {
            if (display->gx_display_color_format == GX_COLOR_FORMAT_565RGB)
            {
                _gx_rotate_canvas_to_working_16bpp(canvas, &Copy, rotation_angle);
            }
            else
            {
                _gx_rotate_canvas_to_working_32bpp(canvas, &Copy, rotation_angle);
            }
        }
    }
}

#if (GX_USE_SYNERGY_JPEG == 1)
#include "sf_jpeg_decode.h"
#include "r_jpeg_decode.h"

#define JPEG_ALIGNMENT_8  (0x07)
#define JPEG_ALIGNMENT_16 (0x0F)
#define JPEG_ALIGNMENT_32 (0x1F)

extern void* sf_el_jpeg_buffer_get(ULONG _display_handle, int *memory_size);

/*****************************************************************************/
/* hardware jpeg decode and draw                                             */
VOID _gx_synergy_jpeg_draw (GX_DRAW_CONTEXT *p_context, INT x, INT y, GX_PIXELMAP *p_pixelmap)
{
    GX_VALUE                  color_format = p_context->gx_draw_context_display->gx_display_color_format;
    UINT                      bytes_per_pixel;
    GX_RECTANGLE              clip_rect;
    GX_RECTANGLE              bound;
    GX_VIEW                  *view;
    UCHAR                    *output_buffer = GX_NULL;
    jpeg_decode_color_space_t pixel_format;
    INT                       ret;
    jpeg_decode_status_t      jpeg_decode_status = JPEG_DECODE_STATUS_FREE;
    GX_VALUE                  image_width;
    GX_VALUE                  image_height;

    GX_PIXELMAP               out_pixelmap;
    sf_jpeg_decode_instance_ctrl_t     sf_jpeg_decode_ctrl;
    jpeg_decode_instance_ctrl_t        jpeg_decode_ctrl;
    jpeg_decode_cfg_t         jpeg_decode_cfg;

    sf_jpeg_decode_api_t *    p_sf_jpeg = (sf_jpeg_decode_api_t *)&g_sf_jpeg_decode_on_sf_jpeg_decode;
    UINT                      minimum_height = 0;
    UINT                      memory_required = 0;
    UINT                      total_lines_decoded = 0;
    UINT                      remaining_lines = 0;
    UINT                      lines_decoded;
    int                       jpeg_buffer_size;

    /** Prepare Synergy Hardware JPEG driver. */
    memset(&sf_jpeg_decode_ctrl, 0x00, sizeof(sf_jpeg_decode_instance_ctrl_t));
    memset(&jpeg_decode_ctrl, 0x00, sizeof(jpeg_decode_instance_ctrl_t));

    jpeg_decode_cfg.alpha_value = 0xff;
    jpeg_decode_cfg.input_data_format  = JPEG_DECODE_DATA_FORMAT_NORMAL;
    jpeg_decode_cfg.output_data_format = JPEG_DECODE_DATA_FORMAT_NORMAL;
    if (GX_COLOR_FORMAT_32ARGB == color_format)
    {
        jpeg_decode_cfg.pixel_format = JPEG_DECODE_PIXEL_FORMAT_ARGB8888;
        bytes_per_pixel = 4;
    }
    else if (GX_COLOR_FORMAT_565RGB == color_format)
    {
        jpeg_decode_cfg.pixel_format = JPEG_DECODE_PIXEL_FORMAT_RGB565;
        bytes_per_pixel = 2;
    }
    else
    {
        /* This driver does not support the canvas color format.  Nothing to be done.
           Simply return. */
        return;
    }

    jpeg_decode_instance_t    jpeg_decode_instance = {
        .p_api  = &g_jpeg_decode_on_jpeg_decode,
        .p_ctrl = (jpeg_decode_ctrl_t *)&jpeg_decode_ctrl,
        .p_cfg  = &jpeg_decode_cfg
    };
    sf_jpeg_decode_cfg_t      sf_jpeg_decode_cfg = {
        .p_lower_lvl_jpeg_decode = &jpeg_decode_instance
    };

    /** Opens the JPEG driver.  */
    ret = p_sf_jpeg->open(&sf_jpeg_decode_ctrl, &sf_jpeg_decode_cfg);

    /** Sets the vertical and horizontal sample.  */
    ret += p_sf_jpeg->imageSubsampleSet(&sf_jpeg_decode_ctrl,
                          JPEG_DECODE_OUTPUT_NO_SUBSAMPLE, JPEG_DECODE_OUTPUT_NO_SUBSAMPLE);

    /** Sets the input buffer address.  */
    ret += p_sf_jpeg->inputBufferSet(&sf_jpeg_decode_ctrl,
                   (UCHAR *)p_pixelmap->gx_pixelmap_data, p_pixelmap->gx_pixelmap_data_size);

    /** Gets the JPEG hardware status.  */
    ret += p_sf_jpeg->wait(&sf_jpeg_decode_ctrl, &jpeg_decode_status, 1);

    if ((ret) || (!(JPEG_DECODE_STATUS_IMAGE_SIZE_READY & jpeg_decode_status)))
    {
        /* nothing to draw.  Close the device and return */
        p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
        return;
    }

    /** Gets the size of JPEG image.  */
    ret += p_sf_jpeg->imageSizeGet(&sf_jpeg_decode_ctrl, (uint16_t *)&image_width, (uint16_t *)&image_height);

    /** Gets the pixel format of JPEG image.  */
    ret += p_sf_jpeg->pixelFormatGet(&sf_jpeg_decode_ctrl, &pixel_format);

    if (ret)
    {
        return;
    }

    /** Calculate rectangle that bounds the JPEG */
    gx_utility_rectangle_define(&bound, (GX_VALUE)x, (GX_VALUE)y, 
                                (GX_VALUE)(x + image_width - 1), (GX_VALUE)(y + image_height - 1));

    /** Clip the line bounding box to the dirty rectangle */
    if (!gx_utility_rectangle_overlap_detect(&bound, &p_context->gx_draw_context_dirty, &bound))
    {
        /* nothing to draw.  Close the device and return */
        p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
        return;
    }

    view = p_context->gx_draw_context_view_head;

    /** Compute the decoding width and height based on pixel format. */
    /** TBO if the input JPEG size is not valid, drap it and return errors.  */
    switch (pixel_format)
    {
    case JPEG_DECODE_COLOR_SPACE_YCBCR444:
        /* 8 lines by 8 pixels. */
        if ((image_width & JPEG_ALIGNMENT_8) || (image_height & JPEG_ALIGNMENT_8))
        {
            p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
            return;
        }

        minimum_height = 8;
        break;

    case JPEG_DECODE_COLOR_SPACE_YCBCR422:
        /* 8 lines by 16 pixels. */
        if ((image_width & JPEG_ALIGNMENT_16) || (image_height & JPEG_ALIGNMENT_8))
        {
            p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
            return;
        }

        minimum_height = 8;
        break;

    case JPEG_DECODE_COLOR_SPACE_YCBCR411:
        /* 8 lines by 32 pixels. */
        if ((image_width & JPEG_ALIGNMENT_32) || (image_height & JPEG_ALIGNMENT_8))
        {
            p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
            return;
        }

        minimum_height = 8;
        break;

    case JPEG_DECODE_COLOR_SPACE_YCBCR420:
        /* 16 lines by 16 pixels. */
        if ((image_width & JPEG_ALIGNMENT_16) || (image_height & JPEG_ALIGNMENT_16))
        {
            p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
            return;
        }
        minimum_height = 16;
        break;

    default:
        p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
        return;
    }

    memory_required = (UINT)(minimum_height * ((UINT)image_width * bytes_per_pixel));

    output_buffer = sf_el_jpeg_buffer_get(p_context->gx_draw_context_display->gx_display_handle, &jpeg_buffer_size);

    /* Verify JPEG output buffer size meets minimum memory requirement. */
    if((UINT)jpeg_buffer_size < memory_required)
    {
        p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
        return;
    }

    /** Detects memory allocation errors. */
    if (output_buffer == GX_NULL)
    {
        /* If the system memory allocation function fails, nothing needs to be done but close the JPEG device and return .*/
        p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
        return;
    }

    /** Reject the buffer if it is not 8-byte aligned.*/
    if ((ULONG)output_buffer & 0x7)
    {
        /* Close the JPEG device and return. */
        p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
        return;
    }        

    /** Sets up the out_pixelmap structure. */
    out_pixelmap.gx_pixelmap_data          = (GX_UBYTE *)(output_buffer);
    out_pixelmap.gx_pixelmap_data_size     = (ULONG)jpeg_buffer_size;
    out_pixelmap.gx_pixelmap_format        = (GX_UBYTE)color_format;
    out_pixelmap.gx_pixelmap_height        = image_height;
    out_pixelmap.gx_pixelmap_width         = image_width;
    out_pixelmap.gx_pixelmap_version_major = p_pixelmap->gx_pixelmap_version_major;
    out_pixelmap.gx_pixelmap_version_minor = p_pixelmap->gx_pixelmap_version_minor;
    out_pixelmap.gx_pixelmap_flags         = 0;

    /** Sets the horizontal stride. */
    ret += (INT)(p_sf_jpeg->horizontalStrideSet(&sf_jpeg_decode_ctrl, (uint32_t)image_width));


    remaining_lines = (UINT)image_height;


    while(remaining_lines)
    {
        /* If running with Dave2D, make sure previous block drawing is completed before
         * we attempt to decode new jpeg block.
         */
        #if (GX_USE_SYNERGY_DRW == 1)
        CHECK_DAVE_STATUS(d2_endframe(p_context->gx_draw_context_display -> gx_display_accelerator))
        /* trigger execution of previous display list, switch to new display list */
        CHECK_DAVE_STATUS(d2_startframe(p_context->gx_draw_context_display -> gx_display_accelerator))
        #endif

        /** Assigns the output buffer to start the decoding process. */
        ret = p_sf_jpeg->outputBufferSet(&sf_jpeg_decode_ctrl,
                                         (VOID*)(INT)(out_pixelmap.gx_pixelmap_data), (UINT)jpeg_buffer_size);

        if(ret != SSP_SUCCESS)
        {
            /* If the system memory allocation function fails, nothing needs to be done but close the JPEG device and return .*/
            p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
            return;
        }

        /** Waits for the device to finish. */
        jpeg_decode_status = JPEG_DECODE_STATUS_FREE;
        while ((ret == 0) && (jpeg_decode_status & JPEG_DECODE_STATUS_OUTPUT_PAUSE) == 0)
        {
            ret = p_sf_jpeg->wait(&sf_jpeg_decode_ctrl, &jpeg_decode_status, 10);
            if(ret != 0)
            {
                return;
            }
            if(jpeg_decode_status & JPEG_DECODE_STATUS_DONE)break;
        }

        lines_decoded = 0;
        ret = p_sf_jpeg->linesDecodedGet(&sf_jpeg_decode_ctrl, (uint32_t *) &lines_decoded);

        if((ret != SSP_SUCCESS) || (lines_decoded == 0))
        {
            p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
            return;
        }

        if(remaining_lines > lines_decoded)
        {
            remaining_lines -= lines_decoded;
        }
        else
        {
            remaining_lines = 0;
        }

        out_pixelmap.gx_pixelmap_height = (GX_VALUE)lines_decoded;
        out_pixelmap.gx_pixelmap_data_size = (ULONG)(lines_decoded * ((UINT)image_width * bytes_per_pixel));

        view = p_context->gx_draw_context_view_head;

        while (view)
        {
            if (!gx_utility_rectangle_overlap_detect(&view->gx_view_rectangle, &bound, &clip_rect))
            {
                view = view->gx_view_next;
                continue;
            }

            p_context->gx_draw_context_clip = &clip_rect;
            /** Draws map.  */
            p_context->gx_draw_context_display->gx_display_driver_pixelmap_draw(p_context, x, y + (INT)total_lines_decoded, &out_pixelmap);

            /** Goes to the next view */
            view = view->gx_view_next;
        }
        total_lines_decoded += lines_decoded;
        
        #if (GX_USE_SYNERGY_DRW == 1)
        CHECK_DAVE_STATUS(d2_endframe(p_context->gx_draw_context_display -> gx_display_accelerator))
        /* trigger execution of previous display list, switch to new display list */
        CHECK_DAVE_STATUS(d2_startframe(p_context->gx_draw_context_display -> gx_display_accelerator))
        #endif
    }
    
    ret = (GX_VALUE)(ret + p_sf_jpeg->close(&sf_jpeg_decode_ctrl));

    /** Detects memory allocation errors. */
    if (ret != SSP_SUCCESS)
    {
        /* If the system memory allocation function fails, nothing needs to be done but close the JPEG device and return .*/
        p_sf_jpeg->close(&sf_jpeg_decode_ctrl);
        return;
    }
}  /* End of function _gx_driver_jpeg_draw() */

#endif /* (GX_USE_SYNERGY_JPEG)  */
