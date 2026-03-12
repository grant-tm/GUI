#ifndef GUI_WIDGETS_H
#define GUI_WIDGETS_H

#include "gui_core.h"

typedef struct GUIToggleStyle
{
    Vec4 background_color;
    Vec4 hot_color;
    Vec4 active_color;
    Vec4 checked_color;
    Vec4 border_color;
    Vec4 text_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    f32 text_size;
    f32 height;
    f32 box_size;
    f32 spacing;
} GUIToggleStyle;

typedef struct GUISliderStyle
{
    Vec4 track_color;
    Vec4 fill_color;
    Vec4 knob_color;
    Vec4 border_color;
    Vec4 text_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    f32 text_size;
    f32 height;
    f32 track_height;
    f32 knob_width;
    f32 spacing;
} GUISliderStyle;

GUIToggleStyle GUIToggleStyle_Default (void);
GUISliderStyle GUISliderStyle_Default (void);

void GUI_BeginPanel (GUIContext *context, GUIID id, Rect2 rect, const GUIPanelStyle *style);
void GUI_EndPanel (GUIContext *context);
void GUI_Label (GUIContext *context, String text, const GUILabelStyle *style);
b32 GUI_Button (GUIContext *context, GUIID id, String text, f32 width, const GUIButtonStyle *style);
b32 GUI_Toggle (GUIContext *context, GUIID id, String text, b32 *value, const GUIToggleStyle *style);
b32 GUI_SliderF32 (GUIContext *context, GUIID id, String label, f32 width, f32 min_value, f32 max_value, f32 *value, const GUISliderStyle *style);
b32 GUI_TextField (GUIContext *context, GUIID id, c8 *buffer, usize capacity, usize *length, String placeholder, const GUITextFieldStyle *style);

#endif // GUI_WIDGETS_H
