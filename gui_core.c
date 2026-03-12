#include "gui_core.h"

static b32 GUI_PointInRect (Vec2 point, Rect2 rect)
{
    return Rect2_ContainsPoint(rect, point);
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

b32 GUIContext_Initialize (GUIContext *context, MemoryArena *persistent_arena, const GUIContextDesc *desc)
{
    ASSERT(context != NULL);
    ASSERT(persistent_arena != NULL);
    ASSERT(desc != NULL);
    ASSERT(desc->max_draw_command_count > 0);
    ASSERT(desc->max_layout_depth > 0);
    ASSERT(desc->max_clip_depth > 0);

    Memory_ZeroStruct(context);
    context->persistent_arena = persistent_arena;
    context->draw_commands.commands = MemoryArena_PushArrayZero(persistent_arena, GUIDrawCommand, desc->max_draw_command_count);
    context->draw_commands.capacity = desc->max_draw_command_count;
    context->clip_stack = MemoryArena_PushArrayZero(persistent_arena, Rect2, desc->max_clip_depth);
    context->clip_stack_capacity = desc->max_clip_depth;
    context->layout_stack = MemoryArena_PushArrayZero(persistent_arena, GUILayoutScope, desc->max_layout_depth);
    context->layout_stack_capacity = desc->max_layout_depth;

    return (context->draw_commands.commands != NULL) &&
           (context->clip_stack != NULL) &&
           (context->layout_stack != NULL);
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

    if (!context->input.mouse_buttons[PLATFORM_MOUSE_BUTTON_LEFT])
    {
        context->active_id = 0;
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

    context->input.mouse_position = Vec2_Create((f32) input_state->mouse_position.x, (f32) input_state->mouse_position.y);
    context->input.mouse_delta = Vec2_Create((f32) input_state->mouse_delta.x, (f32) input_state->mouse_delta.y);
    context->input.mouse_wheel_delta = input_state->mouse_wheel_delta;
    context->input.shift_is_down = input_state->shift_is_down;
    context->input.control_is_down = input_state->control_is_down;
    context->input.alt_is_down = input_state->alt_is_down;

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
    }

    Memory_Copy(context->input.mouse_buttons, input_state->mouse_buttons, sizeof(input_state->mouse_buttons));
}

const GUIDrawCommandBuffer *GUI_GetDrawCommands (const GUIContext *context)
{
    ASSERT(context != NULL);
    return &context->draw_commands;
}

Vec4 GUIColor_Create (f32 r, f32 g, f32 b, f32 a)
{
    return Vec4_Create(r, g, b, a);
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

void GUI_DrawFilledRect (GUIContext *context, Rect2 rect, Vec4 color, f32 corner_radius)
{
    GUIDrawCommand *command;

    command = GUI_PushDrawCommand(context, GUI_DRAW_COMMAND_TYPE_FILLED_RECT);
    ASSERT(command != NULL);
    command->data.filled_rect.rect = rect;
    command->data.filled_rect.color = color;
    command->data.filled_rect.corner_radius = corner_radius;
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
