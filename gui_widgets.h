#ifndef GUI_WIDGETS_H
#define GUI_WIDGETS_H

#include "gui_core.h"

typedef struct GUIPanelStyle
{
    Vec4 background_color;
    Vec4 border_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    Vec2 padding;
    f32 spacing;
} GUIPanelStyle;

typedef struct GUILabelStyle
{
    Vec4 text_color;
    f32 text_size;
    f32 height;
} GUILabelStyle;

typedef struct GUIButtonStyle
{
    Vec4 background_color;
    Vec4 hot_color;
    Vec4 active_color;
    Vec4 border_color;
    Vec4 text_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    f32 text_size;
    f32 height;
    Vec2 padding;
} GUIButtonStyle;

GUIPanelStyle GUIPanelStyle_Default (void);
GUILabelStyle GUILabelStyle_Default (void);
GUIButtonStyle GUIButtonStyle_Default (void);

void GUI_BeginPanel (GUIContext *context, GUIID id, Rect2 rect, const GUIPanelStyle *style);
void GUI_EndPanel (GUIContext *context);
void GUI_Label (GUIContext *context, String text, const GUILabelStyle *style);
b32 GUI_Button (GUIContext *context, GUIID id, String text, f32 width, const GUIButtonStyle *style);

#endif // GUI_WIDGETS_H
