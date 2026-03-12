#include "gui_widgets.h"

static GUILayoutScope *GUI_GetCurrentLayoutScope (GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->layout_stack_count > 0);
    return context->layout_stack + (context->layout_stack_count - 1);
}

static b32 GUI_PointInRect (Vec2 point, Rect2 rect)
{
    return Rect2_ContainsPoint(rect, point);
}

static Rect2 GUI_LayoutNextVerticalRect (GUIContext *context, f32 height)
{
    return GUI_LayoutNextRect(context, Vec2_Create(0.0f, height));
}

static b32 GUI_IsHot (GUIContext *context, GUIID id, Rect2 rect)
{
    if (GUI_PointInRect(context->input.mouse_position, rect))
    {
        context->hot_id = id;
        return true;
    }

    return false;
}

static Vec2 GUI_GetTextPosition (Rect2 rect, Vec2 padding, f32 text_size)
{
    return Vec2_Create(rect.min.x + padding.x, rect.min.y + padding.y + (text_size * 0.75f));
}

GUIPanelStyle GUIPanelStyle_Default (void)
{
    GUIPanelStyle style;

    style.background_color = GUIColor_Create(0.12f, 0.13f, 0.15f, 1.0f);
    style.border_color = GUIColor_Create(0.20f, 0.23f, 0.28f, 1.0f);
    style.corner_radii = GUICornerRadii_All(6.0f);
    style.border_thickness = GUIEdgeThickness_All(1.0f);
    style.padding = Vec2_Create(12.0f, 12.0f);
    style.spacing = 8.0f;
    return style;
}

GUILabelStyle GUILabelStyle_Default (void)
{
    GUILabelStyle style;

    style.text_color = GUIColor_Create(0.92f, 0.93f, 0.95f, 1.0f);
    style.text_size = 16.0f;
    style.height = 20.0f;
    return style;
}

GUIButtonStyle GUIButtonStyle_Default (void)
{
    GUIButtonStyle style;

    style.background_color = GUIColor_Create(0.21f, 0.25f, 0.32f, 0.95f);
    style.hot_color = GUIColor_Create(0.28f, 0.34f, 0.42f, 1.0f);
    style.active_color = GUIColor_Create(0.16f, 0.19f, 0.25f, 1.0f);
    style.border_color = GUIColor_Create(0.38f, 0.44f, 0.54f, 1.0f);
    style.text_color = GUIColor_Create(0.97f, 0.98f, 1.0f, 1.0f);
    style.corner_radii = GUICornerRadii_All(6.0f);
    style.border_thickness = GUIEdgeThickness_All(1.0f);
    style.text_size = 16.0f;
    style.height = 32.0f;
    style.padding = Vec2_Create(10.0f, 7.0f);
    return style;
}

void GUI_BeginPanel (GUIContext *context, GUIID id, Rect2 rect, const GUIPanelStyle *style)
{
    GUILayoutScope *scope;
    GUIPanelStyle resolved_style;

    (void) id;

    ASSERT(context != NULL);
    ASSERT(style != NULL);
    ASSERT(context->layout_stack_count < context->layout_stack_capacity);

    resolved_style = *style;
    GUI_DrawFilledRect(context, rect, resolved_style.background_color, resolved_style.corner_radii);
    GUI_DrawStrokedRect(context, rect, resolved_style.border_color, resolved_style.border_thickness, resolved_style.corner_radii);
    GUI_PushClipRect(context, rect);
    GUI_BeginLayout(context, GUIRect_Inset(rect, resolved_style.padding.x, resolved_style.padding.y), GUI_LAYOUT_AXIS_VERTICAL, resolved_style.spacing);

    scope = GUI_GetCurrentLayoutScope(context);
    scope->rect = rect;
    scope->content_rect = GUIRect_Inset(rect, resolved_style.padding.x, resolved_style.padding.y);
}

void GUI_EndPanel (GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->layout_stack_count > 0);

    context->layout_stack_count -= 1;
    GUI_PopClipRect(context);
}

void GUI_Label (GUIContext *context, String text, const GUILabelStyle *style)
{
    Rect2 rect;
    Vec2 position;

    ASSERT(context != NULL);
    ASSERT(style != NULL);

    rect = GUI_LayoutNextVerticalRect(context, style->height);
    position = Vec2_Create(rect.min.x, rect.min.y + (style->text_size * 0.75f));
    GUI_DrawText(context, position, text, style->text_color, style->text_size);
}

b32 GUI_Button (GUIContext *context, GUIID id, String text, f32 width, const GUIButtonStyle *style)
{
    Rect2 rect;
    Vec2 text_position;
    Vec4 color;
    b32 is_hot;
    b32 was_pressed;
    b32 was_released;
    b32 clicked;

    ASSERT(context != NULL);
    ASSERT(style != NULL);

    rect = GUI_LayoutNextRect(context, Vec2_Create(width, style->height));
    is_hot = GUI_IsHot(context, id, rect);
    was_pressed = context->input.mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_LEFT];
    was_released = context->input.mouse_buttons_released[PLATFORM_MOUSE_BUTTON_LEFT];
    clicked = false;

    if (is_hot && was_pressed)
    {
        context->active_id = id;
    }

    if ((context->active_id == id) && was_released)
    {
        if (is_hot)
        {
            clicked = true;
        }

        context->active_id = 0;
    }

    color = style->background_color;
    if (context->active_id == id)
    {
        color = style->active_color;
    }
    else if (is_hot)
    {
        color = style->hot_color;
    }

    GUI_DrawFilledRect(context, rect, color, style->corner_radii);
    GUI_DrawStrokedRect(context, rect, style->border_color, style->border_thickness, style->corner_radii);
    text_position = GUI_GetTextPosition(rect, style->padding, style->text_size);
    GUI_DrawText(context, text_position, text, style->text_color, style->text_size);
    return clicked;
}
