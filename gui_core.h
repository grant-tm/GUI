#ifndef GUI_CORE_H
#define GUI_CORE_H

#include "../Platform/platform.h"

typedef u64 GUIID;

typedef struct GUICornerRadii
{
    f32 top_left;
    f32 top_right;
    f32 bottom_right;
    f32 bottom_left;
} GUICornerRadii;

typedef struct GUIEdgeThickness
{
    f32 left;
    f32 top;
    f32 right;
    f32 bottom;
} GUIEdgeThickness;

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

typedef struct GUITextFieldStyle
{
    Vec4 background_color;
    Vec4 hot_color;
    Vec4 focused_color;
    Vec4 border_color;
    Vec4 text_color;
    Vec4 placeholder_color;
    Vec4 caret_color;
    Vec4 selection_color;
    GUICornerRadii corner_radii;
    GUIEdgeThickness border_thickness;
    f32 text_size;
    f32 height;
    Vec2 padding;
} GUITextFieldStyle;

typedef struct GUITextMetrics
{
    Vec2 size;
    f32 line_height;
} GUITextMetrics;

typedef GUITextMetrics GUITextMeasureFunction (String text, f32 size, void *user_data);

typedef enum GUIDrawCommandType
{
    GUI_DRAW_COMMAND_TYPE_NONE = 0,
    GUI_DRAW_COMMAND_TYPE_PUSH_CLIP_RECT,
    GUI_DRAW_COMMAND_TYPE_POP_CLIP_RECT,
    GUI_DRAW_COMMAND_TYPE_FILLED_RECT,
    GUI_DRAW_COMMAND_TYPE_STROKED_RECT,
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
    GUICornerRadii corner_radii;
} GUIDrawCommandFilledRect;

typedef struct GUIDrawCommandStrokedRect
{
    Rect2 rect;
    Vec4 color;
    GUIEdgeThickness thickness;
    GUICornerRadii corner_radii;
} GUIDrawCommandStrokedRect;

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
        GUIDrawCommandStrokedRect stroked_rect;
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
    b32 keys[PLATFORM_KEY_COUNT];
    b32 keys_pressed[PLATFORM_KEY_COUNT];
    b32 keys_released[PLATFORM_KEY_COUNT];
    b32 mouse_buttons[PLATFORM_MOUSE_BUTTON_COUNT];
    b32 mouse_buttons_pressed[PLATFORM_MOUSE_BUTTON_COUNT];
    b32 mouse_buttons_released[PLATFORM_MOUSE_BUTTON_COUNT];
    b32 shift_is_down;
    b32 control_is_down;
    b32 alt_is_down;
    u32 text_input_codepoints[32];
    usize text_input_codepoint_count;
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
    usize max_style_stack_depth;
    usize max_scroll_region_count;
} GUIContextDesc;

typedef enum GUILayoutAxis
{
    GUI_LAYOUT_AXIS_VERTICAL = 0,
    GUI_LAYOUT_AXIS_HORIZONTAL,
} GUILayoutAxis;

typedef struct GUILayoutScope
{
    Rect2 rect;
    Rect2 content_rect;
    Vec2 cursor;
    f32 spacing;
    GUILayoutAxis axis;
} GUILayoutScope;

typedef struct GUIScrollRegionState
{
    GUIID id;
    f32 scroll_y;
    f32 drag_grab_offset_y;
    f32 content_height;
    b32 is_used;
} GUIScrollRegionState;

typedef struct GUIScrollRegionScope
{
    Rect2 rect;
    Rect2 viewport_rect;
    Rect2 content_rect;
    Vec4 scrollbar_track_color;
    Vec4 scrollbar_thumb_color;
    Vec4 scrollbar_thumb_hot_color;
    Vec4 scrollbar_thumb_active_color;
    f32 scrollbar_width;
    f32 scrollbar_padding;
    f32 min_thumb_height;
    GUIScrollRegionState *state;
} GUIScrollRegionScope;

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
    usize text_field_caret;
    usize text_field_selection_anchor;
    GUITextMeasureFunction *text_measure_function;
    void *text_measure_user_data;
    GUIDrawCommandBuffer draw_commands;
    Rect2 *clip_stack;
    usize clip_stack_count;
    usize clip_stack_capacity;
    GUILayoutScope *layout_stack;
    usize layout_stack_count;
    usize layout_stack_capacity;
    GUIScrollRegionState *scroll_region_states;
    usize scroll_region_state_count;
    usize scroll_region_state_capacity;
    GUIScrollRegionScope *scroll_region_stack;
    usize scroll_region_stack_count;
    usize scroll_region_stack_capacity;
    GUIPanelStyle *panel_style_stack;
    usize panel_style_stack_count;
    usize panel_style_stack_capacity;
    GUILabelStyle *label_style_stack;
    usize label_style_stack_count;
    usize label_style_stack_capacity;
    GUIButtonStyle *button_style_stack;
    usize button_style_stack_count;
    usize button_style_stack_capacity;
} GUIContext;

b32 GUIContext_Initialize (GUIContext *context, MemoryArena *persistent_arena, const GUIContextDesc *desc);
void GUIContext_Shutdown (GUIContext *context);
void GUI_BeginFrame (GUIContext *context, const GUIFrameDesc *desc);
void GUI_EndFrame (GUIContext *context);
void GUI_SubmitPlatformInput (GUIContext *context, const PlatformEventBuffer *events, const PlatformInputState *input_state);
const GUIDrawCommandBuffer *GUI_GetDrawCommands (const GUIContext *context);
void GUI_SetTextMeasureFunction (GUIContext *context, GUITextMeasureFunction *function, void *user_data);
GUITextMetrics GUI_MeasureText (const GUIContext *context, String text, f32 size);
f32 GUI_MeasureTextWidth (const GUIContext *context, String text, f32 size);

Vec4 GUIColor_Create (f32 r, f32 g, f32 b, f32 a);
GUICornerRadii GUICornerRadii_Create (f32 top_left, f32 top_right, f32 bottom_right, f32 bottom_left);
GUICornerRadii GUICornerRadii_All (f32 value);
GUIEdgeThickness GUIEdgeThickness_Create (f32 left, f32 top, f32 right, f32 bottom);
GUIEdgeThickness GUIEdgeThickness_All (f32 value);
Rect2 GUIRect_CutTop (Rect2 rect, f32 height);
Rect2 GUIRect_Inset (Rect2 rect, f32 amount_x, f32 amount_y);

GUIPanelStyle GUIPanelStyle_Default (void);
GUILabelStyle GUILabelStyle_Default (void);
GUIButtonStyle GUIButtonStyle_Default (void);
GUITextFieldStyle GUITextFieldStyle_Default (void);

const GUIPanelStyle *GUI_GetPanelStyle (const GUIContext *context);
const GUILabelStyle *GUI_GetLabelStyle (const GUIContext *context);
const GUIButtonStyle *GUI_GetButtonStyle (const GUIContext *context);

void GUI_PushPanelStyle (GUIContext *context, const GUIPanelStyle *style);
void GUI_PopPanelStyle (GUIContext *context);
void GUI_PushLabelStyle (GUIContext *context, const GUILabelStyle *style);
void GUI_PopLabelStyle (GUIContext *context);
void GUI_PushButtonStyle (GUIContext *context, const GUIButtonStyle *style);
void GUI_PopButtonStyle (GUIContext *context);

void GUI_BeginLayout (GUIContext *context, Rect2 rect, GUILayoutAxis axis, f32 spacing);
void GUI_EndLayout (GUIContext *context);
void GUI_BeginRow (GUIContext *context, f32 height, f32 spacing);
void GUI_EndRow (GUIContext *context);
Rect2 GUI_LayoutNextRect (GUIContext *context, Vec2 size);
void GUI_Spacer (GUIContext *context, f32 size);

void GUI_PushClipRect (GUIContext *context, Rect2 rect);
void GUI_PopClipRect (GUIContext *context);
void GUI_DrawFilledRect (GUIContext *context, Rect2 rect, Vec4 color, GUICornerRadii corner_radii);
void GUI_DrawStrokedRect (GUIContext *context, Rect2 rect, Vec4 color, GUIEdgeThickness thickness, GUICornerRadii corner_radii);
void GUI_DrawText (GUIContext *context, Vec2 position, String text, Vec4 color, f32 size);

#endif // GUI_CORE_H
