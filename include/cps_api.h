/* CPS (Custom Plugin System) C API, see SDK.md. Structs start with their size and only grow at the end. */
#ifndef CPS_API_H
#define CPS_API_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CPS_API_VERSION 2u

/* 0 is never a valid client. */
typedef uint32_t cps_client;

enum cps_event_type {
    CPS_EVENT_COMMAND_RESULT = 1, /* for commands sent with a "request" field; only the sender gets it */
    CPS_EVENT_MENU_OPENED,
    CPS_EVENT_MENU_CLOSED,
    CPS_EVENT_PLAYER_SPAWNED,   /* after a load or map change */
    CPS_EVENT_WARDROBE_APPLIED, /* this and the next five come from the CNS plugin */
    CPS_EVENT_WARDROBE_CLEARED, /* slot "" means every slot */
    CPS_EVENT_ANIMATION_STARTED,
    CPS_EVENT_ANIMATION_STOPPED,
    CPS_EVENT_POSE_APPLIED,
    CPS_EVENT_POSE_CLEARED,
    CPS_EVENT_CAMERA_LOST,     /* the menu took the camera back; detail has a "reason"; holder only */
    CPS_EVENT_HOTKEY_CONFLICT, /* one of your hotkeys shares a binding; detail names the other; owner only */
    CPS_EVENT_RETURN_GAMEPLAY, /* "return to gameplay": stop previews and poses */
    CPS_EVENT_LANGUAGE_CHANGED /* API 2. The player picked another language; detail {"language": "fr"} */
};

#define CPS_EVENT_BIT(type) (1u << (type))
#define CPS_EVENT_ALL 0xFFFFFFFEu

typedef struct cps_event {
    uint32_t size, type;
    const char* character; /* "EVE", "ADAM", "LILY", "DRONE" or "" */
    const char* slot;      /* wardrobe or animation slot, or "" */
    const char* detail;    /* JSON object, never null; valid only during the callback */
} cps_event;

typedef void (*cps_event_fn)(void* user, const cps_event* event);
typedef void (*cps_tick_fn)(void* user, float delta_seconds);
typedef void (*cps_hotkey_fn)(void* user);

#define CPS_MOD_ALT 1u
#define CPS_MOD_CTRL 2u
#define CPS_MOD_SHIFT 4u

/* Gamepad buttons, Xbox names: A is Cross, B Circle, X Square, Y Triangle, BACK View/Create, START Menu/Options. */
#define CPS_PAD_A 0x0001u
#define CPS_PAD_B 0x0002u
#define CPS_PAD_X 0x0004u
#define CPS_PAD_Y 0x0008u
#define CPS_PAD_LB 0x0010u
#define CPS_PAD_RB 0x0020u
#define CPS_PAD_LT 0x0040u
#define CPS_PAD_RT 0x0080u
#define CPS_PAD_BACK 0x0100u
#define CPS_PAD_START 0x0200u
#define CPS_PAD_LS 0x0400u
#define CPS_PAD_RS 0x0800u
#define CPS_PAD_UP 0x1000u
#define CPS_PAD_DOWN 0x2000u
#define CPS_PAD_LEFT 0x4000u
#define CPS_PAD_RIGHT 0x8000u

typedef struct cps_state {
    uint32_t size;
    int in_gameplay; /* no cutscene, menu, combat lock, fishing or interaction */
    int menu_open;
    int studio_camera; /* the menu's own camera has the view */
    int animating;     /* a plugin reports an animation preview */
    int posed;
    float idle_seconds;        /* since the last keyboard, mouse or gamepad input */
    char gameplay_reason[128]; /* why in_gameplay is 0 */
    int player_idle;           /* confirmed grounded idle animation, without movement or a gameplay action */
    char idle_reason[128];     /* why player_idle is 0; appended in CPS 1.1.0 */
} cps_state;

/* Any thread: the state the core last published. */
typedef struct cps_status {
    uint32_t size;
    int engine_ready, in_gameplay, menu_open;
    int characters[4]; /* instances found nearby: EVE, ADAM, LILY, DRONE */
    int camera_target; /* character the studio camera frames */
    int camera_mode;   /* 0 gameplay, 1 orbit, 2 fly, 3 a plugin holds it */
    int prompts;       /* API 2. CPS_PROMPTS_*: the button style CPS's hint bar shows now */
} cps_status;

#define CPS_PROMPTS_KEYBOARD 0
#define CPS_PROMPTS_XBOX 1
#define CPS_PROMPTS_PLAYSTATION 2

/* World units are centimetres, angles degrees. */
typedef struct cps_character_info {
    uint32_t size;
    float location[3];
    float rotation[3];      /* actor pitch, yaw, roll */
    float mesh_rotation[3]; /* body mesh; can face differently from the actor */
    float floor_z;          /* bottom of the collision capsule */
} cps_character_info;

typedef struct cps_shot {
    uint32_t size;
    float location[3];
    float rotation[3];                 /* pitch, yaw, roll */
    float focal_length;                /* mm */
    float focus_distance;              /* cm; 0 turns depth of field off */
    float sensor_width, sensor_height; /* mm */
    float aperture;                    /* f-stop */
    int cut;                           /* 1: no motion blur or temporal history from the previous frame */
} cps_shot;

/* camera_acquire() flags. */
#define CPS_CAMERA_HIDE_HUD 1u    /* if the player's settings allow it */
#define CPS_CAMERA_BLOCK_INPUT 2u /* blocked input still resets idle_seconds in state() */

/* set_activity() flags; state() and the studio camera combine every plugin's. */
#define CPS_ACTIVITY_ANIMATING 1u
#define CPS_ACTIVITY_POSED 2u /* the orbit camera frames the posed body */

/* theme_color() colours. */
#define CPS_COLOR_TEXT 0u
#define CPS_COLOR_DIM 1u
#define CPS_COLOR_ACCENT 2u
#define CPS_COLOR_PANEL 3u
#define CPS_COLOR_ROW 4u
#define CPS_COLOR_ROW_HOVER 5u
#define CPS_COLOR_ERROR 6u
#define CPS_COLOR_WARNING 7u
#define CPS_COLOR_PANEL_BORDER 8u
#define CPS_COLOR_RULE 9u
#define CPS_COLOR_TRACK 10u /* slider tracks */

/* imgui_font() fonts. */
#define CPS_FONT_BODY 0u
#define CPS_FONT_STRONG 1u
#define CPS_FONT_TITLE 2u

/* API 2. A control hint, drawn like the menu's own: a key cap, or Xbox or PlayStation buttons, whichever CPS's
   hint bar shows now. A hint with nothing for the current input is left out. */
typedef struct cps_hint {
    uint32_t size;
    const char* label;  /* "Move" */
    const char* key;    /* keyboard: key text such as "W" or "Shift"; "Mouse" draws a mouse */
    uint32_t pad;       /* controller: CPS_PAD_* buttons */
    const char* hotkey; /* optional: one of the plugin's hotkey ids; its current key and buttons are shown */
    uint32_t flags;     /* CPS_HINT_* */
} cps_hint;

#define CPS_HINT_EITHER 1u /* the buttons are alternatives: "/" between them, not "+" */
#define CPS_HINT_GAP 2u    /* a wider gap before it, starting a group */

/* Screen pixels. */
typedef struct cps_rect {
    float x, y, width, height;
} cps_rect;

/* Draw callbacks only. int results are 1 on change or press. Labels and ids are UTF-8, unique per page.
   Colours are 0xAABBGGRR, as Dear ImGui's IM_COL32. */
typedef struct cps_ui {
    uint32_t size;
    float (*width)(void); /* pixels */
    void (*heading)(const char* text);
    void (*text)(const char* text); /* wraps to the page width */
    void (*text_dim)(const char* text);
    void (*rule)(void);
    int (*button)(const char* label);
    int (*toggle)(const char* label, int* value);
    /* Ctrl+click the number for keyboard entry, clamped to min/max. */
    int (*slider)(const char* label, float* value, float min, float max, const char* format);
    int (*color)(const char* label, float rgba[4]);
    int (*choice)(const char* id, const char* const* items, int count, int* index);
    int (*input)(const char* id, char* buffer, uint32_t capacity, const char* hint);
    /* For plugins that draw with Dear ImGui themselves: plugin.toml must name this imgui_version.
       Set the allocators and context before use; sdk/cpp/ui/adopt.hpp does all of it. */
    void* (*imgui_context)(void);
    const char* imgui_version;
    void (*imgui_allocators)(void** alloc_fn, void** free_fn, void** user_data);
    /* Custom drawing. scale() is pixels per unit at this resolution and interface scale. */
    float (*scale)(void);
    uint32_t (*theme_color)(uint32_t which);
    void (*cursor)(float out[2]);                           /* where the next widget goes */
    void (*space)(float width, float height);               /* reserves an area at the cursor and moves below it */
    int (*area)(const char* id, float width, float height); /* clickable area at the cursor; 1 when clicked */
    int (*item_hovered)(void);                              /* the last widget or area */
    void (*rect)(const float min[2], const float max[2], uint32_t color, float thickness); /* thickness 0 fills */
    void (*line)(const float a[2], const float b[2], uint32_t color, float thickness);
    void (*circle)(const float center[2], float radius, uint32_t color, float thickness); /* thickness 0 fills */
    void (*label)(const float pos[2], uint32_t color, float size, const char* text);      /* size 0: body text */
    float (*text_width)(const char* text, float size);
    /* The menu's info line for this frame, and what A / Cross does in the hint bar. */
    void (*status)(const char* text);
    void (*action_hint)(const char* label);
    void (*image)(uint64_t image, const float min[2], const float max[2], uint32_t tint); /* from api->image */
    void* (*imgui_font)(uint32_t which); /* ImFont*, CPS_FONT_*; push it with the size you want */
    /* API 2. A row of hints at the cursor (tabs), or with bar set a bar along the bottom of the screen like the
       menu's (tabs and overlays). */
    void (*hints)(const cps_hint* hints, uint32_t count, int bar);
} cps_ui;

typedef void (*cps_draw_fn)(void* user, const cps_ui* ui, const cps_rect* area);
/* Render thread; return 1 when the tab handled Back itself. */
typedef int (*cps_back_fn)(void* user);

/* Tab placements. */
#define CPS_TAB_SIDEBAR 0u       /* icon in the sidebar's upper group; drawn in the page panel */
#define CPS_TAB_SIDEBAR_LOWER 1u /* icon above the menu's own pages */
#define CPS_TAB_LEFT 2u          /* the left panel */
#define CPS_TAB_PLUGIN 3u        /* opened from the Plugins page */
#define CPS_TAB_SETTINGS 4u      /* a section of the Settings page */
#define CPS_TAB_DIAGNOSTICS 5u   /* a section of the Diagnostics page */

typedef struct cps_tab {
    uint32_t size;
    const char* id; /* unique per plugin */
    const char* title;
    const char* icon; /* UTF-8 Font Awesome 6 solid glyph; sidebar tabs need one */
    uint32_t placement;
    cps_draw_fn draw; /* render thread, every frame the tab is shown */
    void* user;
    cps_back_fn back; /* optional */
} cps_tab;

#define CPS_OVERLAY_ALWAYS 1u /* also while the menu is closed */

/* Full screen drawing below the menu's panels; area is the screen. */
typedef struct cps_overlay {
    uint32_t size;
    cps_draw_fn draw;
    void* user;
    uint32_t flags;
} cps_overlay;

/* Hotkey flags. */
#define CPS_HOTKEY_MENU 1u    /* only while the menu is open */
#define CPS_HOTKEY_GAMEPAD 2u /* can also have a gamepad binding */
#define CPS_HOTKEY_HINT 4u    /* shown in the menu's hint bar as hint */

typedef struct cps_hotkey {
    uint32_t size;
    const char* id; /* unique per plugin */
    const char* label;
    uint32_t key, modifiers; /* default: a virtual-key code and CPS_MOD_*; key 0 leaves it unbound */
    uint32_t pad;            /* default gamepad buttons, one or two CPS_PAD_*; needs CPS_HOTKEY_GAMEPAD */
    uint32_t flags;
    cps_hotkey_fn fn; /* game thread */
    void* user;
    const char* hint; /* short hint bar text */
} cps_hotkey;

/* Another hotkey with the same binding. */
typedef struct cps_conflict {
    uint32_t size;
    int found;
    int gamepad;      /* the gamepad binding clashes, not the key */
    char binding[64]; /* e.g. "Ctrl + F7" */
    char owner[64];   /* plugin id of the other hotkey; "cps" for the menu's own */
    char owner_name[96];
    char hotkey[64]; /* its id */
    char label[96];
} cps_conflict;

/* Command handler results. */
#define CPS_COMMAND_FAILED 0
#define CPS_COMMAND_DONE 1
#define CPS_COMMAND_PENDING 2 /* finish later with finish_command */

typedef struct cps_command {
    uint32_t size;
    const char* action;
    const char* json;  /* the whole command object */
    cps_client sender; /* 0 for the menu itself */
    uint32_t id;       /* for finish_command */
} cps_command;

/* Game thread. The message (UTF-8, cut to capacity) goes to the toast and CPS_EVENT_COMMAND_RESULT. */
typedef int (*cps_command_fn)(void* user, const cps_command* command, char* message, uint32_t capacity);
/* Any thread. Writes JSON into out and returns its full length; called again with more room when it didn't fit. */
typedef uint32_t (*cps_json_fn)(void* user, const char* args, char* out, uint32_t capacity);

/* folder() kinds; UTF-8 paths without a trailing slash. */
#define CPS_FOLDER_CPS 1u
#define CPS_FOLDER_GAME 2u /* SB/Binaries/Win64 */
#define CPS_FOLDER_PAKS 3u /* SB/Content/Paks */

/* host_flags. */
#define CPS_HOST_PREVIEW 1u /* the preview app: no game, state() and the reflection calls return 0 */

#define CPS_LOG_INFO 0
#define CPS_LOG_WARNING 1
#define CPS_LOG_ERROR 2
#define CPS_LOG_DEBUG 3

/* The game's Unreal Engine 4.26 runtime as CPS found it, for C++ plugins built with the helpers in sdk/cpp/ue:
   ue::init(api->engine(), error). Addresses in the game's memory. */
typedef struct cps_engine {
    uint32_t size;
    uintptr_t objects;           /* GUObjectArray (FUObjectArray) */
    uintptr_t name_pool;         /* FNamePool */
    uintptr_t name_init;         /* FName::FName(const TCHAR*, EFindName) */
    uintptr_t process_event;     /* UObject::ProcessEvent */
    int32_t process_event_index; /* its virtual table slot */
} cps_engine;

/* scan() flags. */
#define CPS_SCAN_RIP 1u    /* the result is the target of a rel32 operand, see rip_disp and rip_length */
#define CPS_SCAN_UNIQUE 2u /* fail when the pattern matches more than once */

/* Options for scan(); null means the first match in the game exe. */
typedef struct cps_scan {
    uint32_t size;
    uint32_t flags;
    const char* module;  /* a loaded module's file name, or null for the game exe */
    int32_t offset;      /* added to the match; with CPS_SCAN_RIP the start of the instruction */
    uint32_t rip_disp;   /* CPS_SCAN_RIP: where the rel32 sits in the instruction, e.g. 3 for "48 8B 05" */
    uint32_t rip_length; /* CPS_SCAN_RIP: the instruction length, e.g. 7 for "48 8B 05" */
    uint32_t index;      /* which match, 0 for the first */
} cps_scan;

/* notify() levels; they colour the notification's edge. */
#define CPS_NOTIFY_INFO 0u
#define CPS_NOTIFY_SUCCESS 1u
#define CPS_NOTIFY_WARNING 2u
#define CPS_NOTIFY_ERROR 3u

/* In the open menu only the text shows; outside it, a card with the title in the corner the player picked. */
typedef struct cps_notification {
    uint32_t size;
    const char* title; /* null or "": the plugin's name */
    const char* text;  /* UTF-8 */
    uint32_t level;    /* CPS_NOTIFY_* */
    float seconds;     /* time on screen before it fades; 0: the player's setting */
} cps_notification;

/* A UFunction call seen through ProcessEvent. */
typedef struct cps_call {
    uint32_t size;
    void* object;   /* UObject* */
    void* function; /* UFunction* */
    void* params;   /* arguments; after the call also the return value and out parameters */
    int skip;       /* set in a pre callback: neither the function nor later pre callbacks run */
} cps_call;

/* The thread that called ProcessEvent, usually the game thread. */
typedef void (*cps_call_fn)(void* user, cps_call* call);

typedef struct cps_api {
    uint32_t size, version;
    const char* build; /* the full CPS version, e.g. "1.0.0-001" */
    uint32_t host_flags;

    /* For mods using cps_get_api; plugins get theirs in cps_plugin_load. Any thread. */
    cps_client (*register_client)(const char* name, const char* version);

    /* Any thread; runs a JSON command (SDK.md) on the next game tick. 0 if it has no "action". */
    int (*send)(cps_client client, const char* json);

    /* Any thread; free the result with free_string. */
    char* (*snapshot)(void);
    void (*free_string)(char* text);

    /* Game thread; ticks run after the core's own work. */
    int (*subscribe)(cps_client client, uint32_t event_mask, cps_event_fn fn, void* user);
    int (*on_game_tick)(cps_client client, cps_tick_fn fn, void* user);

    int (*add_tab)(cps_client client, const cps_tab* tab);
    int (*add_overlay)(cps_client client, const cps_overlay* overlay);

    /* Listed on the Hotkeys page as "<plugin id>/<id>". conflict (optional) reports a clash. */
    int (*add_hotkey)(cps_client client, const cps_hotkey* hotkey, cps_conflict* conflict);
    int (*hotkey_binding)(cps_client client, const char* id, char* out, uint32_t capacity);
    int (*hotkey_conflict)(cps_client client, const char* id, cps_conflict* out);

    void (*log)(cps_client client, int level, const char* text); /* CPS_LOG_*; Logs page and CPS.log */
    void (*toast)(const char* text, int ok);                     /* API 1, untitled; notify() names the plugin */

    /* Game thread only, else 0 or null. kind: 0 EVE, 1 ADAM, 2 LILY, 3 DRONE. Set struct sizes first. */
    int (*state)(cps_state* out);
    void* (*character)(uint32_t kind); /* the actor's UObject* */
    int (*character_info)(uint32_t kind, cps_character_info* out);
    int (*bone_location)(uint32_t kind, const char* bone, float out[3]); /* a body mesh bone or socket, world cm */

    /* Fails while another client or the menu has it; place a shot every tick until CAMERA_LOST. */
    int (*camera_acquire)(cps_client client, uint32_t flags);
    int (*camera_place)(cps_client client, const cps_shot* shot);
    void (*camera_release)(cps_client client); /* cuts straight back to the gameplay view */
    /* Game thread; takes the camera back from whichever client holds it, which gets CAMERA_LOST. */
    void (*camera_interrupt)(cps_client client, const char* reason);

    /* Game thread. Members by name or struct path ("Node.SpringBone.BoneName"); 0 or null if missing. */
    void* (*object)(void* object, const char* property); /* an object property, e.g. a component */
    int (*get_value)(void* object, const char* property, uint32_t type, void* out);
    int (*set_value)(void* object, const char* property, uint32_t type, const void* value);
    int (*get_text)(void* object, const char* property, char* out, uint32_t capacity); /* FName or FString, UTF-8 */
    int (*call_bool)(void* object, const char* function, int* out);                    /* no parameters, returns bool */
    /* class_name is a full path or a bare name such as "CH_P_EVE_01_AnimBP_New_C"; null or 0 if not loaded. */
    void* (*default_object)(const char* class_name);
    int (*find_objects)(const char* class_name, void** out, int capacity); /* live instances; returns the count */

    /* Any thread. While on and the menu is open, keyboard and controller reach the game; the mouse stays. */
    int (*game_input)(cps_client client, int allow);

    /* Commands, queries and services other plugins use. A name or action belongs to whoever adds it first. */
    int (*add_command)(cps_client client, const char* action, cps_command_fn fn, void* user);
    void (*finish_command)(cps_client client, uint32_t id, int ok, const char* message); /* game thread */
    int (*add_query)(cps_client client, const char* name, cps_json_fn fn, void* user);
    char* (*query)(const char* name, const char* args_json); /* null if nobody answers; free with free_string */
    int (*add_snapshot)(cps_client client, cps_json_fn fn, void* user); /* object keys merged into snapshot() */
    int (*provide)(cps_client client, const char* name, const void* service);
    const void* (*service)(const char* name);

    /* Game thread. Plugin events, CPS_EVENT_WARDROBE_APPLIED to CPS_EVENT_POSE_CLEARED; detail is a JSON object. */
    int (*emit)(cps_client client, uint32_t type, const char* character, const char* slot, const char* detail);
    void (*set_activity)(cps_client client, uint32_t flags);
    int (*status)(cps_status* out);
    int (*folder)(cps_client client, uint32_t which, char* out, uint32_t capacity);

    /* The plugin's own folder, CPS/Configs/<Name> (see cps_plugin.config_dir), for "", or a file in it
       such as "settings.json" or "Presets/Wide.json" (folders are created). UTF-8; 0 if it doesn't fit. */
    int (*config_path)(cps_client client, const char* name, char* out, uint32_t capacity);

    /* Files bundled with the plugin (the .cps archive or the plugin's folder), by relative path with /.
       The bytes stay valid until the game exits; null if missing. asset_name lists them, null past the end. */
    const void* (*asset)(cps_client client, const char* path, uint32_t* size);
    const char* (*asset_name)(cps_client client, uint32_t index);

    /* Render thread. Decodes a PNG or JPG into a texture for ui->image; C++ plugins can draw it with
       ((ImTextureData*)image)->GetTexRef(). 0 on failure. */
    uint64_t (*image)(cps_client client, const void* data, uint32_t size, int* width, int* height);
    void (*free_image)(cps_client client, uint64_t image);

    /* Any thread. Opens the menu, on one of your tabs if tab is an id, or closes it with open 0. */
    int (*menu)(cps_client client, int open, const char* tab);

    /* Any thread. Null in the preview; plugins load after CPS has found the engine. */
    const cps_engine* (*engine)(void);

    /* Any thread. While on, a plugin's own window has the mouse and keyboard, as the open menu does. */
    int (*capture_input)(cps_client client, int capture);

    /* Any thread. Finds a byte pattern such as "48 8B 05 ? ? ? ?"; 0 if missing. Cached for every plugin. */
    uintptr_t (*scan)(cps_client client, const char* pattern, const cps_scan* options);

    /* Any thread. Chains a detour in front of native code; it calls on through ((T)*next)(...). 0 on failure. */
    uint32_t (*hook)(cps_client client, void* target, void* detour, void* const** next);

    /* Any thread. pre and post run around a UFunction called through ProcessEvent, by path. 0 on failure. */
    uint32_t (*hook_function)(cps_client client, const char* path, cps_call_fn pre, cps_call_fn post, void* user);

    /* Any thread. Removes a hook from hook() or hook_function(); a plugin that is turned off loses all. */
    int (*unhook)(cps_client client, uint32_t hook);

    /* Game thread. The native exec thunk (UFunction::Func) of a function by path, for hook(); null if missing. */
    void* (*function_native)(const char* path);

    /* Render thread. A texture from width x height RGBA8 pixels, top row first, like image(); 0 on failure. */
    uint64_t (*image_rgba)(cps_client client, const void* rgba, int width, int height);

    /* API 2. Any thread. Queues a notification; 0 without text. */
    int (*notify)(cps_client client, const cps_notification* notification);
    /* API 2. The CPS_PAD_* buttons bound to a hotkey; 0 if it has none. */
    uint32_t (*hotkey_pad)(cps_client client, const char* id);
    /* API 2. Text for an id in the player's language, from the plugin's own Locale/<code>.json files: the
       language CPS shows if the plugin has it, else plugin.toml's "language" (default "en"), else key itself
       (the same pointer, so a plugin can fall back to its own English). A key can also be text in the
       default language. Any thread; the text lives until the plugin unloads. */
    const char* (*translate)(cps_client client, const char* key);
    /* API 2. The language CPS shows, e.g. "en" or "pt-BR". Any thread; the string lives for the session. */
    const char* (*language)(void);
} cps_api;

/* get_value() and set_value() types, with the C type they read and write. */
#define CPS_VALUE_BOOL 1u    /* int */
#define CPS_VALUE_FLOAT 2u   /* float */
#define CPS_VALUE_VECTOR 3u  /* float[3] */
#define CPS_VALUE_ROTATOR 4u /* float[3]: pitch, yaw, roll */
#define CPS_VALUE_INT 5u     /* int32_t */

/* Exported by CPS.dll; null until CPS has started or if it is older than min_version. */
typedef const cps_api* (*cps_get_api_fn)(uint32_t min_version);
#define CPS_GET_API "cps_get_api"

/* What the loader knows about the plugin it loads. Strings are UTF-8 and live until the game exits.
   The loader has already checked plugin.toml: CPS, API and Dear ImGui versions and requirements. */
typedef struct cps_plugin {
    uint32_t size;
    cps_client client; /* pass it to the API calls */
    const char* id;
    const char* version;
    const char* source; /* the .cps file or the plugin's folder */
    /* CPS/Configs/<Name>, everything the plugin stores; <Name> is plugin.toml's name in words joined with
       capitals ("Jiggle Config" gives JiggleConfig). Renaming the plugin moves it. Exists before load. */
    const char* config_dir;
    uint32_t host_flags; /* CPS_HOST_* */
    int bundled;         /* loaded from a .cps */
} cps_plugin;

/* Plugin DLLs export this; return 0 to refuse, after logging why. */
typedef int (*cps_plugin_load_fn)(const cps_api* api, const cps_plugin* plugin);
#define CPS_PLUGIN_LOAD "cps_plugin_load"

/* Optional. Called on the game thread after CPS callbacks stop. Restore changed game state,
   stop/join workers, and release external callbacks before returning. CPS removes its own
   registrations automatically. Legacy DLLs without this export remain mapped after detaching. */
typedef void (*cps_plugin_unload_fn)(void);
#define CPS_PLUGIN_UNLOAD "cps_plugin_unload"

#ifdef __cplusplus
}
#endif
#endif
