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

static Vec2 GUI_GetTextPositionCenteredY (Rect2 rect, f32 text_size)
{
    f32 text_top;

    text_top = rect.min.y + ((rect.max.y - rect.min.y - text_size) * 0.5f);
    return Vec2_Create(rect.min.x, text_top + (text_size * 0.75f));
}

static usize GUI_GetCaretIndexFromPosition (const GUIContext *context, Rect2 rect, Vec2 padding, String text, f32 text_size, f32 mouse_x)
{
    usize index;
    f32 left_x;
    f32 prefix_width;

    ASSERT(context != NULL);
    left_x = rect.min.x + padding.x;

    for (index = 0; index < text.count; index += 1)
    {
        f32 glyph_center_x;
        f32 glyph_width;

        prefix_width = GUI_MeasureTextWidth(context, String_Prefix(text, index), text_size);
        glyph_width = GUI_MeasureTextWidth(context, String_Slice(text, index, 1), text_size);
        glyph_center_x = left_x + prefix_width + (glyph_width * 0.5f);
        if (mouse_x < glyph_center_x)
        {
            return index;
        }
    }

    return text.count;
}

static b32 GUI_TextFieldInsertByte (c8 *buffer, usize capacity, usize *length, usize index, c8 value)
{
    ASSERT(buffer != NULL);
    ASSERT(length != NULL);
    ASSERT(index <= *length);

    if ((*length + 1) >= capacity)
    {
        return false;
    }

    if (index < *length)
    {
        Memory_Move(buffer + index + 1, buffer + index, *length - index);
    }

    buffer[index] = value;
    *length += 1;
    buffer[*length] = 0;
    return true;
}

static b32 GUI_TextFieldDeleteByte (c8 *buffer, usize *length, usize index)
{
    ASSERT(buffer != NULL);
    ASSERT(length != NULL);
    ASSERT(index < *length);

    if ((index + 1) < *length)
    {
        Memory_Move(buffer + index, buffer + index + 1, *length - index - 1);
    }

    *length -= 1;
    buffer[*length] = 0;
    return true;
}

static b32 GUI_TextFieldHasSelection (const GUIContext *context)
{
    ASSERT(context != NULL);
    return context->text_field_caret != context->text_field_selection_anchor;
}

static RangeUsize GUI_TextFieldGetSelectionRange (const GUIContext *context)
{
    ASSERT(context != NULL);
    return RangeUsize_Create(
        MIN(context->text_field_caret, context->text_field_selection_anchor),
        MAX(context->text_field_caret, context->text_field_selection_anchor)
    );
}

static void GUI_TextFieldSetCaretAndAnchor (GUIContext *context, usize index)
{
    ASSERT(context != NULL);
    context->text_field_caret = index;
    context->text_field_selection_anchor = index;
}

static b32 GUI_TextFieldDeleteRange (c8 *buffer, usize *length, RangeUsize range)
{
    usize remove_count;

    ASSERT(buffer != NULL);
    ASSERT(length != NULL);
    ASSERT(range.min <= range.max);
    ASSERT(range.max <= *length);

    remove_count = range.max - range.min;
    if (remove_count == 0)
    {
        return false;
    }

    if (range.max < *length)
    {
        Memory_Move(buffer + range.min, buffer + range.max, *length - range.max);
    }

    *length -= remove_count;
    buffer[*length] = 0;
    return true;
}

static String GUI_TextFieldGetSelectionText (c8 *buffer, const GUIContext *context)
{
    RangeUsize range;

    ASSERT(buffer != NULL);
    ASSERT(context != NULL);

    if (!GUI_TextFieldHasSelection(context))
    {
        return String_Create(NULL, 0);
    }

    range = GUI_TextFieldGetSelectionRange(context);
    return String_Create(buffer + range.min, RangeUsize_Count(range));
}

static b32 GUI_TextFieldDeleteSelection (GUIContext *context, c8 *buffer, usize *length)
{
    RangeUsize range;
    b32 changed;

    ASSERT(context != NULL);
    ASSERT(buffer != NULL);
    ASSERT(length != NULL);

    if (!GUI_TextFieldHasSelection(context))
    {
        return false;
    }

    range = GUI_TextFieldGetSelectionRange(context);
    changed = GUI_TextFieldDeleteRange(buffer, length, range);
    GUI_TextFieldSetCaretAndAnchor(context, range.min);
    return changed;
}

static GUIScrollRegionState *GUI_GetOrCreateScrollRegionState (GUIContext *context, GUIID id)
{
    usize index;
    GUIScrollRegionState *free_state;

    ASSERT(context != NULL);
    ASSERT(id != 0);

    free_state = NULL;
    for (index = 0; index < context->scroll_region_state_capacity; index += 1)
    {
        GUIScrollRegionState *state;

        state = context->scroll_region_states + index;
        if (state->is_used)
        {
            if (state->id == id)
            {
                return state;
            }
        }
        else if (free_state == NULL)
        {
            free_state = state;
        }
    }

    if (free_state != NULL)
    {
        Memory_ZeroStruct(free_state);
        free_state->is_used = true;
        free_state->id = id;
        context->scroll_region_state_count += 1;
    }

    return free_state;
}

static GUICollapsibleSectionState *GUI_GetOrCreateCollapsibleSectionState (GUIContext *context, GUIID id, b32 default_expanded)
{
    usize index;
    GUICollapsibleSectionState *free_state;

    ASSERT(context != NULL);
    ASSERT(id != 0);

    free_state = NULL;
    for (index = 0; index < context->collapsible_section_state_capacity; index += 1)
    {
        GUICollapsibleSectionState *state;

        state = context->collapsible_section_states + index;
        if (state->is_used)
        {
            if (state->id == id)
            {
                return state;
            }
        }
        else if (free_state == NULL)
        {
            free_state = state;
        }
    }

    if (free_state != NULL)
    {
        Memory_ZeroStruct(free_state);
        free_state->is_used = true;
        free_state->id = id;
        free_state->is_expanded = default_expanded;
        context->collapsible_section_state_count += 1;
    }

    return free_state;
}

static Rect2 GUI_ScrollRegionTrackRect (const GUIScrollRegionScope *scope)
{
    ASSERT(scope != NULL);

    return Rect2_Create(
        scope->viewport_rect.max.x - scope->scrollbar_width,
        scope->viewport_rect.min.y,
        scope->viewport_rect.max.x,
        scope->viewport_rect.max.y
    );
}

static Rect2 GUI_ScrollRegionThumbRect (const GUIScrollRegionScope *scope, f32 content_height, f32 viewport_height, f32 max_scroll)
{
    Rect2 track_rect;
    Rect2 thumb_rect;
    f32 thumb_height;
    f32 thumb_offset;
    f32 thumb_travel;

    ASSERT(scope != NULL);

    track_rect = GUI_ScrollRegionTrackRect(scope);
    thumb_height = MAX(scope->min_thumb_height, (viewport_height / content_height) * viewport_height);
    thumb_height = MIN(thumb_height, viewport_height);
    thumb_travel = MAX(0.0f, viewport_height - thumb_height);
    thumb_offset = (max_scroll > 0.0f) ? (scope->state->scroll_y / max_scroll) * thumb_travel : 0.0f;

    thumb_rect = Rect2_Create(
        track_rect.min.x,
        track_rect.min.y + thumb_offset,
        track_rect.max.x,
        track_rect.min.y + thumb_offset + thumb_height
    );

    return thumb_rect;
}

static f32 GUI_ScrollRegionScrollFromTrackPosition (const GUIScrollRegionScope *scope, Rect2 thumb_rect, f32 viewport_height, f32 max_scroll, f32 mouse_y)
{
    f32 thumb_height;
    f32 thumb_travel;
    f32 thumb_top;
    f32 normalized_scroll;

    ASSERT(scope != NULL);

    thumb_height = thumb_rect.max.y - thumb_rect.min.y;
    thumb_travel = MAX(0.0f, viewport_height - thumb_height);
    thumb_top = mouse_y - scope->state->drag_grab_offset_y - scope->viewport_rect.min.y;
    thumb_top = F32_Clamp(thumb_top, 0.0f, thumb_travel);
    normalized_scroll = (thumb_travel > 0.0f) ? (thumb_top / thumb_travel) : 0.0f;

    return normalized_scroll * max_scroll;
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

GUIScrollRegionStyle GUIScrollRegionStyle_Default (void)
{
    GUIScrollRegionStyle style;

    style.background_color = GUIColor_Create(0.09f, 0.11f, 0.14f, 0.96f);
    style.border_color = GUIColor_Create(0.20f, 0.24f, 0.30f, 1.0f);
    style.scrollbar_track_color = GUIColor_Create(0.07f, 0.09f, 0.12f, 1.0f);
    style.scrollbar_thumb_color = GUIColor_Create(0.34f, 0.40f, 0.50f, 1.0f);
    style.scrollbar_thumb_hot_color = GUIColor_Create(0.43f, 0.51f, 0.63f, 1.0f);
    style.scrollbar_thumb_active_color = GUIColor_Create(0.25f, 0.31f, 0.40f, 1.0f);
    style.corner_radii = GUICornerRadii_All(6.0f);
    style.border_thickness = GUIEdgeThickness_All(1.0f);
    style.padding = Vec2_Create(10.0f, 10.0f);
    style.spacing = 8.0f;
    style.scrollbar_width = 10.0f;
    style.scrollbar_padding = 6.0f;
    style.min_thumb_height = 28.0f;
    return style;
}

GUICollapsibleSectionStyle GUICollapsibleSectionStyle_Default (void)
{
    GUICollapsibleSectionStyle style;

    style.background_color = GUIColor_Create(0.16f, 0.18f, 0.22f, 0.98f);
    style.hot_color = GUIColor_Create(0.21f, 0.24f, 0.30f, 1.0f);
    style.active_color = GUIColor_Create(0.12f, 0.15f, 0.20f, 1.0f);
    style.border_color = GUIColor_Create(0.32f, 0.38f, 0.47f, 1.0f);
    style.text_color = GUIColor_Create(0.95f, 0.97f, 1.0f, 1.0f);
    style.accent_color = GUIColor_Create(0.24f, 0.68f, 0.98f, 1.0f);
    style.corner_radii = GUICornerRadii_All(6.0f);
    style.border_thickness = GUIEdgeThickness_All(1.0f);
    style.padding = Vec2_Create(10.0f, 8.0f);
    style.text_size = 16.0f;
    style.height = 34.0f;
    style.spacing = 8.0f;
    style.indent = 14.0f;
    return style;
}

f32 GUI_MeasureLabelWidth (const GUIContext *context, String text, const GUILabelStyle *style)
{
    const GUILabelStyle *resolved_style;

    ASSERT(context != NULL);
    resolved_style = (style != NULL) ? style : GUI_GetLabelStyle(context);

    return GUI_MeasureTextWidth(context, text, resolved_style->text_size);
}

f32 GUI_MeasureMaxLabelWidth (const GUIContext *context, const String *texts, usize text_count, const GUILabelStyle *style)
{
    usize text_index;
    f32 max_width;

    ASSERT(context != NULL);
    ASSERT((texts != NULL) || (text_count == 0));

    max_width = 0.0f;
    for (text_index = 0; text_index < text_count; text_index += 1)
    {
        max_width = MAX(max_width, GUI_MeasureLabelWidth(context, texts[text_index], style));
    }

    return max_width;
}

Vec2 GUI_MeasureButtonSize (const GUIContext *context, String text, const GUIButtonStyle *style)
{
    const GUIButtonStyle *resolved_style;
    f32 text_width;
    f32 width;

    ASSERT(context != NULL);
    resolved_style = (style != NULL) ? style : GUI_GetButtonStyle(context);

    text_width = GUI_MeasureTextWidth(context, text, resolved_style->text_size);
    width = resolved_style->padding.x + text_width + resolved_style->padding.x;
    return Vec2_Create(width, resolved_style->height);
}

void GUI_BeginFormRow (GUIContext *context, String label, f32 label_width, f32 height, f32 spacing, const GUILabelStyle *label_style)
{
    const GUILabelStyle *resolved_style;
    Rect2 label_rect;
    Vec2 label_position;
    f32 resolved_label_width;

    ASSERT(context != NULL);
    ASSERT(height > 0.0f);
    ASSERT(spacing >= 0.0f);

    resolved_style = (label_style != NULL) ? label_style : GUI_GetLabelStyle(context);
    resolved_label_width = (label_width > 0.0f) ? label_width : GUI_MeasureLabelWidth(context, label, resolved_style);

    GUI_BeginRow(context, height, spacing);
    label_rect = GUI_LayoutNextRect(context, Vec2_Create(resolved_label_width, 0.0f));
    label_position = GUI_GetTextPositionCenteredY(label_rect, resolved_style->text_size);
    GUI_DrawText(context, label_position, label, resolved_style->text_color, resolved_style->text_size);
}

void GUI_EndFormRow (GUIContext *context)
{
    ASSERT(context != NULL);
    GUI_EndRow(context);
}

void GUI_BeginPropertyRow (GUIContext *context, String label, f32 label_width, f32 height, f32 spacing, const GUILabelStyle *label_style)
{
    GUI_BeginFormRow(context, label, label_width, height, spacing, label_style);
}

void GUI_EndPropertyRow (GUIContext *context)
{
    GUI_EndFormRow(context);
}

GUITextFieldStyle GUITextFieldStyle_Default (void)
{
    GUITextFieldStyle style;

    style.background_color = GUIColor_Create(0.11f, 0.13f, 0.17f, 1.0f);
    style.hot_color = GUIColor_Create(0.14f, 0.17f, 0.22f, 1.0f);
    style.focused_color = GUIColor_Create(0.13f, 0.18f, 0.25f, 1.0f);
    style.border_color = GUIColor_Create(0.38f, 0.44f, 0.54f, 1.0f);
    style.text_color = GUIColor_Create(0.97f, 0.98f, 1.0f, 1.0f);
    style.placeholder_color = GUIColor_Create(0.54f, 0.58f, 0.66f, 1.0f);
    style.caret_color = GUIColor_Create(0.87f, 0.92f, 1.0f, 1.0f);
    style.selection_color = GUIColor_Create(0.22f, 0.47f, 0.82f, 0.85f);
    style.corner_radii = GUICornerRadii_All(6.0f);
    style.border_thickness = GUIEdgeThickness_All(1.0f);
    style.text_size = 16.0f;
    style.height = 34.0f;
    style.padding = Vec2_Create(10.0f, 8.0f);
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

void GUI_BeginScrollRegion (GUIContext *context, GUIID id, Rect2 rect, const GUIScrollRegionStyle *style)
{
    GUIScrollRegionStyle default_style;
    const GUIScrollRegionStyle *resolved_style;
    GUIScrollRegionState *state;
    GUIScrollRegionScope *scope;
    GUIScrollRegionScope interaction_scope;
    Rect2 viewport_rect;
    Rect2 content_rect;
    Rect2 layout_rect;
    Rect2 track_rect;
    Rect2 thumb_rect;
    b32 is_region_hovered;
    b32 is_track_hovered;
    b32 is_thumb_hovered;
    b32 can_keyboard_scroll;
    f32 scroll_step;
    f32 viewport_height;
    f32 max_scroll;
    f32 scroll_delta;

    ASSERT(context != NULL);
    ASSERT(id != 0);
    ASSERT(context->scroll_region_stack_count < context->scroll_region_stack_capacity);

    if (style == NULL)
    {
        default_style = GUIScrollRegionStyle_Default();
        resolved_style = &default_style;
    }
    else
    {
        resolved_style = style;
    }

    state = GUI_GetOrCreateScrollRegionState(context, id);
    ASSERT(state != NULL);

    GUI_DrawFilledRect(context, rect, resolved_style->background_color, resolved_style->corner_radii);
    GUI_DrawStrokedRect(context, rect, resolved_style->border_color, resolved_style->border_thickness, resolved_style->corner_radii);

    viewport_rect = GUIRect_Inset(rect, resolved_style->padding.x, resolved_style->padding.y);
    viewport_height = viewport_rect.max.y - viewport_rect.min.y;
    max_scroll = MAX(0.0f, state->content_height - viewport_height);
    scroll_delta = 0.0f;
    is_region_hovered = GUI_PointInRect(context->input.mouse_position, rect);

    interaction_scope.viewport_rect = viewport_rect;
    interaction_scope.scrollbar_width = resolved_style->scrollbar_width;
    interaction_scope.min_thumb_height = resolved_style->min_thumb_height;
    interaction_scope.state = state;

    if (is_region_hovered && context->input.mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        context->focused_id = id;
    }

    if (is_region_hovered && (context->input.mouse_wheel_delta != 0))
    {
        scroll_step = ((f32) context->input.mouse_wheel_delta / 120.0f) * 36.0f;
        state->scroll_y = state->scroll_y - scroll_step;
    }

    if ((max_scroll > 0.0f) && (resolved_style->scrollbar_width > 0.0f))
    {
        track_rect = GUI_ScrollRegionTrackRect(&interaction_scope);
        thumb_rect = GUI_ScrollRegionThumbRect(&interaction_scope, state->content_height, viewport_height, max_scroll);
        is_track_hovered = GUI_PointInRect(context->input.mouse_position, track_rect);
        is_thumb_hovered = GUI_PointInRect(context->input.mouse_position, thumb_rect);

        if (is_track_hovered && context->input.mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_LEFT])
        {
            context->active_id = id;
            context->focused_id = id;

            if (is_thumb_hovered)
            {
                state->drag_grab_offset_y = context->input.mouse_position.y - thumb_rect.min.y;
            }
            else
            {
                state->drag_grab_offset_y = (thumb_rect.max.y - thumb_rect.min.y) * 0.5f;
                state->scroll_y = GUI_ScrollRegionScrollFromTrackPosition(
                    &interaction_scope,
                    thumb_rect,
                    viewport_height,
                    max_scroll,
                    context->input.mouse_position.y
                );
            }
        }

        if ((context->active_id == id) && context->input.mouse_buttons[PLATFORM_MOUSE_BUTTON_LEFT])
        {
            state->scroll_y = GUI_ScrollRegionScrollFromTrackPosition(
                &interaction_scope,
                thumb_rect,
                viewport_height,
                max_scroll,
                context->input.mouse_position.y
            );
        }
    }

    if ((context->active_id == id) && context->input.mouse_buttons_released[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        context->active_id = 0;
    }

    can_keyboard_scroll = (context->focused_id == id) || (is_region_hovered && (context->focused_id == 0));

    if (can_keyboard_scroll)
    {
        if (context->input.keys_pressed[PLATFORM_KEY_UP])
        {
            scroll_delta -= 36.0f;
        }

        if (context->input.keys_pressed[PLATFORM_KEY_DOWN])
        {
            scroll_delta += 36.0f;
        }

        if (context->input.keys_pressed[PLATFORM_KEY_PAGE_UP])
        {
            scroll_delta -= viewport_height * 0.9f;
        }

        if (context->input.keys_pressed[PLATFORM_KEY_PAGE_DOWN])
        {
            scroll_delta += viewport_height * 0.9f;
        }

        if (context->input.keys_pressed[PLATFORM_KEY_HOME])
        {
            state->scroll_y = 0.0f;
        }

        if (context->input.keys_pressed[PLATFORM_KEY_END])
        {
            state->scroll_y = max_scroll;
        }
    }

    if (scroll_delta != 0.0f)
    {
        state->scroll_y += scroll_delta;
    }

    state->scroll_y = F32_Clamp(state->scroll_y, 0.0f, max_scroll);

    content_rect = viewport_rect;
    if (resolved_style->scrollbar_width > 0.0f)
    {
        content_rect.max.x -= resolved_style->scrollbar_width + resolved_style->scrollbar_padding;
    }

    layout_rect = content_rect;
    layout_rect.min.y -= state->scroll_y;
    layout_rect.max.y -= state->scroll_y;

    GUI_PushClipRect(context, viewport_rect);
    GUI_BeginLayout(context, layout_rect, GUI_LAYOUT_AXIS_VERTICAL, resolved_style->spacing);

    scope = context->scroll_region_stack + context->scroll_region_stack_count;
    context->scroll_region_stack_count += 1;
    scope->rect = rect;
    scope->viewport_rect = viewport_rect;
    scope->content_rect = content_rect;
    scope->scrollbar_track_color = resolved_style->scrollbar_track_color;
    scope->scrollbar_thumb_color = resolved_style->scrollbar_thumb_color;
    scope->scrollbar_thumb_hot_color = resolved_style->scrollbar_thumb_hot_color;
    scope->scrollbar_thumb_active_color = resolved_style->scrollbar_thumb_active_color;
    scope->scrollbar_width = resolved_style->scrollbar_width;
    scope->scrollbar_padding = resolved_style->scrollbar_padding;
    scope->min_thumb_height = resolved_style->min_thumb_height;
    scope->state = state;
}

void GUI_EndScrollRegion (GUIContext *context)
{
    GUILayoutScope *layout_scope;
    GUIScrollRegionScope *scope;
    Rect2 track_rect;
    Rect2 thumb_rect;
    f32 content_end_y;
    f32 content_height;
    f32 viewport_height;
    f32 max_scroll;

    ASSERT(context != NULL);
    ASSERT(context->scroll_region_stack_count > 0);
    ASSERT(context->layout_stack_count > 0);

    layout_scope = GUI_GetCurrentLayoutScope(context);
    scope = context->scroll_region_stack + (context->scroll_region_stack_count - 1);

    content_end_y = MAX(layout_scope->cursor.y - layout_scope->spacing, layout_scope->rect.min.y);
    content_height = MAX(0.0f, content_end_y - layout_scope->rect.min.y);
    viewport_height = scope->viewport_rect.max.y - scope->viewport_rect.min.y;
    max_scroll = MAX(0.0f, content_height - viewport_height);
    scope->state->scroll_y = F32_Clamp(scope->state->scroll_y, 0.0f, max_scroll);
    scope->state->content_height = content_height;

    GUI_EndLayout(context);
    GUI_PopClipRect(context);

    if ((max_scroll > 0.0f) && (scope->scrollbar_width > 0.0f))
    {
        Vec4 thumb_color;

        track_rect = GUI_ScrollRegionTrackRect(scope);
        thumb_rect = GUI_ScrollRegionThumbRect(scope, content_height, viewport_height, max_scroll);
        thumb_color = scope->scrollbar_thumb_color;

        if ((context->active_id == scope->state->id) && context->input.mouse_buttons[PLATFORM_MOUSE_BUTTON_LEFT])
        {
            thumb_color = scope->scrollbar_thumb_active_color;
        }
        else if (GUI_PointInRect(context->input.mouse_position, thumb_rect))
        {
            thumb_color = scope->scrollbar_thumb_hot_color;
        }

        GUI_DrawFilledRect(context, track_rect, scope->scrollbar_track_color, GUICornerRadii_All(scope->scrollbar_width * 0.5f));
        GUI_DrawFilledRect(context, thumb_rect, thumb_color, GUICornerRadii_All(scope->scrollbar_width * 0.5f));
    }

    context->scroll_region_stack_count -= 1;
}

b32 GUI_BeginCollapsibleSection (GUIContext *context, GUIID id, String label, b32 default_expanded, const GUICollapsibleSectionStyle *style)
{
    GUICollapsibleSectionStyle default_style;
    const GUICollapsibleSectionStyle *resolved_style;
    GUICollapsibleSectionState *state;
    Rect2 header_rect;
    Rect2 accent_rect;
    Rect2 content_rect;
    Vec2 text_position;
    Vec4 header_color;
    c8 indicator_buffer[2];
    String indicator_text;
    b32 is_hot;
    b32 was_pressed;
    b32 was_released;

    ASSERT(context != NULL);
    ASSERT(id != 0);
    ASSERT(context->collapsible_section_stack_count < context->collapsible_section_stack_capacity);

    if (style == NULL)
    {
        default_style = GUICollapsibleSectionStyle_Default();
        resolved_style = &default_style;
    }
    else
    {
        resolved_style = style;
    }

    state = GUI_GetOrCreateCollapsibleSectionState(context, id, default_expanded);
    ASSERT(state != NULL);

    header_rect = GUI_LayoutNextRect(context, Vec2_Create(0.0f, resolved_style->height));
    is_hot = GUI_IsHot(context, id, header_rect);
    was_pressed = context->input.mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_LEFT];
    was_released = context->input.mouse_buttons_released[PLATFORM_MOUSE_BUTTON_LEFT];

    if (is_hot && was_pressed)
    {
        context->active_id = id;
    }

    if ((context->active_id == id) && was_released)
    {
        if (is_hot)
        {
            state->is_expanded = !state->is_expanded;
        }

        context->active_id = 0;
    }

    header_color = resolved_style->background_color;
    if (context->active_id == id)
    {
        header_color = resolved_style->active_color;
    }
    else if (is_hot)
    {
        header_color = resolved_style->hot_color;
    }

    GUI_DrawFilledRect(context, header_rect, header_color, resolved_style->corner_radii);
    GUI_DrawStrokedRect(context, header_rect, resolved_style->border_color, resolved_style->border_thickness, resolved_style->corner_radii);

    accent_rect = Rect2_Create(
        header_rect.min.x,
        header_rect.min.y,
        MIN(header_rect.max.x, header_rect.min.x + 4.0f),
        header_rect.max.y
    );
    GUI_DrawFilledRect(context, accent_rect, resolved_style->accent_color, GUICornerRadii_Create(
        resolved_style->corner_radii.top_left,
        0.0f,
        0.0f,
        resolved_style->corner_radii.bottom_left
    ));

    indicator_buffer[0] = state->is_expanded ? '-' : '+';
    indicator_buffer[1] = 0;
    indicator_text = String_Create(indicator_buffer, 1);

    text_position = GUI_GetTextPosition(header_rect, resolved_style->padding, resolved_style->text_size);
    GUI_DrawText(context, text_position, indicator_text, resolved_style->accent_color, resolved_style->text_size);
    GUI_DrawText(
        context,
        Vec2_Create(text_position.x + 18.0f, text_position.y),
        label,
        resolved_style->text_color,
        resolved_style->text_size
    );

    context->collapsible_section_stack[context->collapsible_section_stack_count] = state->is_expanded;
    context->collapsible_section_stack_count += 1;

    if (state->is_expanded)
    {
        GUILayoutScope *parent_scope;

        parent_scope = GUI_GetCurrentLayoutScope(context);
        ASSERT(parent_scope->axis == GUI_LAYOUT_AXIS_VERTICAL);

        content_rect = parent_scope->content_rect;
        content_rect.min = parent_scope->cursor;
        content_rect.min.x += resolved_style->indent;
        GUI_BeginLayout(context, content_rect, GUI_LAYOUT_AXIS_VERTICAL, resolved_style->spacing);
    }

    return state->is_expanded;
}

void GUI_EndCollapsibleSection (GUIContext *context)
{
    b32 is_expanded;

    ASSERT(context != NULL);
    ASSERT(context->collapsible_section_stack_count > 0);

    is_expanded = context->collapsible_section_stack[context->collapsible_section_stack_count - 1];
    context->collapsible_section_stack_count -= 1;

    if (is_expanded)
    {
        GUILayoutScope *child_scope;
        GUILayoutScope *parent_scope;
        f32 content_end_y;

        child_scope = GUI_GetCurrentLayoutScope(context);
        content_end_y = MAX(child_scope->cursor.y - child_scope->spacing, child_scope->rect.min.y);
        GUI_EndLayout(context);

        parent_scope = GUI_GetCurrentLayoutScope(context);
        ASSERT(parent_scope->axis == GUI_LAYOUT_AXIS_VERTICAL);
        parent_scope->cursor.y = content_end_y + parent_scope->spacing;
    }
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

b32 GUI_ButtonAuto (GUIContext *context, GUIID id, String text, const GUIButtonStyle *style)
{
    Vec2 measured_size;

    ASSERT(context != NULL);

    measured_size = GUI_MeasureButtonSize(context, text, style);
    return GUI_Button(context, id, text, measured_size.x, style);
}

b32 GUI_PropertyButton (GUIContext *context, GUIID id, String label, String text, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUIButtonStyle *button_style)
{
    const GUIButtonStyle *resolved_button_style;
    b32 clicked;

    ASSERT(context != NULL);

    resolved_button_style = (button_style != NULL) ? button_style : GUI_GetButtonStyle(context);
    GUI_BeginPropertyRow(context, label, label_width, resolved_button_style->height, spacing, label_style);
    clicked = GUI_Button(context, id, text, GUI_GetRemainingWidth(context), button_style);
    GUI_EndPropertyRow(context);
    return clicked;
}

b32 GUI_PropertyToggle (GUIContext *context, GUIID id, String label, b32 *value, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUIToggleStyle *toggle_style)
{
    GUIToggleStyle default_style;
    const GUIToggleStyle *resolved_toggle_style;
    b32 changed;

    ASSERT(context != NULL);

    if (toggle_style == NULL)
    {
        default_style = GUIToggleStyle_Default();
        resolved_toggle_style = &default_style;
    }
    else
    {
        resolved_toggle_style = toggle_style;
    }

    GUI_BeginPropertyRow(context, label, label_width, resolved_toggle_style->height, spacing, label_style);
    changed = GUI_Toggle(context, id, String_Create(NULL, 0), value, toggle_style);
    GUI_EndPropertyRow(context);
    return changed;
}

b32 GUI_PropertySliderF32 (GUIContext *context, GUIID id, String label, f32 min_value, f32 max_value, f32 *value, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUISliderStyle *slider_style)
{
    GUISliderStyle default_style;
    const GUISliderStyle *resolved_slider_style;
    b32 changed;

    ASSERT(context != NULL);

    if (slider_style == NULL)
    {
        default_style = GUISliderStyle_Default();
        resolved_slider_style = &default_style;
    }
    else
    {
        resolved_slider_style = slider_style;
    }

    GUI_BeginPropertyRow(context, label, label_width, resolved_slider_style->height, spacing, label_style);
    changed = GUI_SliderF32(context, id, String_Create(NULL, 0), GUI_GetRemainingWidth(context), min_value, max_value, value, slider_style);
    GUI_EndPropertyRow(context);
    return changed;
}

b32 GUI_PropertyTextField (GUIContext *context, GUIID id, String label, c8 *buffer, usize capacity, usize *length, String placeholder, f32 label_width, f32 spacing, const GUILabelStyle *label_style, const GUITextFieldStyle *text_field_style)
{
    GUITextFieldStyle default_style;
    const GUITextFieldStyle *resolved_text_field_style;
    b32 changed;

    ASSERT(context != NULL);

    if (text_field_style == NULL)
    {
        default_style = GUITextFieldStyle_Default();
        resolved_text_field_style = &default_style;
    }
    else
    {
        resolved_text_field_style = text_field_style;
    }

    GUI_BeginPropertyRow(context, label, label_width, resolved_text_field_style->height, spacing, label_style);
    changed = GUI_TextField(context, id, buffer, capacity, length, placeholder, text_field_style);
    GUI_EndPropertyRow(context);
    return changed;
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

b32 GUI_TextField (GUIContext *context, GUIID id, c8 *buffer, usize capacity, usize *length, String placeholder, const GUITextFieldStyle *style)
{
    Rect2 rect;
    Vec2 text_position;
    Vec4 background_color;
    GUITextFieldStyle default_style;
    const GUITextFieldStyle *resolved_style;
    String text;
    b32 is_hot;
    b32 is_focused;
    b32 changed;
    b32 shift_extend;
    usize event_index;

    ASSERT(context != NULL);
    ASSERT(buffer != NULL);
    ASSERT(length != NULL);
    ASSERT(capacity > 0);
    ASSERT(*length < capacity);

    if (style == NULL)
    {
        default_style = GUITextFieldStyle_Default();
        resolved_style = &default_style;
    }
    else
    {
        resolved_style = style;
    }

    rect = GUI_LayoutNextRect(context, Vec2_Create(0.0f, resolved_style->height));
    text = String_Create(buffer, *length);
    is_hot = GUI_IsHot(context, id, rect);
    is_focused = context->focused_id == id;
    changed = false;

    if (is_hot && context->input.mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        context->active_id = id;
        context->focused_id = id;
        GUI_TextFieldSetCaretAndAnchor(
            context,
            GUI_GetCaretIndexFromPosition(context, rect, resolved_style->padding, text, resolved_style->text_size, context->input.mouse_position.x)
        );
        is_focused = true;
    }
    else if (context->input.mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_LEFT] && !is_hot && is_focused)
    {
        context->focused_id = 0;
        is_focused = false;
    }

    if (context->active_id == id && context->input.mouse_buttons_released[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        context->active_id = 0;
    }

    if ((context->active_id == id) && context->input.mouse_buttons[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        context->text_field_caret = GUI_GetCaretIndexFromPosition(context, rect, resolved_style->padding, text, resolved_style->text_size, context->input.mouse_position.x);
    }

    if (is_focused)
    {
        shift_extend = context->input.shift_is_down;

        if (context->input.keys_pressed[PLATFORM_KEY_LEFT] && (context->text_field_caret > 0))
        {
            context->text_field_caret -= 1;
            if (!shift_extend)
            {
                context->text_field_selection_anchor = context->text_field_caret;
            }
        }

        if (context->input.keys_pressed[PLATFORM_KEY_RIGHT] && (context->text_field_caret < *length))
        {
            context->text_field_caret += 1;
            if (!shift_extend)
            {
                context->text_field_selection_anchor = context->text_field_caret;
            }
        }

        if (context->input.keys_pressed[PLATFORM_KEY_HOME])
        {
            context->text_field_caret = 0;
            if (!shift_extend)
            {
                context->text_field_selection_anchor = 0;
            }
        }

        if (context->input.keys_pressed[PLATFORM_KEY_END])
        {
            context->text_field_caret = *length;
            if (!shift_extend)
            {
                context->text_field_selection_anchor = context->text_field_caret;
            }
        }

        if (context->input.keys_pressed[PLATFORM_KEY_BACKSPACE])
        {
            if (GUI_TextFieldHasSelection(context))
            {
                changed |= GUI_TextFieldDeleteSelection(context, buffer, length);
            }
            else if (context->text_field_caret > 0)
            {
                changed |= GUI_TextFieldDeleteByte(buffer, length, context->text_field_caret - 1);
                context->text_field_caret -= 1;
                context->text_field_selection_anchor = context->text_field_caret;
            }
        }

        if (context->input.keys_pressed[PLATFORM_KEY_DELETE])
        {
            if (GUI_TextFieldHasSelection(context))
            {
                changed |= GUI_TextFieldDeleteSelection(context, buffer, length);
            }
            else if (context->text_field_caret < *length)
            {
                changed |= GUI_TextFieldDeleteByte(buffer, length, context->text_field_caret);
                context->text_field_selection_anchor = context->text_field_caret;
            }
        }

        if (context->input.control_is_down)
        {
            if (context->input.keys_pressed[PLATFORM_KEY_A])
            {
                context->text_field_selection_anchor = 0;
                context->text_field_caret = *length;
            }

            if (context->input.keys_pressed[PLATFORM_KEY_C])
            {
                if (GUI_TextFieldHasSelection(context))
                {
                    Platform_SetClipboardText(GUI_TextFieldGetSelectionText(buffer, context));
                }
            }

            if (context->input.keys_pressed[PLATFORM_KEY_X])
            {
                if (GUI_TextFieldHasSelection(context))
                {
                    Platform_SetClipboardText(GUI_TextFieldGetSelectionText(buffer, context));
                    changed |= GUI_TextFieldDeleteSelection(context, buffer, length);
                }
            }

            if (context->input.keys_pressed[PLATFORM_KEY_V] && Platform_HasClipboardText())
            {
                c8 clipboard_buffer[2048];
                MemoryArena clipboard_arena;
                String clipboard_text;
                usize clipboard_index;

                clipboard_arena = MemoryArena_Create(clipboard_buffer, sizeof(clipboard_buffer));
                clipboard_text = Platform_GetClipboardText(&clipboard_arena);
                if (GUI_TextFieldHasSelection(context))
                {
                    changed |= GUI_TextFieldDeleteSelection(context, buffer, length);
                }

                for (clipboard_index = 0; clipboard_index < clipboard_text.count; clipboard_index += 1)
                {
                    if ((clipboard_text.data[clipboard_index] >= 32) && (clipboard_text.data[clipboard_index] <= 126))
                    {
                        if (GUI_TextFieldInsertByte(buffer, capacity, length, context->text_field_caret, clipboard_text.data[clipboard_index]))
                        {
                            context->text_field_caret += 1;
                            changed = true;
                        }
                    }
                }
            }
        }
        else
        {
            for (event_index = 0; event_index < context->input.text_input_codepoint_count; event_index += 1)
            {
                u32 codepoint;

                codepoint = context->input.text_input_codepoints[event_index];
                if ((codepoint >= 32u) && (codepoint <= 126u))
                {
                    if (GUI_TextFieldHasSelection(context))
                    {
                        changed |= GUI_TextFieldDeleteSelection(context, buffer, length);
                    }

                    if (GUI_TextFieldInsertByte(buffer, capacity, length, context->text_field_caret, (c8) codepoint))
                    {
                        context->text_field_caret += 1;
                        context->text_field_selection_anchor = context->text_field_caret;
                        changed = true;
                    }
                }
            }
        }
    }

    if (context->text_field_caret > *length)
    {
        context->text_field_caret = *length;
    }

    background_color = resolved_style->background_color;
    if (is_focused)
    {
        background_color = resolved_style->focused_color;
    }
    else if (is_hot)
    {
        background_color = resolved_style->hot_color;
    }

    GUI_DrawFilledRect(context, rect, background_color, resolved_style->corner_radii);
    GUI_DrawStrokedRect(context, rect, resolved_style->border_color, resolved_style->border_thickness, resolved_style->corner_radii);

    text = String_Create(buffer, *length);
    text_position = GUI_GetTextPosition(rect, resolved_style->padding, resolved_style->text_size);
    if (text.count > 0)
    {
        if (is_focused && GUI_TextFieldHasSelection(context))
        {
            RangeUsize selection_range;
            f32 selection_min_x;
            f32 selection_max_x;
            Rect2 selection_rect;

            selection_range = GUI_TextFieldGetSelectionRange(context);
            selection_min_x = rect.min.x + resolved_style->padding.x + GUI_MeasureTextWidth(context, String_Prefix(text, selection_range.min), resolved_style->text_size);
            selection_max_x = rect.min.x + resolved_style->padding.x + GUI_MeasureTextWidth(context, String_Prefix(text, selection_range.max), resolved_style->text_size);
            selection_rect = Rect2_Create(
                selection_min_x,
                rect.min.y + resolved_style->padding.y - 1.0f,
                selection_max_x,
                rect.max.y - resolved_style->padding.y + 1.0f
            );
            GUI_DrawFilledRect(context, selection_rect, resolved_style->selection_color, GUICornerRadii_All(2.0f));
        }

        GUI_DrawText(context, text_position, text, resolved_style->text_color, resolved_style->text_size);
    }
    else if (!String_IsEmpty(placeholder))
    {
        GUI_DrawText(context, text_position, placeholder, resolved_style->placeholder_color, resolved_style->text_size);
    }

    if (is_focused)
    {
        f32 caret_x;
        Rect2 caret_rect;

        caret_x = rect.min.x + resolved_style->padding.x + GUI_MeasureTextWidth(context, String_Create(buffer, context->text_field_caret), resolved_style->text_size);
        caret_rect = Rect2_Create(
            caret_x,
            rect.min.y + resolved_style->padding.y - 1.0f,
            caret_x + 1.5f,
            rect.max.y - resolved_style->padding.y + 1.0f
        );
        GUI_DrawFilledRect(context, caret_rect, resolved_style->caret_color, GUICornerRadii_All(0.0f));
    }

    return changed;
}
