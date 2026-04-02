#include "gui_core.h"

static b32 GUI_PointInRect (Vec2 point, Rect2 rect)
{
    return Rect2_ContainsPoint(rect, point);
}

static f32 GUI_DefaultMeasureCharacterAdvance (u8 codepoint, f32 size)
{
    f32 factor;

    factor = 0.60f;
    if (codepoint == ' ')
    {
        factor = 0.38f;
    }
    else if ((codepoint == 'i') || (codepoint == 'l') || (codepoint == 'I') || (codepoint == '!') || (codepoint == '.') || (codepoint == ','))
    {
        factor = 0.32f;
    }
    else if ((codepoint == 'm') || (codepoint == 'w') || (codepoint == 'M') || (codepoint == 'W'))
    {
        factor = 0.82f;
    }
    else if ((codepoint >= 'A') && (codepoint <= 'Z'))
    {
        factor = 0.68f;
    }
    else if ((codepoint >= '0') && (codepoint <= '9'))
    {
        factor = 0.62f;
    }

    return size * factor;
}

static GUITextMetrics GUI_DefaultMeasureText (String text, f32 size, void *user_data)
{
    GUITextMetrics result;
    usize index;

    (void) user_data;

    result.size = Vec2_Create(0.0f, size);
    result.line_height = size;

    for (index = 0; index < text.count; index += 1)
    {
        result.size.x += GUI_DefaultMeasureCharacterAdvance((u8) text.data[index], size);
    }

    return result;
}

static String GUI_CopyFrameString (GUIContext *context, String string)
{
    c8 *memory;

    ASSERT(context != NULL);
    ASSERT(context->frame_arena != NULL);

    if (String_IsEmpty(string))
    {
        return string;
    }

    memory = MemoryArena_PushArray(context->frame_arena, c8, string.count);
    ASSERT(memory != NULL);
    Memory_Copy(memory, string.data, string.count);
    return String_Create(memory, string.count);
}

static GUIDrawCommand *GUI_PushDrawCommand (GUIContext *context, GUIDrawCommandType type)
{
    GUIDrawCommand *command;

    ASSERT(context != NULL);

    if (context->draw_commands.count >= context->draw_commands.capacity)
    {
        return NULL;
    }

    command = context->draw_commands.commands + context->draw_commands.count;
    context->draw_commands.count += 1;
    Memory_ZeroStruct(command);
    command->type = type;
    return command;
}

static GUILayoutScope *GUI_GetCurrentLayoutScope (GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->layout_stack_count > 0);
    return context->layout_stack + (context->layout_stack_count - 1);
}

static void GUI_InitializeStyleStacks (GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->panel_style_stack_capacity > 0);
    ASSERT(context->label_style_stack_capacity > 0);
    ASSERT(context->button_style_stack_capacity > 0);

    context->panel_style_stack_count = 1;
    context->label_style_stack_count = 1;
    context->button_style_stack_count = 1;

    context->panel_style_stack[0] = GUIPanelStyle_Default();
    context->label_style_stack[0] = GUILabelStyle_Default();
    context->button_style_stack[0] = GUIButtonStyle_Default();
}

b32 GUIContext_Initialize (GUIContext *context, MemoryArena *persistent_arena, const GUIContextDesc *desc)
{
    ASSERT(context != NULL);
    ASSERT(persistent_arena != NULL);
    ASSERT(desc != NULL);
    ASSERT(desc->max_draw_command_count > 0);
    ASSERT(desc->max_layout_depth > 0);
    ASSERT(desc->max_clip_depth > 0);
    ASSERT(desc->max_style_stack_depth > 0);
    ASSERT(desc->max_scroll_region_count > 0);
    ASSERT(desc->max_collapsible_section_count > 0);

    Memory_ZeroStruct(context);
    context->persistent_arena = persistent_arena;
    context->draw_commands.commands = MemoryArena_PushArrayZero(persistent_arena, GUIDrawCommand, desc->max_draw_command_count);
    context->draw_commands.capacity = desc->max_draw_command_count;
    context->clip_stack = MemoryArena_PushArrayZero(persistent_arena, Rect2, desc->max_clip_depth);
    context->clip_stack_capacity = desc->max_clip_depth;
    context->layout_stack = MemoryArena_PushArrayZero(persistent_arena, GUILayoutScope, desc->max_layout_depth);
    context->layout_stack_capacity = desc->max_layout_depth;
    context->scroll_region_states = MemoryArena_PushArrayZero(persistent_arena, GUIScrollRegionState, desc->max_scroll_region_count);
    context->scroll_region_state_capacity = desc->max_scroll_region_count;
    context->scroll_region_stack = MemoryArena_PushArrayZero(persistent_arena, GUIScrollRegionScope, desc->max_layout_depth);
    context->scroll_region_stack_capacity = desc->max_layout_depth;
    context->collapsible_section_states = MemoryArena_PushArrayZero(persistent_arena, GUICollapsibleSectionState, desc->max_collapsible_section_count);
    context->collapsible_section_state_capacity = desc->max_collapsible_section_count;
    context->collapsible_section_stack = MemoryArena_PushArrayZero(persistent_arena, b32, desc->max_layout_depth);
    context->collapsible_section_stack_capacity = desc->max_layout_depth;
    context->panel_style_stack = MemoryArena_PushArrayZero(persistent_arena, GUIPanelStyle, desc->max_style_stack_depth);
    context->panel_style_stack_capacity = desc->max_style_stack_depth;
    context->label_style_stack = MemoryArena_PushArrayZero(persistent_arena, GUILabelStyle, desc->max_style_stack_depth);
    context->label_style_stack_capacity = desc->max_style_stack_depth;
    context->button_style_stack = MemoryArena_PushArrayZero(persistent_arena, GUIButtonStyle, desc->max_style_stack_depth);
    context->button_style_stack_capacity = desc->max_style_stack_depth;

    if ((context->draw_commands.commands != NULL) &&
        (context->clip_stack != NULL) &&
        (context->layout_stack != NULL) &&
        (context->scroll_region_states != NULL) &&
        (context->scroll_region_stack != NULL) &&
        (context->collapsible_section_states != NULL) &&
        (context->collapsible_section_stack != NULL) &&
        (context->panel_style_stack != NULL) &&
        (context->label_style_stack != NULL) &&
        (context->button_style_stack != NULL))
    {
        GUI_InitializeStyleStacks(context);
        return true;
    }

    return false;
}

void GUIContext_Shutdown (GUIContext *context)
{
    ASSERT(context != NULL);
    Memory_ZeroStruct(context);
}

void GUI_BeginFrame (GUIContext *context, const GUIFrameDesc *desc)
{
    ASSERT(context != NULL);
    ASSERT(desc != NULL);
    ASSERT(desc->frame_arena != NULL);

    context->frame_arena = desc->frame_arena;
    context->window = desc->window;
    context->size = desc->size;
    context->hot_id = 0;
    context->draw_commands.count = 0;
    context->clip_stack_count = 0;
    context->layout_stack_count = 0;
    context->scroll_region_stack_count = 0;
    context->collapsible_section_stack_count = 0;

    if (!context->input.mouse_buttons[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        context->active_id = 0;
        context->drag_origin_id = 0;
        context->drag_origin_control_is_down = false;
    }
}

void GUI_EndFrame (GUIContext *context)
{
    ASSERT(context != NULL);
}

void GUI_SubmitPlatformInput (GUIContext *context, const PlatformEventBuffer *events, const PlatformInputState *input_state)
{
    usize event_index;

    ASSERT(context != NULL);
    ASSERT(events != NULL);
    ASSERT(input_state != NULL);

    Memory_ZeroArray(context->input.mouse_buttons_pressed, ARRAY_COUNT(context->input.mouse_buttons_pressed));
    Memory_ZeroArray(context->input.mouse_buttons_released, ARRAY_COUNT(context->input.mouse_buttons_released));
    Memory_ZeroArray(context->input.keys_pressed, ARRAY_COUNT(context->input.keys_pressed));
    Memory_ZeroArray(context->input.keys_released, ARRAY_COUNT(context->input.keys_released));
    context->input.text_input_codepoint_count = 0;

    context->input.mouse_position = Vec2_Create((f32) input_state->mouse_position.x, (f32) input_state->mouse_position.y);
    context->input.mouse_delta = Vec2_Create((f32) input_state->mouse_delta.x, (f32) input_state->mouse_delta.y);
    context->input.mouse_wheel_delta = input_state->mouse_wheel_delta;
    context->input.shift_is_down = input_state->shift_is_down;
    context->input.control_is_down = input_state->control_is_down;
    context->input.alt_is_down = input_state->alt_is_down;
    Memory_Copy(context->input.keys, input_state->keys, sizeof(input_state->keys));

    for (event_index = 0; event_index < events->count; event_index += 1)
    {
        const PlatformEvent *event;

        event = events->events + event_index;
        if (event->type == PLATFORM_EVENT_MOUSE_BUTTON_DOWN)
        {
            if ((event->data.mouse_button.button > PLATFORM_MOUSE_BUTTON_NONE) &&
                (event->data.mouse_button.button < PLATFORM_MOUSE_BUTTON_COUNT))
            {
                context->input.mouse_buttons_pressed[event->data.mouse_button.button] = true;
                context->input.mouse_button_press_timestamps[event->data.mouse_button.button] = event->timestamp;
            }
        }
        else if (event->type == PLATFORM_EVENT_MOUSE_BUTTON_UP)
        {
            if ((event->data.mouse_button.button > PLATFORM_MOUSE_BUTTON_NONE) &&
                (event->data.mouse_button.button < PLATFORM_MOUSE_BUTTON_COUNT))
            {
                context->input.mouse_buttons_released[event->data.mouse_button.button] = true;
            }
        }
        else if (event->type == PLATFORM_EVENT_KEY_DOWN)
        {
            if ((event->data.key.key > PLATFORM_KEY_NONE) && (event->data.key.key < PLATFORM_KEY_COUNT))
            {
                context->input.keys_pressed[event->data.key.key] = true;
            }
        }
        else if (event->type == PLATFORM_EVENT_KEY_UP)
        {
            if ((event->data.key.key > PLATFORM_KEY_NONE) && (event->data.key.key < PLATFORM_KEY_COUNT))
            {
                context->input.keys_released[event->data.key.key] = true;
            }
        }
        else if (event->type == PLATFORM_EVENT_TEXT_INPUT)
        {
            if (context->input.text_input_codepoint_count < ARRAY_COUNT(context->input.text_input_codepoints))
            {
                context->input.text_input_codepoints[context->input.text_input_codepoint_count] = event->data.text_input.codepoint;
                context->input.text_input_codepoint_count += 1;
            }
        }
    }

    Memory_Copy(context->input.mouse_buttons, input_state->mouse_buttons, sizeof(input_state->mouse_buttons));
}

const GUIDrawCommandBuffer *GUI_GetDrawCommands (const GUIContext *context)
{
    ASSERT(context != NULL);
    return &context->draw_commands;
}

void GUI_SetTextMeasureFunction (GUIContext *context, GUITextMeasureFunction *function, void *user_data)
{
    ASSERT(context != NULL);
    context->text_measure_function = function;
    context->text_measure_user_data = user_data;
}

GUITextMetrics GUI_MeasureText (const GUIContext *context, String text, f32 size)
{
    ASSERT(context != NULL);

    if (context->text_measure_function != NULL)
    {
        return context->text_measure_function(text, size, context->text_measure_user_data);
    }

    return GUI_DefaultMeasureText(text, size, NULL);
}

f32 GUI_MeasureTextWidth (const GUIContext *context, String text, f32 size)
{
    return GUI_MeasureText(context, text, size).size.x;
}

Vec4 GUIColor_Create (f32 r, f32 g, f32 b, f32 a)
{
    return Vec4_Create(r, g, b, a);
}

GUICornerRadii GUICornerRadii_Create (f32 top_left, f32 top_right, f32 bottom_right, f32 bottom_left)
{
    GUICornerRadii result;

    result.top_left = top_left;
    result.top_right = top_right;
    result.bottom_right = bottom_right;
    result.bottom_left = bottom_left;
    return result;
}

GUICornerRadii GUICornerRadii_All (f32 value)
{
    return GUICornerRadii_Create(value, value, value, value);
}

GUIEdgeThickness GUIEdgeThickness_Create (f32 left, f32 top, f32 right, f32 bottom)
{
    GUIEdgeThickness result;

    result.left = left;
    result.top = top;
    result.right = right;
    result.bottom = bottom;
    return result;
}

GUIEdgeThickness GUIEdgeThickness_All (f32 value)
{
    return GUIEdgeThickness_Create(value, value, value, value);
}

const GUIPanelStyle *GUI_GetPanelStyle (const GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->panel_style_stack_count > 0);
    return context->panel_style_stack + (context->panel_style_stack_count - 1);
}

const GUILabelStyle *GUI_GetLabelStyle (const GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->label_style_stack_count > 0);
    return context->label_style_stack + (context->label_style_stack_count - 1);
}

const GUIButtonStyle *GUI_GetButtonStyle (const GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->button_style_stack_count > 0);
    return context->button_style_stack + (context->button_style_stack_count - 1);
}

void GUI_PushPanelStyle (GUIContext *context, const GUIPanelStyle *style)
{
    ASSERT(context != NULL);
    ASSERT(style != NULL);
    ASSERT(context->panel_style_stack_count < context->panel_style_stack_capacity);

    context->panel_style_stack[context->panel_style_stack_count] = *style;
    context->panel_style_stack_count += 1;
}

void GUI_PopPanelStyle (GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->panel_style_stack_count > 1);
    context->panel_style_stack_count -= 1;
}

void GUI_PushLabelStyle (GUIContext *context, const GUILabelStyle *style)
{
    ASSERT(context != NULL);
    ASSERT(style != NULL);
    ASSERT(context->label_style_stack_count < context->label_style_stack_capacity);

    context->label_style_stack[context->label_style_stack_count] = *style;
    context->label_style_stack_count += 1;
}

void GUI_PopLabelStyle (GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->label_style_stack_count > 1);
    context->label_style_stack_count -= 1;
}

void GUI_PushButtonStyle (GUIContext *context, const GUIButtonStyle *style)
{
    ASSERT(context != NULL);
    ASSERT(style != NULL);
    ASSERT(context->button_style_stack_count < context->button_style_stack_capacity);

    context->button_style_stack[context->button_style_stack_count] = *style;
    context->button_style_stack_count += 1;
}

void GUI_PopButtonStyle (GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->button_style_stack_count > 1);
    context->button_style_stack_count -= 1;
}

Rect2 GUIRect_CutTop (Rect2 rect, f32 height)
{
    rect.max.y = MIN(rect.max.y, rect.min.y + height);
    return rect;
}

Rect2 GUIRect_Inset (Rect2 rect, f32 amount_x, f32 amount_y)
{
    rect.min.x += amount_x;
    rect.min.y += amount_y;
    rect.max.x -= amount_x;
    rect.max.y -= amount_y;
    return rect;
}

void GUI_BeginLayout (GUIContext *context, Rect2 rect, GUILayoutAxis axis, f32 spacing)
{
    GUILayoutScope *scope;

    ASSERT(context != NULL);
    ASSERT(context->layout_stack_count < context->layout_stack_capacity);

    scope = context->layout_stack + context->layout_stack_count;
    context->layout_stack_count += 1;
    scope->rect = rect;
    scope->content_rect = rect;
    scope->cursor = rect.min;
    scope->spacing = spacing;
    scope->axis = axis;
}

void GUI_EndLayout (GUIContext *context)
{
    ASSERT(context != NULL);
    ASSERT(context->layout_stack_count > 0);
    context->layout_stack_count -= 1;
}

Rect2 GUI_LayoutNextRect (GUIContext *context, Vec2 size)
{
    GUILayoutScope *scope;
    Rect2 rect;

    scope = GUI_GetCurrentLayoutScope(context);
    rect.min = scope->cursor;
    rect.max = rect.min;

    if (scope->axis == GUI_LAYOUT_AXIS_VERTICAL)
    {
        ASSERT(size.y > 0.0f);
        rect.max.x = (size.x > 0.0f) ? MIN(scope->content_rect.max.x, rect.min.x + size.x) : scope->content_rect.max.x;
        rect.max.y = rect.min.y + size.y;
        scope->cursor.y = rect.max.y + scope->spacing;
    }
    else
    {
        rect.max.x = (size.x > 0.0f) ? (rect.min.x + size.x) : scope->content_rect.max.x;
        rect.max.y = (size.y > 0.0f) ? MIN(scope->content_rect.max.y, rect.min.y + size.y) : scope->content_rect.max.y;
        scope->cursor.x = rect.max.x + scope->spacing;
    }

    return rect;
}

f32 GUI_GetRemainingWidth (const GUIContext *context)
{
    const GUILayoutScope *scope;

    ASSERT(context != NULL);
    ASSERT(context->layout_stack_count > 0);

    scope = context->layout_stack + (context->layout_stack_count - 1);
    return MAX(0.0f, scope->content_rect.max.x - scope->cursor.x);
}

f32 GUI_GetRemainingHeight (const GUIContext *context)
{
    const GUILayoutScope *scope;

    ASSERT(context != NULL);
    ASSERT(context->layout_stack_count > 0);

    scope = context->layout_stack + (context->layout_stack_count - 1);
    return MAX(0.0f, scope->content_rect.max.y - scope->cursor.y);
}

Rect2 GUI_LayoutNextRemainingRect (GUIContext *context)
{
    const GUILayoutScope *scope;

    ASSERT(context != NULL);
    ASSERT(context->layout_stack_count > 0);

    scope = context->layout_stack + (context->layout_stack_count - 1);
    if (scope->axis == GUI_LAYOUT_AXIS_VERTICAL)
    {
        return GUI_LayoutNextRect(context, Vec2_Create(0.0f, GUI_GetRemainingHeight(context)));
    }

    return GUI_LayoutNextRect(context, Vec2_Create(GUI_GetRemainingWidth(context), 0.0f));
}

void GUI_BeginRow (GUIContext *context, f32 height, f32 spacing)
{
    Rect2 rect;

    ASSERT(context != NULL);
    ASSERT(height > 0.0f);

    rect = GUI_LayoutNextRect(context, Vec2_Create(0.0f, height));
    GUI_BeginLayout(context, rect, GUI_LAYOUT_AXIS_HORIZONTAL, spacing);
}

void GUI_EndRow (GUIContext *context)
{
    GUI_EndLayout(context);
}

void GUI_Spacer (GUIContext *context, f32 size)
{
    GUILayoutScope *scope;

    ASSERT(context != NULL);
    ASSERT(size >= 0.0f);

    scope = GUI_GetCurrentLayoutScope(context);
    if (scope->axis == GUI_LAYOUT_AXIS_VERTICAL)
    {
        scope->cursor.y += size;
    }
    else
    {
        scope->cursor.x += size;
    }
}

void GUI_PushClipRect (GUIContext *context, Rect2 rect)
{
    GUIDrawCommand *command;

    ASSERT(context != NULL);
    ASSERT(context->clip_stack_count < context->clip_stack_capacity);

    context->clip_stack[context->clip_stack_count] = rect;
    context->clip_stack_count += 1;

    command = GUI_PushDrawCommand(context, GUI_DRAW_COMMAND_TYPE_PUSH_CLIP_RECT);
    ASSERT(command != NULL);
    command->data.push_clip_rect.rect = rect;
}

void GUI_PopClipRect (GUIContext *context)
{
    GUIDrawCommand *command;

    ASSERT(context != NULL);
    ASSERT(context->clip_stack_count > 0);

    context->clip_stack_count -= 1;
    command = GUI_PushDrawCommand(context, GUI_DRAW_COMMAND_TYPE_POP_CLIP_RECT);
    ASSERT(command != NULL);
}

void GUI_DrawFilledRect (GUIContext *context, Rect2 rect, Vec4 color, GUICornerRadii corner_radii)
{
    GUIDrawCommand *command;

    command = GUI_PushDrawCommand(context, GUI_DRAW_COMMAND_TYPE_FILLED_RECT);
    ASSERT(command != NULL);
    command->data.filled_rect.rect = rect;
    command->data.filled_rect.color = color;
    command->data.filled_rect.corner_radii = corner_radii;
}

void GUI_DrawStrokedRect (GUIContext *context, Rect2 rect, Vec4 color, GUIEdgeThickness thickness, GUICornerRadii corner_radii)
{
    GUIDrawCommand *command;

    command = GUI_PushDrawCommand(context, GUI_DRAW_COMMAND_TYPE_STROKED_RECT);
    ASSERT(command != NULL);
    command->data.stroked_rect.rect = rect;
    command->data.stroked_rect.color = color;
    command->data.stroked_rect.thickness = thickness;
    command->data.stroked_rect.corner_radii = corner_radii;
}

void GUI_DrawText (GUIContext *context, Vec2 position, String text, Vec4 color, f32 size)
{
    GUIDrawCommand *command;

    command = GUI_PushDrawCommand(context, GUI_DRAW_COMMAND_TYPE_TEXT);
    ASSERT(command != NULL);
    command->data.text.position = position;
    command->data.text.text = GUI_CopyFrameString(context, text);
    command->data.text.color = color;
    command->data.text.size = size;
}

void GUI_DrawImage (GUIContext *context, u64 image_id, Rect2 rect, Rect2 uv, Vec4 tint)
{
    GUIDrawCommand *command;

    ASSERT(context != NULL);
    ASSERT(image_id != 0);

    command = GUI_PushDrawCommand(context, GUI_DRAW_COMMAND_TYPE_IMAGE);
    ASSERT(command != NULL);
    command->data.image.image_id = image_id;
    command->data.image.rect = rect;
    command->data.image.uv = uv;
    command->data.image.tint = tint;
}

void GUI_DrawImageFit (GUIContext *context, u64 image_id, Rect2 rect, Vec2 image_size, GUIImageFitMode fit_mode, Vec4 tint)
{
    Rect2 draw_rect;
    Rect2 uv_rect;
    f32 rect_width;
    f32 rect_height;
    f32 image_aspect;
    f32 rect_aspect;
    f32 draw_width;
    f32 draw_height;
    f32 pad_x;
    f32 pad_y;
    f32 uv_trim_x;
    f32 uv_trim_y;

    ASSERT(context != NULL);
    ASSERT(image_id != 0);

    draw_rect = rect;
    uv_rect = Rect2_Create(0.0f, 0.0f, 1.0f, 1.0f);
    rect_width = rect.max.x - rect.min.x;
    rect_height = rect.max.y - rect.min.y;

    if ((image_size.x <= 0.0f) || (image_size.y <= 0.0f) ||
        (rect_width <= 0.0f) || (rect_height <= 0.0f))
    {
        GUI_DrawImage(context, image_id, draw_rect, uv_rect, tint);
        return;
    }

    image_aspect = image_size.x / image_size.y;
    rect_aspect = rect_width / rect_height;

    if (fit_mode == GUI_IMAGE_FIT_MODE_CONTAIN)
    {
        if (image_aspect > rect_aspect)
        {
            draw_width = rect_width;
            draw_height = rect_width / image_aspect;
        }
        else
        {
            draw_height = rect_height;
            draw_width = rect_height * image_aspect;
        }

        pad_x = (rect_width - draw_width) * 0.5f;
        pad_y = (rect_height - draw_height) * 0.5f;
        draw_rect = Rect2_Create(
            rect.min.x + pad_x,
            rect.min.y + pad_y,
            rect.min.x + pad_x + draw_width,
            rect.min.y + pad_y + draw_height);
    }
    else if (fit_mode == GUI_IMAGE_FIT_MODE_COVER)
    {
        if (image_aspect > rect_aspect)
        {
            uv_trim_x = (1.0f - (rect_aspect / image_aspect)) * 0.5f;
            uv_rect = Rect2_Create(uv_trim_x, 0.0f, 1.0f - uv_trim_x, 1.0f);
        }
        else if (image_aspect < rect_aspect)
        {
            uv_trim_y = (1.0f - (image_aspect / rect_aspect)) * 0.5f;
            uv_rect = Rect2_Create(0.0f, uv_trim_y, 1.0f, 1.0f - uv_trim_y);
        }
    }

    GUI_DrawImage(context, image_id, draw_rect, uv_rect, tint);
}
