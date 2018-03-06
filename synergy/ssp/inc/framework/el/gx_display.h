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


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** GUIX Component                                                        */
/**                                                                       */
/**   Display Management (Display)                                        */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/
/*                                                                        */
/*  COMPONENT DEFINITION                                   RELEASE        */
/*                                                                        */
/*    gx_display.h                                        PORTABLE C      */
/*                                                           5.3.2        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Express Logic, Inc.                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file defines the GUIX Display component, including all data    */
/*    types and external references.  It is assumed that gx_api.h and     */
/*    gx_port.h have already been included.                               */
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
/*                                            new APIs support,           */
/*                                            resulting in version 5.2.5  */
/*  09-21-2015     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.2.6  */
/*  02-22-2016     William E. Lamie         Modified comment(s), and      */
/*                                            added support for more      */
/*                                            low level draw routings,    */
/*                                            fixed compiler warnings,    */
/*                                            resulting in version 5.3    */
/*  04-05-2016     William E. Lamie         Modified comment(s), added    */
/*                                            new features and APIs,      */
/*                                            resulting in version 5.3.1  */
/*  06-15-2016     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 5.3.2  */
/*                                                                        */
/**************************************************************************/

#ifndef GX_DISPLAY_H
#define GX_DISPLAY_H


/* Define Display management constants.  */

#define GX_DISPLAY_ID ((ULONG)0x53435245)


/* Define display management function prototypes.  */

UINT _gx_display_create(GX_DISPLAY *display, GX_CONST GX_CHAR *name, UINT (*display_driver_setup)(GX_DISPLAY *), GX_VALUE width, GX_VALUE height);
UINT _gx_display_delete(GX_DISPLAY *display, VOID (*display_driver_cleanup)(GX_DISPLAY *));
VOID _gx_display_canvas_dirty(GX_DISPLAY *display);
UINT _gx_display_color_set(GX_DISPLAY *display, GX_RESOURCE_ID id, GX_COLOR color);
UINT _gx_display_color_table_set(GX_DISPLAY *display, GX_COLOR *color_table, INT number_of_colors);
UINT _gx_display_font_table_set(GX_DISPLAY *display, GX_FONT **font_table, UINT number_of_fonts);
UINT _gx_display_pixelmap_table_set(GX_DISPLAY *display, GX_PIXELMAP **pixelmap_table, UINT number_of_pixelmaps);

UINT _gxe_display_create(GX_DISPLAY *display, GX_CONST GX_CHAR *name, UINT (*display_driver_setup)(GX_DISPLAY *), GX_VALUE width, GX_VALUE height, UINT display_control_block_size);
UINT _gxe_display_color_set(GX_DISPLAY *display, GX_RESOURCE_ID resource_id, GX_COLOR new_color);
UINT _gxe_display_color_table_set(GX_DISPLAY *display, GX_COLOR *color_table, INT number_of_colors);
UINT _gxe_display_delete(GX_DISPLAY *display, VOID (*display_driver_cleanup)(GX_DISPLAY *));
UINT _gxe_display_font_table_set(GX_DISPLAY *display, GX_FONT **font_table, UINT number_of_fonts);
UINT _gxe_display_pixelmap_table_set(GX_DISPLAY *display, GX_PIXELMAP **pixelmap_table, UINT number_of_pixelmaps);
/* Generic driver level functions (not specific to color depth) */

VOID _gx_display_driver_generic_simple_wide_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID _gx_display_driver_generic_aliased_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID _gx_display_driver_generic_aliased_wide_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID _gx_display_driver_generic_filled_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, GX_FIXED_VAL r);
VOID _gx_display_driver_generic_aliased_filled_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, GX_FIXED_VAL r);
VOID _gx_display_driver_generic_glyph_8bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID _gx_display_driver_generic_glyph_4bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID _gx_display_driver_32bpp_glyph_1bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID _gx_display_driver_16bpp_glyph_1bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);


/* Define screen driver function prototypes.  */
VOID      _gx_display_driver_1bpp_canvas_blend(GX_CANVAS *source, GX_CANVAS *dest);
VOID      _gx_display_driver_1bpp_canvas_copy(GX_CANVAS *source, GX_CANVAS *dest);
VOID      _gx_display_driver_1bpp_horizontal_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos, INT width, GX_COLOR color);
VOID      _gx_display_driver_1bpp_horizontal_pattern_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos);
GX_COLOR  _gx_display_driver_1bpp_native_color_get(GX_DISPLAY *, GX_COLOR rawcolor);
VOID      _gx_display_driver_1bpp_pixel_write(GX_DRAW_CONTEXT *context, INT xcoord, INT ycoord, GX_COLOR color);
VOID      _gx_display_driver_1bpp_vertical_line_draw(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos, INT width, GX_COLOR color);
VOID      _gx_display_driver_1bpp_vertical_pattern_line_draw(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos);
VOID      _gx_display_driver_1bpp_pixelmap_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, GX_PIXELMAP *pmp);
VOID      _gx_display_driver_1bpp_block_move(GX_DRAW_CONTEXT *context, GX_RECTANGLE *src, INT xshift, INT yshift);
USHORT    _gx_display_driver_1bpp_row_pitch_get(USHORT width);
VOID      _gx_display_driver_1bpp_simple_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID      _gx_display_driver_1bpp_glyph_1bpp_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);

VOID      _gx_display_driver_332rgb_pixelmap_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap);
GX_COLOR  _gx_display_driver_332rgb_native_color_get(GX_DISPLAY *display, GX_COLOR rawcolor);
VOID      _gx_display_driver_8bpp_pixel_blend(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR fcolor, GX_UBYTE alpha);

VOID      _gx_display_driver_8bpp_canvas_copy(GX_CANVAS *source, GX_CANVAS *dest);
VOID      _gx_display_driver_8bpp_horizontal_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos, INT width, GX_COLOR color);
VOID      _gx_display_driver_8bpp_horizontal_pattern_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos);
VOID      _gx_display_driver_8bpp_horizontal_pixelmap_line_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos,
                                                                INT xstart, INT xend, INT y, GX_PIXELMAP *pixelmap);
GX_COLOR  _gx_display_driver_8bit_palette_native_color_get(GX_DISPLAY *, GX_COLOR rawcolor);
VOID      _gx_display_driver_8bpp_pixel_write(GX_DRAW_CONTEXT *context, INT xcoord, INT ycoord, GX_COLOR color);
VOID      _gx_display_driver_8bpp_vertical_line_draw(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos, INT width, GX_COLOR color);
VOID      _gx_display_driver_8bpp_pixelmap_blend(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, GX_PIXELMAP *pmp, GX_UBYTE alpha);
VOID      _gx_display_driver_8bpp_vertical_pattern_line_draw(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos);
VOID      _gx_display_driver_8bpp_pixelmap_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, GX_PIXELMAP *pmp);
VOID      _gx_display_driver_8bpp_pixelmap_rotate(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap, INT angle, INT rot_cx, INT rot_cy);
VOID      _gx_display_driver_8bpp_block_move(GX_DRAW_CONTEXT *context, GX_RECTANGLE *src, INT xshift, INT yshift);
USHORT    _gx_display_driver_8bpp_row_pitch_get(USHORT width);
VOID      _gx_display_driver_8bpp_simple_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);

VOID      _gx_display_driver_8bpp_glyph_1bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);
VOID      _gx_display_driver_8bpp_glyph_4bit_draw(GX_DRAW_CONTEXT *context, GX_RECTANGLE *draw_area, GX_POINT *map_offset, const GX_GLYPH *glyph);

VOID      _gx_display_driver_565rgb_canvas_blend(GX_CANVAS *source, GX_CANVAS *dest);
VOID      _gx_display_driver_16bpp_canvas_copy(GX_CANVAS *source, GX_CANVAS *dest);
VOID      _gx_display_driver_16bpp_horizontal_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos, INT width, GX_COLOR color);
VOID      _gx_display_driver_16bpp_horizontal_pattern_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos);
VOID      _gx_display_driver_16bpp_horizontal_pixelmap_line_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos,
                                                                 INT xstart, INT xend, INT y, GX_PIXELMAP *pixelmap);
VOID      _gx_display_driver_565rgb_pixel_blend(GX_DRAW_CONTEXT *context, INT xcoord, INT ycoord, GX_COLOR fcolor, GX_UBYTE alpha);
VOID      _gx_display_driver_16bpp_pixel_write(GX_DRAW_CONTEXT *context, INT xcoord, INT ycoord, GX_COLOR color);
VOID      _gx_display_driver_16bpp_vertical_line_draw(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos, INT width, GX_COLOR color);
VOID      _gx_display_driver_16bpp_vertical_pattern_line_draw(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos);
VOID      _gx_display_driver_16bpp_pixelmap_blend(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, GX_PIXELMAP *pmp, GX_UBYTE alpha);
VOID      _gx_display_driver_565rgb_pixelmap_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, GX_PIXELMAP *pmp);
#if defined(GX_SOFTWARE_DECODER_SUPPORT)
VOID      _gx_display_driver_565rgb_jpeg_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap);
VOID      _gx_display_driver_565rgb_png_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap);
#endif
VOID      _gx_display_driver_16bpp_block_move(GX_DRAW_CONTEXT *context, GX_RECTANGLE *src, INT xshift, INT yshift);
GX_COLOR  _gx_display_driver_565rgb_native_color_get(GX_DISPLAY *, GX_COLOR rawcolor);
USHORT    _gx_display_driver_16bpp_row_pitch_get(USHORT width);
VOID      _gx_display_driver_16bpp_simple_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);
VOID      _gx_display_driver_16bpp_pixelmap_rotate(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap,
                                                   INT angle, INT rot_cx, INT rot_cy);

VOID      _gx_display_driver_24xrgb_canvas_blend(GX_CANVAS *source, GX_CANVAS *dest);
VOID      _gx_display_driver_24xrgb_pixel_blend(GX_DRAW_CONTEXT *context, INT xcoord, INT ycoord, GX_COLOR fcolor, GX_UBYTE alpha);
VOID      _gx_display_driver_24xrgb_pixelmap_blend(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, GX_PIXELMAP *pmp, GX_UBYTE alpha);
VOID      _gx_display_driver_24xrgb_pixelmap_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, GX_PIXELMAP *pmp);
#if defined(GX_SOFTWARE_DECODER_SUPPORT)
VOID      _gx_display_driver_24xrgb_jpeg_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap);
VOID      _gx_display_driver_24xrgb_png_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap);
#endif
GX_COLOR  _gx_display_driver_24xrgb_native_color_get(GX_DISPLAY *, GX_COLOR rawcolor);

VOID      _gx_display_driver_32bpp_canvas_copy(GX_CANVAS *source, GX_CANVAS *dest);
VOID      _gx_display_driver_32bpp_horizontal_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos, INT width, GX_COLOR color);
VOID      _gx_display_driver_32bpp_horizontal_pattern_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT xend, INT ypos);
VOID      _gx_display_driver_32bpp_horizontal_pixelmap_line_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos,
                                                                 INT xstart, INT xend, INT y, GX_PIXELMAP *pixelmap);
VOID      _gx_display_driver_32bpp_pixel_write(GX_DRAW_CONTEXT *context, INT xcoord, INT ycoord, GX_COLOR color);
VOID      _gx_display_driver_32bpp_vertical_line_draw(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos, INT width, GX_COLOR color);
VOID      _gx_display_driver_32bpp_vertical_pattern_line_draw(GX_DRAW_CONTEXT *context, INT ystart, INT yend, INT xpos);
VOID      _gx_display_driver_32bpp_block_move(GX_DRAW_CONTEXT *context, GX_RECTANGLE *src, INT xshift, INT yshift);
USHORT    _gx_display_driver_32bpp_row_pitch_get(USHORT width);
VOID      _gx_display_driver_32bpp_simple_line_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, INT xend, INT yend);

VOID      _gx_display_driver_32argb_pixel_blend(GX_DRAW_CONTEXT *context, INT xcoord, INT ycoord, GX_COLOR fcolor, GX_UBYTE alpha);
VOID      _gx_display_driver_32argb_pixelmap_blend(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, GX_PIXELMAP *pmp, GX_UBYTE alpha);
VOID      _gx_display_driver_32argb_pixelmap_draw(GX_DRAW_CONTEXT *context, INT xstart, INT ystart, GX_PIXELMAP *pmp);
GX_COLOR  _gx_display_driver_32argb_native_color_get(GX_DISPLAY *, GX_COLOR rawcolor);
VOID      _gx_display_driver_32bpp_pixelmap_rotate(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap,
                                                   INT angle, INT rot_cx, INT rot_cy);
VOID      _gx_display_driver_4444argb_pixel_blend(GX_DRAW_CONTEXT *context, INT x, INT y, GX_COLOR fcolor, GX_UBYTE alpha);                                                   
VOID      _gx_display_driver_4444argb_horizontal_pixelmap_line_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos,
                                                                    INT xstart, INT xend, INT y, GX_PIXELMAP *pixelmap);
VOID      _gx_display_driver_4444argb_pixelmap_draw(GX_DRAW_CONTEXT *context,
                                                    INT xpos, INT ypos, GX_PIXELMAP *pixelmap);
GX_COLOR  _gx_display_driver_4444argb_native_color_get(GX_DISPLAY *display, GX_COLOR rawcolor);                                               
                                               
VOID      _gx_display_driver_generic_alphamap_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pmp);

GX_POINT *_gx_display_driver_generic_wide_line_points_calculate(GX_DRAW_CONTEXT *context, INT xStart, INT yStart,
                                                                INT xEnd, INT yEnd, INT brush_width, GX_BOOL outline);
VOID      _gx_display_driver_generic_wide_line_fill(GX_DRAW_CONTEXT *context, GX_POINT *pPoints);

GX_BOOL   _gx_display_driver_generic_polygon_vertex_offset(GX_POINT *vertex, INT num, GX_VALUE index, GX_POINT *inward_vertex,
                                                           GX_VALUE inward_off, GX_POINT *outward_vertex, GX_VALUE outward_off);
VOID      _gx_display_driver_generic_polygon_draw(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num);
VOID      _gx_display_driver_generic_polygon_fill(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num);
VOID      _gx_display_driver_generic_polygon_convex_fill(GX_DRAW_CONTEXT *context, GX_POINT *vertex, INT num);
#if defined(GX_ARC_DRAWING_SUPPORT)
VOID _gx_display_driver_generic_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_display_driver_generic_wide_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_display_driver_generic_aliased_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_display_driver_generic_aliased_wide_circle_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_display_driver_generic_circle_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r);
VOID _gx_display_driver_generic_pie_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_display_driver_generic_simple_pie_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);

VOID _gx_display_driver_generic_arc_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_display_driver_generic_wide_arc_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_display_driver_generic_aliased_arc_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_display_driver_generic_aliased_wide_arc_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_display_driver_generic_arc_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle);
VOID _gx_display_driver_arc_clipping_get(INT xcenter, INT ycenter, UINT r, INT start_angle, INT end_angle,
                                         GX_RECTANGLE *clip_1, GX_RECTANGLE *clip_2, GX_RECTANGLE *clip_3, GX_RECTANGLE *clip_4);
VOID _gx_display_driver_circle_point_get(INT xcenter, INT ycenter, UINT r, INT angle, GX_POINT *point);
#endif

VOID _gx_display_driver_generic_aliased_ellipse_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
VOID _gx_display_driver_generic_aliased_wide_ellipse_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
VOID _gx_display_driver_generic_ellipse_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
VOID _gx_display_driver_generic_wide_ellipse_draw(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
VOID _gx_display_driver_generic_ellipse_fill(GX_DRAW_CONTEXT *context, INT xcenter, INT ycenter, INT a, INT b);
VOID _gx_display_driver_565rgb_pixelmap_raw_write(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pixelmap);

VOID _gx_display_driver_32argb_setup(GX_DISPLAY *display, VOID *aux_data,
                                     VOID (*toggle_function)(struct GX_CANVAS_STRUCT *canvas,
                                                             GX_RECTANGLE *dirty_area));

VOID _gx_display_driver_24xrgb_setup(GX_DISPLAY *display, VOID *aux_data,
                                     VOID (*toggle_function)(struct GX_CANVAS_STRUCT *canvas,
                                                             GX_RECTANGLE *dirty_area));
VOID _gx_display_driver_4444argb_setup(GX_DISPLAY *display, VOID *aux_data,
                                       VOID (*toggle_function)(struct GX_CANVAS_STRUCT *canvas,
                                                               GX_RECTANGLE *dirty_area));
                                                             
VOID _gx_display_driver_565rgb_setup(GX_DISPLAY *display, VOID *aux_data,
                                     VOID (*toggle_function)(struct GX_CANVAS_STRUCT *canvas,
                                                             GX_RECTANGLE *dirty_area));

VOID _gx_display_driver_8bit_palette_setup(GX_DISPLAY *display, VOID *aux_data,
                                           VOID (*toggle_function)(struct GX_CANVAS_STRUCT *canvas,
                                                                   GX_RECTANGLE *dirty_area));

VOID _gx_display_driver_332rgb_setup(GX_DISPLAY *display, VOID *aux_data,
                                     VOID(*toggle_function)(struct GX_CANVAS_STRUCT *canvas,
                                                            GX_RECTANGLE *dirty_area));

VOID _gx_display_driver_monochrome_setup(GX_DISPLAY *display, VOID *aux_data,
                                         VOID (*toggle_function)(struct GX_CANVAS_STRUCT *canvas,
                                                                 GX_RECTANGLE *dirty_area));

VOID  _gx_display_driver_generic_alphamap_raw_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pmp);
VOID  _gx_display_driver_generic_alphamap_compressed_draw(GX_DRAW_CONTEXT *context, INT xpos, INT ypos, GX_PIXELMAP *pmp);


#define REDVAL_16BPP(_c)   (GX_UBYTE)(((_c) >> 11) & 0x1f)
#define GREENVAL_16BPP(_c) (GX_UBYTE)(((_c) >> 5) & 0x3f)
#define BLUEVAL_16BPP(_c)  (GX_UBYTE)(((_c)) & 0x1f)


/* Define macros for assembling a 16-bit r:g:b value from 3 components.  */

#define ASSEMBLECOLOR_16BPP(_r, _g, _b) \
    ((((_r) & 0x1f) << 11) |            \
     (((_g) & 0x3f) << 5) |             \
     (((_b) & 0x1f)))


#define REDVAL_24BPP(_c)   (GX_UBYTE)((_c) >> 16)
#define GREENVAL_24BPP(_c) (GX_UBYTE)((_c) >> 8)
#define BLUEVAL_24BPP(_c)  (GX_UBYTE)(_c)

/* Define macros for assembling a 24-bit r:g:b value from 3 components.  */
#define ASSEMBLECOLOR_24BPP(_r, _g, _b) \
    (((_r) << 16) |                     \
     ((_g) << 8) |                      \
     (_b))


#define REDVAL_32BPP(_c)   (GX_UBYTE)((_c) >> 16)
#define GREENVAL_32BPP(_c) (GX_UBYTE)((_c) >> 8)
#define BLUEVAL_32BPP(_c)  (GX_UBYTE)(_c)


/* Define macros for assembling a 32-bit r:g:b value from 3 components.  */

#define ASSEMBLECOLOR_32BPP(_r, _g, _b) \
    (((_r) << 16) |                     \
     ((_g) << 8) |                      \
     (_b))
#endif

