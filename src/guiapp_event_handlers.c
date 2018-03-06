

#include "gui/guiapp_resources.h"
#include "gui/guiapp_specifications.h"

#include "main_thread.h"
#include "hal_data.h"

static bool button_enabled = false;

extern GX_WINDOW_ROOT * p_window_root;

extern uint32_t enable_led0;
extern uint32_t enable_led1;
extern uint32_t enable_led2;

static UINT show_window(GX_WINDOW * p_new, GX_WIDGET * p_widget, bool detach_old);
static void update_text_id(GX_WIDGET * p_widget, GX_RESOURCE_ID id, UINT string_id);
static void update_progressbar(GX_WIDGET * p_widget, GX_RESOURCE_ID id_pb, GX_RESOURCE_ID id_slider);
static void set_theme(uint32_t t);

UINT window1_handler(GX_WINDOW *widget, GX_EVENT *event_ptr)
{
    UINT result = gx_window_event_process(widget, event_ptr);

    switch (event_ptr->gx_event_type)
    {
    case GX_SIGNAL(ID_BUTTONENABLER, GX_EVENT_TOGGLE_ON):
        button_enabled = true;
        update_text_id(widget->gx_widget_parent, ID_WINDOWCHANGER, GX_STRING_ID_BUTTON_ENABLED);
        break;
    case GX_SIGNAL(ID_BUTTONENABLER, GX_EVENT_TOGGLE_OFF):
        button_enabled = false;
        update_text_id(widget->gx_widget_parent, ID_WINDOWCHANGER, GX_STRING_ID_BUTTON_DISABLED);
        break;
    case GX_SIGNAL(ID_WINDOWCHANGER, GX_EVENT_CLICKED):
        if(button_enabled){
            show_window((GX_WINDOW*)&window2, (GX_WIDGET*)widget, true);
        }
        break;
    case GX_SIGNAL(ID_LED1, GX_EVENT_TOGGLE_ON):
        update_text_id(widget->gx_widget_parent, ID_LED1, GX_STRING_ID_LED1_OFF);
        enable_led0=1;
        break;
    case GX_SIGNAL(ID_LED1, GX_EVENT_TOGGLE_OFF):
        update_text_id(widget->gx_widget_parent, ID_LED1, GX_STRING_ID_LED1_ON);
        enable_led0=0;
        break;
    case GX_SIGNAL(ID_LED2, GX_EVENT_TOGGLE_ON):
        update_text_id(widget->gx_widget_parent, ID_LED2, GX_STRING_ID_LED2_OFF);
        enable_led1=1;
        break;
    case GX_SIGNAL(ID_LED2, GX_EVENT_TOGGLE_OFF):
        update_text_id(widget->gx_widget_parent, ID_LED2, GX_STRING_ID_LED2_ON);
        enable_led1=0;
        break;
    case GX_SIGNAL(ID_LED3, GX_EVENT_TOGGLE_ON):
        update_text_id(widget->gx_widget_parent, ID_LED3, GX_STRING_ID_LED3_OFF);
        enable_led2=1;
        break;
    case GX_SIGNAL(ID_LED3, GX_EVENT_TOGGLE_OFF):
        update_text_id(widget->gx_widget_parent, ID_LED3, GX_STRING_ID_LED3_ON);
        enable_led2=0;
        break;
    default:
        gx_window_event_process(widget, event_ptr);
        break;
    }

    return result;
}

UINT window2_handler(GX_WINDOW *widget, GX_EVENT *event_ptr)
{
    UINT result = gx_window_event_process(widget, event_ptr);

    switch (event_ptr->gx_event_type){
        case GX_SIGNAL(ID_BACK, GX_EVENT_CLICKED):
            show_window((GX_WINDOW*)&window1, (GX_WIDGET*)widget, true);
            break;
        case GX_SIGNAL(ID_SLIDER, GX_EVENT_SLIDER_VALUE):
            update_progressbar(widget->gx_widget_parent, ID_PROGRESS, ID_SLIDER);
            break;
        case GX_SIGNAL(ID_RADIO1, GX_EVENT_RADIO_SELECT):
            set_theme(0);
            break;
        case GX_SIGNAL(ID_RADIO2, GX_EVENT_RADIO_SELECT):
            set_theme(1);
            break;
        default:
            result = gx_window_event_process(widget, event_ptr);
            break;
    }
    return result;
}

UINT splash_handler(GX_WINDOW *widget, GX_EVENT *event_ptr)
{
    UINT result = gx_window_event_process(widget, event_ptr);

    switch (event_ptr->gx_event_type){
        /* Don't know what GX_SIGNAL(0... means, it was discovered by debugging */
        case GX_SIGNAL(0, GX_EVENT_PEN_DOWN):
            show_window((GX_WINDOW*)&window1, (GX_WIDGET*)widget, true);
            break;
        default:
            result = gx_window_event_process(widget, event_ptr);
            break;
    }

    return result;
}

static UINT show_window(GX_WINDOW * p_new, GX_WIDGET * p_widget, bool detach_old)
{
    UINT err = GX_SUCCESS;

    if (!p_new->gx_widget_parent)
    {
        err = gx_widget_attach(p_window_root, p_new);
    }
    else
    {
        err = gx_widget_show(p_new);
    }

    gx_system_focus_claim(p_new);

    GX_WIDGET * p_old = p_widget;
    if (p_old && detach_old)
    {
        if (p_old != (GX_WIDGET*)p_new)
        {
            gx_widget_detach(p_old);
        }
    }

    return err;
}

static void update_text_id(GX_WIDGET * p_widget, GX_RESOURCE_ID id, UINT string_id)
{
    GX_PROMPT * p_prompt = NULL;

    ssp_err_t err = gx_widget_find(p_widget, id, GX_SEARCH_DEPTH_INFINITE, (GX_WIDGET**)&p_prompt);
    if (TX_SUCCESS == err)
    {
        gx_prompt_text_id_set(p_prompt, string_id);
    }
}

static void update_progressbar(GX_WIDGET * p_widget, GX_RESOURCE_ID id_pb, GX_RESOURCE_ID id_slider)
{
    GX_PROGRESS_BAR * p_progress_bar = NULL;
    GX_SLIDER * p_slider = NULL;

    ssp_err_t err = gx_widget_find(p_widget,
                                   id_pb,
                                   GX_SEARCH_DEPTH_INFINITE,
                                   (GX_WIDGET**)&p_progress_bar);
    if (TX_SUCCESS != err) return;

    err = gx_widget_find(p_widget,
                         id_slider,
                         GX_SEARCH_DEPTH_INFINITE,
                         (GX_WIDGET**)&p_slider);

    if (TX_SUCCESS != err) return;

    gx_progress_bar_value_set(
            p_progress_bar,
            p_slider->gx_slider_info.gx_slider_info_current_val);


    g_timer.p_api->periodSet(
            g_timer.p_ctrl,
            p_slider->gx_slider_info.gx_slider_info_current_val,
            TIMER_UNIT_FREQUENCY_HZ);
}

extern GX_STUDIO_DISPLAY_INFO guiapp_display_table[1];

static void set_theme(uint32_t t)
{
    GX_CONST GX_THEME *theme_ptr;

    GX_STUDIO_DISPLAY_INFO *display_info = &guiapp_display_table[0];

    /* install the request theme                                                   */

    theme_ptr = guiapp_display_table[0].theme_table[t];
    gx_display_color_table_set(display_info->display, theme_ptr->theme_color_table, theme_ptr->theme_color_table_size);

    /* install the color palette if required                                       */
    if (display_info->display->gx_display_driver_palette_set &&
        theme_ptr->theme_palette != NULL)
    {
        display_info->display->gx_display_driver_palette_set(display_info->display, theme_ptr->theme_palette, theme_ptr->theme_palette_size);
    }

    gx_display_font_table_set(display_info->display, theme_ptr->theme_font_table, theme_ptr->theme_font_table_size);
    gx_display_pixelmap_table_set(display_info->display, theme_ptr->theme_pixelmap_table, theme_ptr->theme_pixelmap_table_size);
    gx_system_scroll_appearance_set(theme_ptr->theme_vertical_scroll_style, (GX_SCROLLBAR_APPEARANCE *) &theme_ptr->theme_vertical_scrollbar_appearance);
    gx_system_scroll_appearance_set(theme_ptr->theme_horizontal_scroll_style, (GX_SCROLLBAR_APPEARANCE *) &theme_ptr->theme_horizontal_scrollbar_appearance);
    gx_system_language_table_set((GX_CHAR ***) display_info->language_table, display_info->language_table_size, display_info->string_table_size);
    gx_system_active_language_set(LANGUAGE_ENGLISH);
}

