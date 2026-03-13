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

typedef struct GUIDragF32Style
{
    Vec4 background_color;
    Vec4 hot_color;
    Vec4 active_color;
    Vec4 border_color;
    Vec4 text_color;
    Vec4 accent_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    Vec2 padding;
    f32 text_size;
    f32 height;
    f32 drag_sensitivity;
    f32 step;
    i32 precision;
} GUIDragF32Style;

typedef struct GUIDragI32Style
{
    Vec4 background_color;
    Vec4 hot_color;
    Vec4 active_color;
    Vec4 border_color;
    Vec4 text_color;
    Vec4 accent_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    Vec2 padding;
    f32 text_size;
    f32 height;
    f32 drag_sensitivity;
    i32 step;
} GUIDragI32Style;

typedef struct GUISpinBoxStyle
{
    Vec4 background_color;
    Vec4 button_color;
    Vec4 button_hot_color;
    Vec4 button_active_color;
    Vec4 border_color;
    Vec4 text_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    Vec2 padding;
    f32 text_size;
    f32 height;
    f32 button_width;
    f32 step;
    i32 precision;
} GUISpinBoxStyle;

typedef struct GUISpinBoxI32Style
{
    Vec4 background_color;
    Vec4 button_color;
    Vec4 button_hot_color;
    Vec4 button_active_color;
    Vec4 border_color;
    Vec4 text_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    Vec2 padding;
    f32 text_size;
    f32 height;
    f32 button_width;
    i32 step;
} GUISpinBoxI32Style;

typedef struct GUICollapsibleSectionStyle
{
    Vec4 background_color;
    Vec4 hot_color;
    Vec4 active_color;
    Vec4 border_color;
    Vec4 text_color;
    Vec4 accent_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    Vec2 padding;
    f32 text_size;
    f32 height;
    f32 spacing;
    f32 indent;
} GUICollapsibleSectionStyle;

GUIToggleStyle GUIToggleStyle_Default (void);
GUISliderStyle GUISliderStyle_Default (void);
GUIScrollRegionStyle GUIScrollRegionStyle_Default (void);
GUIDragF32Style GUIDragF32Style_Default (void);
GUIDragI32Style GUIDragI32Style_Default (void);
GUISpinBoxStyle GUISpinBoxStyle_Default (void);
GUISpinBoxI32Style GUISpinBoxI32Style_Default (void);
GUICollapsibleSectionStyle GUICollapsibleSectionStyle_Default (void);

f32 GUI_MeasureLabelWidth (const GUIContext *context, String text, const GUILabelStyle *style);
f32 GUI_MeasureMaxLabelWidth (const GUIContext *context, const String *texts, usize text_count, const GUILabelStyle *style);
Vec2 GUI_MeasureButtonSize (const GUIContext *context, String text, const GUIButtonStyle *style);

void GUI_BeginPanel (GUIContext *context, GUIID id, Rect2 rect, const GUIPanelStyle *style);
void GUI_EndPanel (GUIContext *context);
void GUI_BeginScrollRegion (GUIContext *context, GUIID id, Rect2 rect, const GUIScrollRegionStyle *style);
void GUI_EndScrollRegion (GUIContext *context);
b32 GUI_BeginCollapsibleSection (GUIContext *context, GUIID id, String label, b32 default_expanded, const GUICollapsibleSectionStyle *style);
void GUI_EndCollapsibleSection (GUIContext *context);
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
b32 GUI_PropertyDragF32 (GUIContext *context, GUIID id, String label, f32 min_value, f32 max_value, f32 *value, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUIDragF32Style *drag_style);
b32 GUI_PropertyDragI32 (GUIContext *context, GUIID id, String label, i32 min_value, i32 max_value, i32 *value, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUIDragI32Style *drag_style);
b32 GUI_PropertySpinBoxF32 (GUIContext *context, GUIID id, String label, f32 min_value, f32 max_value, f32 *value, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUISpinBoxStyle *spin_box_style);
b32 GUI_PropertySpinBoxI32 (GUIContext *context, GUIID id, String label, i32 min_value, i32 max_value, i32 *value, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUISpinBoxI32Style *spin_box_style);
b32 GUI_Toggle (GUIContext *context, GUIID id, String text, b32 *value, const GUIToggleStyle *style);
b32 GUI_SliderF32 (GUIContext *context, GUIID id, String label, f32 width, f32 min_value, f32 max_value, f32 *value, const GUISliderStyle *style);
b32 GUI_DragF32 (GUIContext *context, GUIID id, f32 width, f32 min_value, f32 max_value, f32 *value, const GUIDragF32Style *style);
b32 GUI_DragI32 (GUIContext *context, GUIID id, f32 width, i32 min_value, i32 max_value, i32 *value, const GUIDragI32Style *style);
b32 GUI_SpinBoxF32 (GUIContext *context, GUIID id, f32 width, f32 min_value, f32 max_value, f32 *value, const GUISpinBoxStyle *style);
b32 GUI_SpinBoxI32 (GUIContext *context, GUIID id, f32 width, i32 min_value, i32 max_value, i32 *value, const GUISpinBoxI32Style *style);
b32 GUI_TextField (GUIContext *context, GUIID id, c8 *buffer, usize capacity, usize *length, String placeholder, const GUITextFieldStyle *style);

#endif // GUI_WIDGETS_H
