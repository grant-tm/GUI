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

static f32 GUI_ClampToRange (f32 value, f32 min_value, f32 max_value)
{
    return F32_Clamp(value, min_value, max_value);
}

static f32 GUI_RangeNormalize (f32 value, f32 min_value, f32 max_value)
{
    f32 range;

    range = max_value - min_value;
    if (range <= 0.0f)
    {
        return 0.0f;
    }

    return F32_Clamp((value - min_value) / range, 0.0f, 1.0f);
}

static f32 GUI_RangeLerp (f32 min_value, f32 max_value, f32 t)
{
    return F32_Lerp(min_value, max_value, F32_Clamp(t, 0.0f, 1.0f));
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

GUIToggleStyle GUIToggleStyle_Default (void)
{
    GUIToggleStyle style;

    style.background_color = GUIColor_Create(0.18f, 0.21f, 0.26f, 0.95f);
    style.hot_color = GUIColor_Create(0.25f, 0.29f, 0.35f, 1.0f);
    style.active_color = GUIColor_Create(0.14f, 0.17f, 0.22f, 1.0f);
    style.checked_color = GUIColor_Create(0.32f, 0.69f, 0.52f, 1.0f);
    style.border_color = GUIColor_Create(0.38f, 0.44f, 0.54f, 1.0f);
    style.text_color = GUIColor_Create(0.95f, 0.96f, 0.98f, 1.0f);
    style.corner_radii = GUICornerRadii_All(5.0f);
    style.border_thickness = GUIEdgeThickness_All(1.0f);
    style.text_size = 16.0f;
    style.height = 28.0f;
    style.box_size = 18.0f;
    style.spacing = 10.0f;
    return style;
}

GUISliderStyle GUISliderStyle_Default (void)
{
    GUISliderStyle style;

    style.track_color = GUIColor_Create(0.10f, 0.12f, 0.16f, 1.0f);
    style.fill_color = GUIColor_Create(0.24f, 0.68f, 0.98f, 1.0f);
    style.knob_color = GUIColor_Create(0.97f, 0.98f, 1.0f, 1.0f);
    style.border_color = GUIColor_Create(0.38f, 0.44f, 0.54f, 1.0f);
    style.text_color = GUIColor_Create(0.95f, 0.96f, 0.98f, 1.0f);
    style.corner_radii = GUICornerRadii_All(4.0f);
    style.border_thickness = GUIEdgeThickness_All(1.0f);
    style.text_size = 15.0f;
    style.height = 46.0f;
    style.track_height = 12.0f;
    style.knob_width = 16.0f;
    style.spacing = 8.0f;
    return style;
}

void GUI_BeginPanel (GUIContext *context, GUIID id, Rect2 rect, const GUIPanelStyle *style)
{
    GUILayoutScope *scope;
    const GUIPanelStyle *resolved_style;

    (void) id;

    ASSERT(context != NULL);
    ASSERT(context->layout_stack_count < context->layout_stack_capacity);

    resolved_style = (style != NULL) ? style : GUI_GetPanelStyle(context);
    GUI_DrawFilledRect(context, rect, resolved_style->background_color, resolved_style->corner_radii);
    GUI_DrawStrokedRect(context, rect, resolved_style->border_color, resolved_style->border_thickness, resolved_style->corner_radii);
    GUI_PushClipRect(context, rect);
    GUI_BeginLayout(context, GUIRect_Inset(rect, resolved_style->padding.x, resolved_style->padding.y), GUI_LAYOUT_AXIS_VERTICAL, resolved_style->spacing);

    scope = GUI_GetCurrentLayoutScope(context);
    scope->rect = rect;
    scope->content_rect = GUIRect_Inset(rect, resolved_style->padding.x, resolved_style->padding.y);
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
    const GUILabelStyle *resolved_style;

    ASSERT(context != NULL);
    resolved_style = (style != NULL) ? style : GUI_GetLabelStyle(context);

    rect = GUI_LayoutNextVerticalRect(context, resolved_style->height);
    position = Vec2_Create(rect.min.x, rect.min.y + (resolved_style->text_size * 0.75f));
    GUI_DrawText(context, position, text, resolved_style->text_color, resolved_style->text_size);
}

b32 GUI_Button (GUIContext *context, GUIID id, String text, f32 width, const GUIButtonStyle *style)
{
    Rect2 rect;
    Vec2 text_position;
    Vec4 color;
    const GUIButtonStyle *resolved_style;
    b32 is_hot;
    b32 was_pressed;
    b32 was_released;
    b32 clicked;

    ASSERT(context != NULL);
    resolved_style = (style != NULL) ? style : GUI_GetButtonStyle(context);

    rect = GUI_LayoutNextRect(context, Vec2_Create(width, resolved_style->height));
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

    color = resolved_style->background_color;
    if (context->active_id == id)
    {
        color = resolved_style->active_color;
    }
    else if (is_hot)
    {
        color = resolved_style->hot_color;
    }

    GUI_DrawFilledRect(context, rect, color, resolved_style->corner_radii);
    GUI_DrawStrokedRect(context, rect, resolved_style->border_color, resolved_style->border_thickness, resolved_style->corner_radii);
    text_position = GUI_GetTextPosition(rect, resolved_style->padding, resolved_style->text_size);
    GUI_DrawText(context, text_position, text, resolved_style->text_color, resolved_style->text_size);
    return clicked;
}

b32 GUI_Toggle (GUIContext *context, GUIID id, String text, b32 *value, const GUIToggleStyle *style)
{
    Rect2 rect;
    Rect2 box_rect;
    Vec2 text_position;
    const GUIToggleStyle *resolved_style;
    Vec4 box_color;
    GUIToggleStyle default_style;
    b32 is_hot;
    b32 was_pressed;
    b32 was_released;
    b32 changed;

    ASSERT(context != NULL);
    ASSERT(value != NULL);

    if (style == NULL)
    {
        default_style = GUIToggleStyle_Default();
        resolved_style = &default_style;
    }
    else
    {
        resolved_style = style;
    }

    rect = GUI_LayoutNextRect(context, Vec2_Create(0.0f, resolved_style->height));
    is_hot = GUI_IsHot(context, id, rect);
    was_pressed = context->input.mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_LEFT];
    was_released = context->input.mouse_buttons_released[PLATFORM_MOUSE_BUTTON_LEFT];
    changed = false;

    if (is_hot && was_pressed)
    {
        context->active_id = id;
    }

    if ((context->active_id == id) && was_released)
    {
        if (is_hot)
        {
            *value = !(*value);
            changed = true;
        }

        context->active_id = 0;
    }

    box_rect = Rect2_Create(
        rect.min.x,
        rect.min.y + ((rect.max.y - rect.min.y - resolved_style->box_size) * 0.5f),
        rect.min.x + resolved_style->box_size,
        rect.min.y + ((rect.max.y - rect.min.y - resolved_style->box_size) * 0.5f) + resolved_style->box_size
    );

    box_color = resolved_style->background_color;
    if (*value)
    {
        box_color = resolved_style->checked_color;
    }

    if (context->active_id == id)
    {
        box_color = resolved_style->active_color;
    }
    else if (is_hot)
    {
        box_color = resolved_style->hot_color;
    }

    GUI_DrawFilledRect(context, box_rect, box_color, resolved_style->corner_radii);
    GUI_DrawStrokedRect(context, box_rect, resolved_style->border_color, resolved_style->border_thickness, resolved_style->corner_radii);

    if (*value)
    {
        Rect2 check_rect;

        check_rect = GUIRect_Inset(box_rect, 4.0f, 4.0f);
        GUI_DrawFilledRect(context, check_rect, resolved_style->text_color, GUICornerRadii_All(2.0f));
    }

    text_position = Vec2_Create(
        box_rect.max.x + resolved_style->spacing,
        rect.min.y + ((rect.max.y - rect.min.y + resolved_style->text_size) * 0.5f) - 2.0f
    );
    GUI_DrawText(context, text_position, text, resolved_style->text_color, resolved_style->text_size);

    return changed;
}

b32 GUI_SliderF32 (GUIContext *context, GUIID id, String label, f32 width, f32 min_value, f32 max_value, f32 *value, const GUISliderStyle *style)
{
    Rect2 rect;
    Rect2 track_rect;
    Rect2 fill_rect;
    Rect2 knob_rect;
    Rect2 knob_accent_rect;
    Vec2 label_position;
    const GUISliderStyle *resolved_style;
    GUISliderStyle default_style;
    f32 clamped_value;
    f32 normalized_value;
    f32 track_min_x;
    f32 track_max_x;
    f32 knob_center_x;
    b32 is_hot;
    b32 changed;

    ASSERT(context != NULL);
    ASSERT(value != NULL);
    ASSERT(max_value > min_value);

    if (style == NULL)
    {
        default_style = GUISliderStyle_Default();
        resolved_style = &default_style;
    }
    else
    {
        resolved_style = style;
    }

    rect = GUI_LayoutNextRect(context, Vec2_Create(width, resolved_style->height));
    clamped_value = GUI_ClampToRange(*value, min_value, max_value);
    normalized_value = GUI_RangeNormalize(clamped_value, min_value, max_value);
    changed = false;

    label_position = Vec2_Create(rect.min.x, rect.min.y + (resolved_style->text_size * 0.75f));
    GUI_DrawText(context, label_position, label, resolved_style->text_color, resolved_style->text_size);

    track_rect = Rect2_Create(
        rect.min.x,
        rect.max.y - resolved_style->track_height,
        rect.max.x,
        rect.max.y
    );

    is_hot = GUI_IsHot(context, id, rect);
    if (is_hot && context->input.mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        context->active_id = id;
    }

    track_min_x = track_rect.min.x + (resolved_style->knob_width * 0.5f);
    track_max_x = track_rect.max.x - (resolved_style->knob_width * 0.5f);

    if ((context->active_id == id) && context->input.mouse_buttons[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        f32 t;
        f32 new_value;

        t = 0.0f;
        if (track_max_x > track_min_x)
        {
            t = (context->input.mouse_position.x - track_min_x) / (track_max_x - track_min_x);
        }

        new_value = GUI_RangeLerp(min_value, max_value, t);
        new_value = GUI_ClampToRange(new_value, min_value, max_value);
        if (new_value != clamped_value)
        {
            *value = new_value;
            clamped_value = new_value;
            normalized_value = GUI_RangeNormalize(clamped_value, min_value, max_value);
            changed = true;
        }
    }

    if ((context->active_id == id) && context->input.mouse_buttons_released[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        context->active_id = 0;
    }

    knob_center_x = F32_Lerp(track_min_x, track_max_x, normalized_value);
    fill_rect = track_rect;
    fill_rect.max.x = knob_center_x;
    knob_rect = Rect2_Create(
        knob_center_x - (resolved_style->knob_width * 0.5f),
        track_rect.min.y - 5.0f,
        knob_center_x + (resolved_style->knob_width * 0.5f),
        track_rect.max.y + 5.0f
    );
    knob_accent_rect = GUIRect_Inset(knob_rect, 4.0f, 4.0f);

    GUI_DrawFilledRect(context, track_rect, resolved_style->track_color, resolved_style->corner_radii);
    if (fill_rect.max.x > fill_rect.min.x)
    {
        GUI_DrawFilledRect(context, fill_rect, resolved_style->fill_color, resolved_style->corner_radii);
    }

    GUI_DrawStrokedRect(context, track_rect, resolved_style->border_color, resolved_style->border_thickness, resolved_style->corner_radii);
    GUI_DrawFilledRect(context, knob_rect, resolved_style->knob_color, resolved_style->corner_radii);
    GUI_DrawFilledRect(context, knob_accent_rect, resolved_style->fill_color, GUICornerRadii_All(2.0f));
    GUI_DrawStrokedRect(context, knob_rect, resolved_style->border_color, resolved_style->border_thickness, resolved_style->corner_radii);

    return changed;
}
