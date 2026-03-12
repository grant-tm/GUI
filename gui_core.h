#ifndef GUI_CORE_H
#define GUI_CORE_H

#include "../Platform/platform.h"

typedef u64 GUIID;

typedef enum GUIDrawCommandType
{
    GUI_DRAW_COMMAND_TYPE_NONE = 0,
    GUI_DRAW_COMMAND_TYPE_PUSH_CLIP_RECT,
    GUI_DRAW_COMMAND_TYPE_POP_CLIP_RECT,
    GUI_DRAW_COMMAND_TYPE_FILLED_RECT,
    GUI_DRAW_COMMAND_TYPE_TEXT,
} GUIDrawCommandType;

typedef struct GUIDrawCommandPushClipRect
{
    Rect2 rect;
} GUIDrawCommandPushClipRect;

typedef struct GUIDrawCommandFilledRect
{
    Rect2 rect;
    Vec4 color;
    f32 corner_radius;
} GUIDrawCommandFilledRect;

typedef struct GUIDrawCommandText
{
    Vec2 position;
    String text;
    Vec4 color;
    f32 size;
} GUIDrawCommandText;

typedef struct GUIDrawCommand
{
    GUIDrawCommandType type;

    union
    {
        GUIDrawCommandPushClipRect push_clip_rect;
        GUIDrawCommandFilledRect filled_rect;
        GUIDrawCommandText text;
    } data;
} GUIDrawCommand;

typedef struct GUIDrawCommandBuffer
{
    GUIDrawCommand *commands;
    usize count;
    usize capacity;
} GUIDrawCommandBuffer;

typedef struct GUIInputState
{
    Vec2 mouse_position;
    Vec2 mouse_delta;
    i32 mouse_wheel_delta;
    b32 mouse_buttons[PLATFORM_MOUSE_BUTTON_COUNT];
    b32 mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_COUNT];
    b32 mouse_buttons_released[PLATFORM_MOUSE_BUTTON_COUNT];
    b32 shift_is_down;
    b32 control_is_down;
    b32 alt_is_down;
} GUIInputState;

typedef struct GUIFrameDesc
{
    PlatformWindow window;
    Vec2 size;
    MemoryArena *frame_arena;
} GUIFrameDesc;

typedef struct GUIContextDesc
{
    usize max_draw_command_count;
    usize max_layout_depth;
    usize max_clip_depth;
} GUIContextDesc;

typedef struct GUILayoutScope
{
    Rect2 rect;
    Rect2 content_rect;
    Vec2 cursor;
    f32 spacing;
} GUILayoutScope;

typedef struct GUIContext
{
    MemoryArena *persistent_arena;
    MemoryArena *frame_arena;
    PlatformWindow window;
    Vec2 size;
    GUIInputState input;
    GUIID hot_id;
    GUIID active_id;
    GUIID focused_id;
    GUIDrawCommandBuffer draw_commands;
    Rect2 *clip_stack;
    usize clip_stack_count;
    usize clip_stack_capacity;
    GUILayoutScope *layout_stack;
    usize layout_stack_count;
    usize layout_stack_capacity;
} GUIContext;

b32 GUIContext_Initialize (GUIContext *context, MemoryArena *persistent_arena, const GUIContextDesc *desc);
void GUIContext_Shutdown (GUIContext *context);
void GUI_BeginFrame (GUIContext *context, const GUIFrameDesc *desc);
void GUI_EndFrame (GUIContext *context);
void GUI_SubmitPlatformInput (GUIContext *context, const PlatformEventBuffer *events, const PlatformInputState *input_state);
const GUIDrawCommandBuffer *GUI_GetDrawCommands (const GUIContext *context);

Vec4 GUIColor_Create (f32 r, f32 g, f32 b, f32 a);
Rect2 GUIRect_CutTop (Rect2 rect, f32 height);
Rect2 GUIRect_Inset (Rect2 rect, f32 amount_x, f32 amount_y);

void GUI_PushClipRect (GUIContext *context, Rect2 rect);
void GUI_PopClipRect (GUIContext *context);
void GUI_DrawFilledRect (GUIContext *context, Rect2 rect, Vec4 color, f32 corner_radius);
void GUI_DrawText (GUIContext *context, Vec2 position, String text, Vec4 color, f32 size);

#endif // GUI_CORE_H
