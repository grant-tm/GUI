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

typedef struct GUIScrollRegionStyle
{
    Vec4 background_color;
    Vec4 border_color;
    Vec4 scrollbar_track_color;
    Vec4 scrollbar_thumb_color;
    Vec4 scrollbar_thumb_hot_color;
    Vec4 scrollbar_thumb_active_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    Vec2 padding;
    f32 spacing;
    f32 scrollbar_width;
    f32 scrollbar_padding;
    f32 min_thumb_height;
} GUIScrollRegionStyle;

GUIToggleStyle GUIToggleStyle_Default (void);
GUISliderStyle GUISliderStyle_Default (void);
GUIScrollRegionStyle GUIScrollRegionStyle_Default (void);

f32 GUI_MeasureLabelWidth (const GUIContext *context, String text, const GUILabelStyle *style);
f32 GUI_MeasureMaxLabelWidth (const GUIContext *context, const String *texts, usize text_count, const GUILabelStyle *style);
Vec2 GUI_MeasureButtonSize (const GUIContext *context, String text, const GUIButtonStyle *style);

void GUI_BeginPanel (GUIContext *context, GUIID id, Rect2 rect, const GUIPanelStyle *style);
void GUI_EndPanel (GUIContext *context);
void GUI_BeginScrollRegion (GUIContext *context, GUIID id, Rect2 rect, const GUIScrollRegionStyle *style);
void GUI_EndScrollRegion (GUIContext *context);
void GUI_BeginFormRow (GUIContext *context, String label, f32 label_width, f32 height, f32 spacing, const GUILabelStyle *label_style);
void GUI_EndFormRow (GUIContext *context);
void GUI_BeginPropertyRow (GUIContext *context, String label, f32 label_width, f32 height, f32 spacing, const GUILabelStyle *label_style);
void GUI_EndPropertyRow (GUIContext *context);
void GUI_Label (GUIContext *context, String text, const GUILabelStyle *style);
b32 GUI_Button (GUIContext *context, GUIID id, String text, f32 width, const GUIButtonStyle *style);
b32 GUI_ButtonAuto (GUIContext *context, GUIID id, String text, const GUIButtonStyle *style);
b32 GUI_PropertyButton (GUIContext *context, GUIID id, String label, String text, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUIButtonStyle *button_style);
b32 GUI_PropertyToggle (GUIContext *context, GUIID id, String label, b32 *value, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUIToggleStyle *toggle_style);
b32 GUI_PropertySliderF32 (GUIContext *context, GUIID id, String label, f32 min_value, f32 max_value, f32 *value, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUISliderStyle *slider_style);
b32 GUI_PropertyTextField (GUIContext *context, GUIID id, String label, c8 *buffer, usize capacity, usize *length, String placeholder, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUITextFieldStyle *text_field_style);
b32 GUI_Toggle (GUIContext *context, GUIID id, String text, b32 *value, const GUIToggleStyle *style);
b32 GUI_SliderF32 (GUIContext *context, GUIID id, String label, f32 width, f32 min_value, f32 max_value, f32 *value, const GUISliderStyle *style);
b32 GUI_TextField (GUIContext *context, GUIID id, c8 *buffer, usize capacity, usize *length, String placeholder, const GUITextFieldStyle *style);

#endif // GUI_WIDGETS_H
