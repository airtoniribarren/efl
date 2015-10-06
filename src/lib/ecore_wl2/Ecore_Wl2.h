#ifndef _ECORE_WL2_H_
# define _ECORE_WL2_H_

# include <Eina.h>
# include <Ecore.h>
# include <wayland-client.h>
# include <wayland-cursor.h>
# include <xkbcommon/xkbcommon.h>

# define WL_HIDE_DEPRECATED
# include <wayland-server.h>

# ifdef EAPI
#  undef EAPI
# endif

# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif

/* # ifdef __cplusplus */
/* extern "C" { */
/* # endif */

typedef struct _Ecore_Wl2_Subsurface Ecore_Wl2_Subsurface;

# ifndef _ECORE_WL2_WINDOW_PREDEF
typedef struct _Ecore_Wl2_Window Ecore_Wl2_Window;
# endif

typedef struct _Ecore_Wl2_Display Ecore_Wl2_Display;
typedef struct _Ecore_Wl2_Output Ecore_Wl2_Output;
typedef struct _Ecore_Wl2_Input Ecore_Wl2_Input;

typedef struct _Ecore_Wl2_Global
{
   Eina_Stringshare *interface;
   unsigned int id, version;
} Ecore_Wl2_Global;

typedef struct _Ecore_Wl2_Event_Global
{
   Ecore_Wl2_Display *display;
   Eina_Stringshare *interface;
   unsigned int id, version;
} Ecore_Wl2_Event_Global;

typedef struct _Ecore_Wl2_Event_Focus_In
{
   unsigned int window;
   unsigned int timestamp;
} Ecore_Wl2_Event_Focus_In;

typedef struct _Ecore_Wl2_Event_Focus_Out
{
   unsigned int window;
   unsigned int timestamp;
} Ecore_Wl2_Event_Focus_Out;

typedef struct _Ecore_Wl2_Event_Dnd_Enter
{
   unsigned int win, source, serial;
   char **types;
   int num_types, x, y;
   struct wl_data_offer *offer;
} Ecore_Wl2_Event_Dnd_Enter;

typedef struct _Ecore_Wl2_Event_Dnd_Leave
{
   unsigned int win, source;
} Ecore_Wl2_Event_Dnd_Leave;

typedef struct _Ecore_Wl2_Event_Dnd_Motion
{
   unsigned int win, source, serial;
   int x, y;
} Ecore_Wl2_Event_Dnd_Motion;

typedef struct _Ecore_Wl2_Event_Dnd_Drop
{
   unsigned int win, source;
   int x, y;
} Ecore_Wl2_Event_Dnd_Drop;

typedef struct _Ecore_Wl2_Event_Dnd_End
{
   unsigned int win, source;
} Ecore_Wl2_Event_Dnd_End;

typedef struct _Ecore_Wl2_Event_Data_Source_Cancelled
{
   unsigned int win, source;
} Ecore_Wl2_Event_Data_Source_Cancelled;

typedef struct _Ecore_Wl2_Event_Data_Source_Target
{
   char *type;
} Ecore_Wl2_Event_Data_Source_Target;

typedef struct _Ecore_Wl2_Event_Data_Source_Send
{
   char *type;
   int fd;
} Ecore_Wl2_Event_Data_Source_Send;

typedef struct _Ecore_Wl2_Event_Selection_Data_Ready
{
   char *data;
   int len;
   Eina_Bool done;
} Ecore_Wl2_Event_Selection_Data_Ready;

typedef struct _Ecore_Wl2_Event_Window_Configure
{
   unsigned int win, event_win, edges;
   int x, y, w, h;
} Ecore_Wl2_Event_Window_Configure;

typedef enum _Ecore_Wl2_Window_Type
{
   ECORE_WL2_WINDOW_TYPE_NONE,
   ECORE_WL2_WINDOW_TYPE_TOPLEVEL,
   ECORE_WL2_WINDOW_TYPE_FULLSCREEN,
   ECORE_WL2_WINDOW_TYPE_MAXIMIZED,
   ECORE_WL2_WINDOW_TYPE_TRANSIENT,
   ECORE_WL2_WINDOW_TYPE_MENU,
   ECORE_WL2_WINDOW_TYPE_DND,
   ECORE_WL2_WINDOW_TYPE_CUSTOM,
   ECORE_WL2_WINDOW_TYPE_LAST
} Ecore_Wl2_Window_Type;

EAPI extern int ECORE_WL2_EVENT_GLOBAL_ADDED;
EAPI extern int ECORE_WL2_EVENT_GLOBAL_REMOVED;
EAPI extern int ECORE_WL2_EVENT_FOCUS_IN;
EAPI extern int ECORE_WL2_EVENT_FOCUS_OUT;
EAPI extern int ECORE_WL2_EVENT_DND_ENTER;
EAPI extern int ECORE_WL2_EVENT_DND_LEAVE;
EAPI extern int ECORE_WL2_EVENT_DND_MOTION;
EAPI extern int ECORE_WL2_EVENT_DND_DROP;
EAPI extern int ECORE_WL2_EVENT_DND_END;
EAPI extern int ECORE_WL2_EVENT_DATA_SOURCE_CANCELLED;
EAPI extern int ECORE_WL2_EVENT_DATA_SOURCE_TARGET;
EAPI extern int ECORE_WL2_EVENT_DATA_SOURCE_SEND;
EAPI extern int ECORE_WL2_EVENT_SELECTION_DATA_READY;
EAPI extern int ECORE_WL2_EVENT_WINDOW_CONFIGURE;

/**
 * @file
 * @brief Ecore functions for dealing with the Wayland display protocol
 *
 * @defgroup Ecore_Wl2_Group Ecore_Wl2 - Wayland integration
 * @ingroup Ecore
 *
 * Ecore_Wl2 provides a wrapper and convenience functions for using the
 * Wayland protocol in implementing a window system. Function groups for
 * this part of the library include the following:
 *
 * @li @ref Ecore_Wl2_Init_Group
 * @li @ref Ecore_Wl2_Display_Group
 * @li @ref Ecore_Wl2_Window_Group
 * @li @ref Ecore_Wl2_Subsurface_Group
 */

/**
 * @defgroup Ecore_Wl2_Init_Group Wayland Library Init and Shutdown Functions
 * @ingroup Ecore_Wl2_Group
 *
 * Functions that start and shutdown the Ecore Wl2 Library.
 */

/**
 * Initialize the Ecore_Wl2 library
 *
 * @return  The number of times the library has been initialized without being
 *          shutdown. 0 is returned if an error occurs.
 *
 * @ingroup Ecore_Wl2_Init_Group
 */
EAPI int ecore_wl2_init(void);

/**
 * Shutdown the Ecore_Wl2 Library
 *
 * @return  The number of times the library has been initialized without
 *          being shutdown.
 *
 * @ingroup Ecore_Wl2_Init_Group
 */
EAPI int ecore_wl2_shutdown(void);

/**
 * @defgroup Ecore_Wl2_Display_Group Wayland Library Display Functions
 * @ingroup Ecore_Wl2_Group
 *
 * Functions that deal with creating, connecting, or interacting with
 * Wayland displays
 */

/**
 * Create a new Wayland display
 *
 * @brief This function is typically used to create a new display for
 * use with compositors, or to create a new display for use in nested
 * compositors.
 *
 * @param name The display target name to create. If @c NULL, a default
 *             display name will be assigned.
 * @return The newly created Ecore_Wl2_Display
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI Ecore_Wl2_Display *ecore_wl2_display_create(const char *name);

/**
 * Destroy an existing Wayland display
 *
 * @brief This function is typically used by servers to terminate an
 * existing Wayland display.
 *
 * @param display The display to terminate
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI void ecore_wl2_display_destroy(Ecore_Wl2_Display *display);

/**
 * Connect to an existing Wayland display
 *
 * @brief This function is typically used by clients to connect to an
 * existing Wayland display.
 *
 * @param name The display target name to connect to. If @c NULL, the default
 *             display is assumed.
 *
 * @return The Ecore_Wl2_Display which was connected to
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI Ecore_Wl2_Display *ecore_wl2_display_connect(const char *name);

/**
 * Disconnect an existing Wayland display
 *
 * @brief This function is typically used by clients to disconnect from an
 * existing Wayland display.
 *
 * @param display The display to disconnect from
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI void ecore_wl2_display_disconnect(Ecore_Wl2_Display *display);

/**
 * Terminate a Wayland display's main loop
 *
 * @brief This function is typically used by servers to terminate the
 * Wayland display main loop. This is usually only called when a server
 * encounters an error.
 *
 * @param display The Ecore_Wl2_Display to terminate
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI void ecore_wl2_display_terminate(Ecore_Wl2_Display *display);

/**
 * Retrieve the existing Wayland display
 *
 * @param display The Ecore_Wl2_Display for which to retrieve the existing
 *                Wayland display from
 *
 * @return The wl_display which this Ecore_Wl2_Display is using
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI struct wl_display *ecore_wl2_display_get(Ecore_Wl2_Display *display);

/**
 * Retrieve the wl_shm from a given Ecore_Wl2_Display
 *
 * @param display The Ecore_Wl2_Display for which to retrieve the existing
 *                Wayland shm interface from
 *
 * @return The wl_shm which this Ecore_Wl2_Display is using
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI struct wl_shm *ecore_wl2_display_shm_get(Ecore_Wl2_Display *display);

/**
 * Return an Eina_Iterator that can be used to iterate through globals
 *
 * @param display The Ecore_Wl2_Display for which to return a global iterator
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI Eina_Iterator *ecore_wl2_display_globals_get(Ecore_Wl2_Display *display);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_display_animator_source_set(Ecore_Wl2_Display *display, Ecore_Animator_Source source);

/**
 * Retrieves the size of the current screen.
 *
 * @param display The display to get the screen size of
 * @param w where to return the width. May be NULL. Returns 0 on error.
 * @param h where to return the height. May be NULL. Returns 0 on error.
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI void ecore_wl2_display_screen_size_get(Ecore_Wl2_Display *display, int *w, int *h);

/* TODO: doxy */
EAPI Ecore_Wl2_Window *ecore_wl2_display_window_find(Ecore_Wl2_Display *display, unsigned int id);

/**
 * Retrieves the Wayland Registry used for the current Wayland display.
 *
 * @param display The display to get the registry of
 *
 * @return The current wayland registry, or NULL on error
 *
 * @ingroup Ecore_Wl2_Display_Group
 */
EAPI struct wl_registry *ecore_wl2_display_registry_get(Ecore_Wl2_Display *display);

/**
 * @defgroup Ecore_Wl2_Window_Group Wayland Library Window Functions
 * @ingroup Ecore_Wl2_Group
 *
 * Functions that deal with creating, destroying, or interacting with
 * Wayland windows
 */

/**
 * Create a new Ecore_Wl2_Window
 *
 * @param display The Ecore_Wl2_Display on which to create this new window
 * @param parent The Ecore_Wl2_Window which is the parent of this window
 * @param x Initial x position of window
 * @param y Initial y position of window
 * @param w Initial width of window
 * @param h Initial height of window
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI Ecore_Wl2_Window *ecore_wl2_window_new(Ecore_Wl2_Display *display, Ecore_Wl2_Window *parent, int x, int y, int w, int h);

/**
 * Get the window id associated with an Ecore_Wl2_Window
 *
 * @param window The Ecore_Wl2_Window of which to retrieve the window id
 *
 * @return The id associated with this window
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI int ecore_wl2_window_id_get(Ecore_Wl2_Window *window);

/**
 * Get the wl_surface which belongs to this window
 *
 * @param window The Ecore_Wl2_Window to get the surface of
 *
 * @return The wl_surface associated with this window.
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI struct wl_surface *ecore_wl2_window_surface_get(Ecore_Wl2_Window *window);

/* TODO: doxy */
EAPI int ecore_wl2_window_surface_id_get(Ecore_Wl2_Window *window);

/**
 * Show a given Ecore_Wl2_Window
 *
 * @param window The Ecore_Wl2_Window to show
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_show(Ecore_Wl2_Window *window);

/**
 * Hide a given Ecore_Wl2_Window
 *
 * @param window The Ecore_Wl2_Window to hide
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_hide(Ecore_Wl2_Window *window);

/**
 * Free a given Ecore_Wl2_Window
 *
 * @param window The Ecore_Wl2_Window to free
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_free(Ecore_Wl2_Window *window);

/**
 * Move a given Ecore_Wl2_Window
 *
 * @brief The position requested (@p x, @p y) is not honored by Wayland because
 * Wayland does not allow specific window placement to be set.
 *
 * @param window The Ecore_Wl2_Window which to move
 * @param x Desired x position of window
 * @param y Desired y position of window
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_move(Ecore_Wl2_Window *window, int x, int y);

/**
 * Resize a given Ecore_Wl2_Window
 *
 * @brief The size requested (@p w, @p h) is not honored by Wayland because
 * Wayland does not allow specific window sizes to be set.
 *
 * @param window The Ecore_Wl2_Window which to resize
 * @param w Desired width of window
 * @param h Desired height of window
 * @param location The edge of the window from where the resize should start
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_resize(Ecore_Wl2_Window *window, int w, int h, int location);

/**
 * Raise a given Ecore_Wl2_Window
 *
 * @param window The Ecore_Wl2_Window which to raise
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_raise(Ecore_Wl2_Window *window);

/* TODO: doxy */
EAPI Ecore_Wl2_Window *ecore_wl2_window_find(Ecore_Wl2_Display *display, int id);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_window_alpha_get(Ecore_Wl2_Window *window);

/* TODO: doxy */
EAPI void ecore_wl2_window_alpha_set(Ecore_Wl2_Window *window, Eina_Bool alpha);

/* TODO: doxy */
EAPI void ecore_wl2_window_transparent_set(Ecore_Wl2_Window *window, Eina_Bool transparent);

/**
 * Set the opaque region of the Ecore_Wl2_Window
 *
 * @param win The window
 * @param x The left point of the region.
 * @param y The top point of the region.
 * @param w The width of the region.
 * @param h The height of the region.
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_opaque_region_set(Ecore_Wl2_Window *window, int x, int y, int w, int h);

/**
 * Set the input region of the Ecore_Wl2_Window.
 *
 * To set an empty region, pass width and height as 0.
 *
 * An empty input region means the entire window surface will accept input.
 *
 * @param window The window to set the input region of
 * @param x The left point of the region.
 * @param y The top point of the region.
 * @param w The width of the region.
 * @param h The height of the region.
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_input_region_set(Ecore_Wl2_Window *window, int x, int y, int w, int h);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_window_maximized_get(Ecore_Wl2_Window *window);

/* TODO: doxy */
EAPI void ecore_wl2_window_maximized_set(Ecore_Wl2_Window *window, Eina_Bool maximized);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_window_fullscreen_get(Ecore_Wl2_Window *window);

/* TODO: doxy */
EAPI void ecore_wl2_window_fullscreen_set(Ecore_Wl2_Window *window, Eina_Bool fullscreen);

/* TODO: doxy */
EAPI void ecore_wl2_window_rotation_set(Ecore_Wl2_Window *window, int rotation);

/* TODO: doxy */
EAPI void ecore_wl2_window_title_set(Ecore_Wl2_Window *window, const char *title);

/* TODO: doxy */
EAPI void ecore_wl2_window_class_name_set(Ecore_Wl2_Window *window, const char *class_name);

/* TODO: doxy */
EAPI void ecore_wl2_window_geometry_get(Ecore_Wl2_Window *window, int *x, int *y, int *w, int *h);

/* TODO: doxy */
EAPI void ecore_wl2_window_geometry_set(Ecore_Wl2_Window *window, int x, int y, int w, int h);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_window_iconified_get(Ecore_Wl2_Window *window);

/**
 * Iconify a window
 *
 * @param win The window to iconifiy
 * @param iconified The new iconified state to set
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_iconified_set(Ecore_Wl2_Window *window, Eina_Bool iconified);

/**
 * Retrieves the mouse position of the current window.
 *
 * @param window The window on which to retrieve the mouse position
 * @param x where to return the horizontal position. May be NULL. Returns 0 on error.
 * @param y where to return the vertical position. May be NULL. Returns 0 on error.
 *
 * @ingroup Ecore_Wl2_Window_Group
 */
EAPI void ecore_wl2_window_pointer_xy_get(Ecore_Wl2_Window *window, int *x, int *y);

/* TODO: doxy */
EAPI void ecore_wl2_window_pointer_set(Ecore_Wl2_Window *window, struct wl_surface *surface, int hot_x, int hot_y);

/* TODO: doxy */
EAPI void ecore_wl2_window_cursor_from_name_set(Ecore_Wl2_Window *window, const char *cursor);

/* TODO: doxy */
EAPI void ecore_wl2_window_type_set(Ecore_Wl2_Window *window, Ecore_Wl2_Window_Type type);

/* TODO: doxy */
EAPI Ecore_Wl2_Input *ecore_wl2_window_input_get(Ecore_Wl2_Window *window);

/* TODO: doxy */
EAPI void ecore_wl2_dnd_drag_types_set(Ecore_Wl2_Input *input, const char **types);

/* TODO: doxy */
EAPI void ecore_wl2_dnd_drag_start(Ecore_Wl2_Input *input, Ecore_Wl2_Window *window, Ecore_Wl2_Window *drag_window);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_dnd_drag_get(Ecore_Wl2_Input *input, const char *type);

/* TODO: doxy */
EAPI void ecore_wl2_dnd_drag_end(Ecore_Wl2_Input *input);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_dnd_selection_owner_has(Ecore_Wl2_Input *input);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_dnd_selection_set(Ecore_Wl2_Input *input, const char **types);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_dnd_selection_get(Ecore_Wl2_Input *input, const char *type);

/* TODO: doxy */
EAPI Eina_Bool ecore_wl2_dnd_selection_clear(Ecore_Wl2_Input *input);

/* TODO: doxy */
EAPI void ecore_wl2_input_ungrab(Ecore_Wl2_Input *input);

/**
 * Create and return a new subsurface.
 *
 * Create a new surface (and subsurface interface), with the parent surface
 * being the one associated with the given @param win.
 *
 * The @param win must be visible, otherwise there will be no surface created
 * for it yet.
 *
 * @return the allocated and initialized Ecore_Wl2_Subsurface object, or
 * NULL on failure
 *
 * @ingroup Ecore_Wl2_Subsurface_Group
 */
EAPI Ecore_Wl2_Subsurface *ecore_wl2_subsurface_new(Ecore_Wl2_Window *window);

/**
 * Destroy the given subsurface, as well as the surface associated with it.
 *
 * @param ess the subsurface
 *
 * @ingroup Ecore_Wl2_Subsurface_Group
 */
EAPI void ecore_wl2_subsurface_del(Ecore_Wl2_Subsurface *subsurface);

/* # ifdef __cplusplus */
/* } */
/* # endif */

# undef EAPI
# define EAPI

#endif
