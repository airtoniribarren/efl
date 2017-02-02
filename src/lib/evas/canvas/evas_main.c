#define EVAS_CANVAS_BETA
#define EFL_INPUT_EVENT_PROTECTED
#include "evas_common_private.h"
#include "evas_private.h"
//#include "evas_cs.h"
#ifdef EVAS_CSERVE2
#include "evas_cs2_private.h"
#endif

#include "evas_image_private.h"
#include "evas_polygon_private.h"
#include "evas_vg_private.h"

#include <Ecore.h>

#define EFL_INTERNAL_UNSTABLE
#include "interfaces/efl_common_internal.h"

#define MY_CLASS EVAS_CANVAS_CLASS

#ifdef LKDEBUG
EAPI Eina_Bool lockdebug = EINA_FALSE;
EAPI int lockmax = 0;
#endif

static int _evas_init_count = 0;
int _evas_log_dom_global = -1;

EAPI int
evas_init(void)
{
   if (++_evas_init_count != 1)
     return _evas_init_count;

#ifdef LKDEBUG
   if (getenv("EVAS_LOCK_DEBUG"))
      {
         lockdebug = EINA_TRUE;
         lockmax = atoi(getenv("EVAS_LOCK_DEBUG"));
      }
#endif

#ifdef HAVE_EVIL
   if (!evil_init())
     return --_evas_init_count;
#endif

   if (!eina_init())
     goto shutdown_evil;

   if (!eet_init())
     goto shutdown_eet;

   _evas_log_dom_global = eina_log_domain_register
     ("evas_main", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_log_dom_global < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        goto shutdown_eina;
     }

   efl_object_init();

#ifdef BUILD_LOADER_EET
   eet_init();
#endif

   evas_module_init();
   if (!evas_async_events_init())
     goto shutdown_module;
#ifdef EVAS_CSERVE2
   int cs2 = 0;
   {
      const char *env;
      env = getenv("EVAS_CSERVE2");
      if (env && atoi(env))
        {
           cs2 = evas_cserve2_init();
           if (!cs2) goto shutdown_async_events;
        }
   }
#endif
   _evas_preload_thread_init();
   evas_filter_init();

   if (!evas_thread_init())
     goto shutdown_filter;

   eina_log_timing(_evas_log_dom_global,
		   EINA_LOG_STATE_STOP,
		   EINA_LOG_STATE_INIT);

   return _evas_init_count;

 shutdown_filter:
   evas_filter_shutdown();
   _evas_preload_thread_shutdown();
#ifdef EVAS_CSERVE2
   if (cs2) evas_cserve2_shutdown();
 shutdown_async_events:
#endif
   evas_async_events_shutdown();
 shutdown_module:
   evas_module_shutdown();
#ifdef BUILD_LOADER_EET
   eet_shutdown();
#endif
   efl_object_shutdown();
   eina_log_domain_unregister(_evas_log_dom_global);
 shutdown_eet:
   eet_shutdown();
 shutdown_eina:
   eina_shutdown();
 shutdown_evil:
#ifdef HAVE_EVIL
   evil_shutdown();
#endif

   return --_evas_init_count;
}

EAPI int
evas_shutdown(void)
{
   if (_evas_init_count <= 0)
     {
        EINA_LOG_ERR("Init count not greater than 0 in shutdown.");
        return 0;
     }
   if (--_evas_init_count != 0)
     return _evas_init_count;

   eina_log_timing(_evas_log_dom_global,
                   EINA_LOG_STATE_START,
                   EINA_LOG_STATE_SHUTDOWN);

#ifdef EVAS_CSERVE2
   if (evas_cserve2_use_get())
     evas_cserve2_shutdown();
#endif

   evas_font_path_global_clear();
   eina_cow_del(evas_object_proxy_cow);
   eina_cow_del(evas_object_map_cow);
   eina_cow_del(evas_object_state_cow);
   evas_object_state_cow = NULL;
   evas_object_map_cow = NULL;
   evas_object_proxy_cow = NULL;

   eina_cow_del(evas_object_image_pixels_cow);
   eina_cow_del(evas_object_image_load_opts_cow);
   eina_cow_del(evas_object_image_state_cow);
   evas_object_image_pixels_cow = NULL;
   evas_object_image_load_opts_cow = NULL;
   evas_object_image_state_cow = NULL;

   evas_filter_shutdown();
   eina_cow_del(evas_object_filter_cow);
   eina_cow_del(evas_object_mask_cow);
   evas_object_filter_cow = NULL;
   evas_object_mask_cow = NULL;

   evas_thread_shutdown();
   _evas_preload_thread_shutdown();
   evas_async_events_shutdown();
   evas_module_shutdown();

#ifdef BUILD_LOADER_EET
   eet_shutdown();
#endif
   efl_object_shutdown();

   eina_log_domain_unregister(_evas_log_dom_global);

   eet_shutdown();

   eina_shutdown();
#ifdef HAVE_EVIL
   evil_shutdown();
#endif

   return _evas_init_count;
}


EAPI Evas *
evas_new(void)
{
   Evas_Object *eo_obj = efl_add(EVAS_CANVAS_CLASS, NULL);
   return eo_obj;
}

static void
_evas_key_mask_free(void *data)
{
   free(data);
}

EOLIAN static Eo *
_evas_canvas_efl_object_constructor(Eo *eo_obj, Evas_Public_Data *e)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   e->evas = eo_obj;
   e->output.render_method = RENDER_METHOD_INVALID;
   e->viewport.w = 1;
   e->viewport.h = 1;
   e->framespace.x = 0;
   e->framespace.y = 0;
   e->framespace.w = 0;
   e->framespace.h = 0;
   e->hinting = EVAS_FONT_HINTING_BYTECODE;
   e->name_hash = eina_hash_string_superfast_new(NULL);
   eina_clist_init(&e->calc_list);
   eina_clist_init(&e->calc_done);

#define EVAS_ARRAY_SET(E, Array) \
   eina_array_step_set(&E->Array, sizeof (E->Array), \
                       ((1024 * sizeof (void*)) - sizeof (E->Array)) / sizeof (void*));

   EVAS_ARRAY_SET(e, delete_objects);
   EVAS_ARRAY_SET(e, restack_objects);
   EVAS_ARRAY_SET(e, render_objects);
   EVAS_ARRAY_SET(e, pending_objects);
   EVAS_ARRAY_SET(e, obscuring_objects);
   EVAS_ARRAY_SET(e, temporary_objects);
   EVAS_ARRAY_SET(e, snapshot_objects);
   EVAS_ARRAY_SET(e, clip_changes);
   EVAS_ARRAY_SET(e, scie_unref_queue);
   EVAS_ARRAY_SET(e, image_unref_queue);
   EVAS_ARRAY_SET(e, glyph_unref_queue);
   EVAS_ARRAY_SET(e, texts_unref_queue);

   e->active_objects.version = EINA_ARRAY_VERSION;
   eina_inarray_step_set(&e->active_objects,
                         sizeof(Eina_Inarray),
                         sizeof(Evas_Active_Entry),
                         256);

#undef EVAS_ARRAY_SET
   eina_lock_new(&(e->lock_objects));
   eina_spinlock_new(&(e->render.lock));
   eina_spinlock_new(&(e->post_render.lock));

   _evas_canvas_event_init(eo_obj, e);

   e->focused_objects = eina_hash_pointer_new(NULL);
   e->locks.masks = eina_hash_pointer_new(_evas_key_mask_free);
   e->modifiers.masks = eina_hash_pointer_new(_evas_key_mask_free);
   e->locks.e = e->modifiers.e = e;

   return eo_obj;
}

EAPI void
evas_free(Evas *eo_e)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();
   evas_sync(eo_e);
   efl_unref(eo_e);
}

EOLIAN static void
_evas_canvas_efl_object_destructor(Eo *eo_e, Evas_Public_Data *e)
{
   Eina_Rectangle *r;
   Evas_Coord_Touch_Point *touch_point;
   Evas_Post_Render_Job *job;
   Evas_Layer *lay;
   Evas_Out *evo;
   int i;
   Eina_Bool del;
   Evas_Pointer_Data *pdata;

   evas_canvas_async_block(e);
   if (e->walking_list == 0) evas_render_idle_flush(eo_e);

   if (e->walking_list > 0) return;

   evas_render_idle_flush(eo_e);

   _evas_post_event_callback_free(eo_e);
   _evas_canvas_event_shutdown(eo_e, e);

   del = EINA_TRUE;
   e->walking_list++;
   e->cleanup = 1;
   while (del)
     {
        del = EINA_FALSE;
        EINA_INLIST_FOREACH(e->layers, lay)
          {
             Eo *eo_obj;
             Evas_Object_Protected_Data *o;
             Eina_List *unrefs = NULL;

             evas_layer_pre_free(lay);

             EINA_INLIST_FOREACH(lay->objects, o)
               {
                  if (!o->delete_me)
                    {
                       if ((o->ref > 0) || (efl_ref_get(o->object) > 0))
                         {
                            ERR("obj(%p, %s) ref count(%d) is bigger than 0. This object couldn't be deleted", o, o->type, efl_ref_get(o->object));
                            continue;
                         }
                       else
                         {
                            unrefs = eina_list_append(unrefs, o->object);
                         }
                       del = EINA_TRUE;
                    }
               }
             EINA_LIST_FREE(unrefs, eo_obj)
               {
                  ERR("Killing Zombie Object [%p] ref=%i:%i\n", eo_obj, efl_ref_get(eo_obj), ___efl_ref2_get(eo_obj));
                  ___efl_ref2_reset(eo_obj);
                  while (efl_ref_get(eo_obj) > 1) efl_unref(eo_obj);
                  while (efl_ref_get(eo_obj) < 1) efl_ref(eo_obj);
                  efl_del(eo_obj);
               }
          }
     }
   EINA_INLIST_FOREACH(e->layers, lay)
     evas_layer_free_objects(lay);
   evas_layer_clean(eo_e);

   e->walking_list--;

   evas_font_path_clear(eo_e);

   if (e->name_hash) eina_hash_free(e->name_hash);
   e->name_hash = NULL;

   EINA_LIST_FREE(e->damages, r)
      eina_rectangle_free(r);
   EINA_LIST_FREE(e->obscures, r)
      eina_rectangle_free(r);

   evas_fonts_zero_free(eo_e);

   evas_event_callback_all_del(eo_e);
   evas_event_callback_cleanup(eo_e);

   EINA_LIST_FREE(e->outputs, evo) efl_del(evo);

   if (e->engine.func)
     {
        e->engine.func->ector_destroy(e->engine.data.output,
                                      e->engine.ector);
        e->engine.func->output_free(e->engine.data.output);
        e->engine.func->info_free(eo_e, e->engine.info);
     }
   if (e->common_init)
     {
        e->common_init = 0;
        evas_common_shutdown();
     }

   for (i = 0; i < e->modifiers.mod.count; i++)
     free(e->modifiers.mod.list[i]);
   if (e->modifiers.mod.list) free(e->modifiers.mod.list);

   for (i = 0; i < e->locks.lock.count; i++)
     free(e->locks.lock.list[i]);
   if (e->locks.lock.list) free(e->locks.lock.list);

   if (e->engine.module) evas_module_unref(e->engine.module);

   eina_array_flush(&e->delete_objects);
   eina_inarray_flush(&e->active_objects);
   eina_array_flush(&e->restack_objects);
   eina_array_flush(&e->render_objects);
   eina_array_flush(&e->pending_objects);
   eina_array_flush(&e->obscuring_objects);
   eina_array_flush(&e->temporary_objects);
   eina_array_flush(&e->snapshot_objects);
   eina_array_flush(&e->clip_changes);
   eina_array_flush(&e->scie_unref_queue);
   eina_array_flush(&e->image_unref_queue);
   eina_array_flush(&e->glyph_unref_queue);
   eina_array_flush(&e->texts_unref_queue);
   eina_hash_free(e->focused_objects);

   EINA_INLIST_FREE(e->post_render.jobs, job)
     {
        e->post_render.jobs = (Evas_Post_Render_Job *)
              eina_inlist_remove(EINA_INLIST_GET(e->post_render.jobs), EINA_INLIST_GET(job));
        free(job);
     }

   EINA_LIST_FREE(e->touch_points, touch_point)
     free(touch_point);

   _evas_device_cleanup(eo_e);
   e->focused_by = eina_list_free(e->focused_by);

   EINA_LIST_FREE(e->pointers, pdata)
     {
        eina_list_free(pdata->object.in);
        free(pdata);
     }

   eina_lock_free(&(e->lock_objects));
   eina_spinlock_free(&(e->render.lock));
   eina_spinlock_free(&(e->post_render.lock));
   eina_hash_free(e->locks.masks);
   eina_hash_free(e->modifiers.masks);

   e->magic = 0;
   efl_destructor(efl_super(eo_e, MY_CLASS));
}

EOLIAN static Evas_Engine_Info*
_evas_canvas_engine_info_get(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e)
{
   Evas_Engine_Info *info;

   if (!e->engine.info) return NULL;

   info = e->engine.info;
   ((Evas_Public_Data *)e)->engine.info_magic = info->magic;

   return info;
}

EOLIAN static Eina_Bool
_evas_canvas_engine_info_set(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, Evas_Engine_Info *info)
{
   if (!info) return EINA_FALSE;
   if (info != e->engine.info) return EINA_FALSE;
   if (info->magic != e->engine.info_magic) return EINA_FALSE;

   evas_canvas_async_block(e);

   if (e->engine.data.output)
     {
        if (e->engine.func->update)
          {
             e->engine.func->update(e->engine.data.output, info, e->output.w, e->output.h);
          }
        else
          {
             // For engine who do not provide an update function
             e->engine.func->output_free(e->engine.data.output);

             goto setup;
          }
     }
   else
     {
        if (!e->common_init)
          {
             e->common_init = 1;
             evas_common_init();
          }

     setup:
        e->engine.data.output = e->engine.func->setup(info, e->output.w, e->output.h);
     }

   return !!e->engine.data.output;
}

EOLIAN static Evas_Coord
_evas_canvas_coord_screen_x_to_world(const Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, int x)
{
   if (e->output.w == e->viewport.w) return e->viewport.x + x;
   else return (long long)e->viewport.x + (((long long)x * (long long)e->viewport.w) / (long long)e->output.w);
}

EOLIAN static Evas_Coord
_evas_canvas_coord_screen_y_to_world(const Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, int y)
{
   if (e->output.h == e->viewport.h) return e->viewport.y + y;
   else return (long long)e->viewport.y + (((long long)y * (long long)e->viewport.h) / (long long)e->output.h);
}

EOLIAN static int
_evas_canvas_coord_world_x_to_screen(const Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, Evas_Coord x)
{
   if (e->output.w == e->viewport.w) return x - e->viewport.x;
   else return (int)((((long long)x - (long long)e->viewport.x) * (long long)e->output.w) /  (long long)e->viewport.w);
}

EOLIAN static int
_evas_canvas_coord_world_y_to_screen(const Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, Evas_Coord y)
{
   if (e->output.h == e->viewport.h) return y - e->viewport.y;
   else return (int)((((long long)y - (long long)e->viewport.y) * (long long)e->output.h) /  (long long)e->viewport.h);
}

EOLIAN static Efl_Input_Device *
_evas_canvas_default_device_get(Eo *eo_e EINA_UNUSED,
                                Evas_Public_Data *e,
                                Efl_Input_Device_Class klass)
{
   if (klass == EFL_INPUT_DEVICE_CLASS_SEAT)
     return e->default_seat;
   if (klass == EFL_INPUT_DEVICE_CLASS_MOUSE)
     return e->default_mouse;
   if (klass == EFL_INPUT_DEVICE_CLASS_KEYBOARD)
     return e->default_keyboard;
   return NULL;
}

EAPI int
evas_render_method_lookup(const char *name)
{
   Evas_Module *em;

   if (!name) return RENDER_METHOD_INVALID;
   /* search on the engines list for the name */
   em = evas_module_find_type(EVAS_MODULE_TYPE_ENGINE, name);
   if (!em) return RENDER_METHOD_INVALID;

   return em->id_engine;
}

EAPI Eina_List *
evas_render_method_list(void)
{
   return evas_module_engine_list();
}

EAPI void
evas_render_method_list_free(Eina_List *list)
{
   const char *s;

   EINA_LIST_FREE(list, s) eina_stringshare_del(s);
}

EAPI Eina_Bool
evas_object_image_extension_can_load_get(const char *file)
{
   const char *tmp;
   Eina_Bool result;

   tmp = eina_stringshare_add(file);
   result = evas_common_extension_can_load_get(tmp);
   eina_stringshare_del(tmp);

   return result;
}

EAPI Eina_Bool
evas_object_image_extension_can_load_fast_get(const char *file)
{
   return evas_common_extension_can_load_get(file);
}

EOLIAN static void
_evas_canvas_pointer_output_xy_by_device_get(Eo *eo_e EINA_UNUSED,
                                             Evas_Public_Data *e,
                                             Efl_Input_Device *dev,
                                             int *x, int *y)
{
   Evas_Pointer_Data *pdata = _evas_pointer_data_by_device_get(e, dev);

   if (!pdata)
     {
        if (x) *x = 0;
        if (y) *y = 0;
     }
   else
     {
        if (x) *x = pdata->x;
        if (y) *y = pdata->y;
     }

}

EOLIAN static void
_evas_canvas_pointer_canvas_xy_by_device_get(Eo *eo_e EINA_UNUSED,
                                             Evas_Public_Data *e,
                                             Efl_Input_Device *dev,
                                             int *x, int *y)
{
   Evas_Pointer_Data *pdata = _evas_pointer_data_by_device_get(e, dev);

   if (!pdata)
     {
        if (x) *x = 0;
        if (y) *y = 0;
     }
   else
     {
        if (x) *x = pdata->x;
        if (y) *y = pdata->y;
     }
}

EOLIAN static unsigned int
_evas_canvas_pointer_button_down_mask_by_device_get(Eo *eo_e EINA_UNUSED,
                                                    Evas_Public_Data *e,
                                                    Efl_Input_Device *dev)
{
   Evas_Pointer_Data *pdata = _evas_pointer_data_by_device_get(e, dev);
   if (!pdata) return 0;
   return pdata->button;
}

EOLIAN static Eina_Bool
_evas_canvas_pointer_inside_by_device_get(Eo *eo_e EINA_UNUSED,
                                          Evas_Public_Data *e,
                                          Efl_Input_Device *dev)
{
   Evas_Pointer_Data *pdata = _evas_pointer_data_by_device_get(e, dev);
   if (!pdata) return EINA_FALSE;
   return pdata->inside;
}

EOLIAN static void
_evas_canvas_pointer_output_xy_get(Eo *eo_e, Evas_Public_Data *e, int *x, int *y)
{
   return _evas_canvas_pointer_output_xy_by_device_get(eo_e, e, NULL, x, y);
}

EOLIAN static void
_evas_canvas_pointer_canvas_xy_get(Eo *eo_e, Evas_Public_Data *e, Evas_Coord *x, Evas_Coord *y)
{
   return _evas_canvas_pointer_canvas_xy_by_device_get(eo_e, e, NULL, x, y);
}

EOLIAN static unsigned int
_evas_canvas_pointer_button_down_mask_get(Eo *eo_e, Evas_Public_Data *e)
{
   return _evas_canvas_pointer_button_down_mask_by_device_get(eo_e, e, NULL);
}

EOLIAN static Eina_Bool
_evas_canvas_pointer_inside_get(Eo *eo_e, Evas_Public_Data *e)
{
   return _evas_canvas_pointer_inside_by_device_get(eo_e, e, NULL);
}

EOLIAN static void
_evas_canvas_data_attach_set(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, void *data)
{
   e->attach_data = data;
}

EOLIAN static void*
_evas_canvas_data_attach_get(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e)
{
   return e->attach_data;
}

static void
_evas_canvas_focus_inout_dispatch(Eo *eo_e, Efl_Input_Device *seat,
                                  Eina_Bool in)
{
   Efl_Input_Focus_Data *ev_data;
   Efl_Input_Focus *evt;

   evt = efl_input_instance_get(EFL_INPUT_FOCUS_CLASS, eo_e, (void **) &ev_data);
   if (!evt) return;

   ev_data->device = efl_ref(seat);
   ev_data->timestamp = time(NULL);
   efl_event_callback_call(eo_e,
                           in ? EFL_EVENT_FOCUS_IN : EFL_EVENT_FOCUS_OUT,
                           evt);
   efl_del(evt);
}

EOLIAN static void
_evas_canvas_seat_focus_in(Eo *eo_e, Evas_Public_Data *e,
                           Efl_Input_Device *seat)
{
   if (!seat) seat = e->default_seat;
   if (!seat || efl_input_device_type_get(seat) != EFL_INPUT_DEVICE_CLASS_SEAT) return;
   _evas_canvas_focus_inout_dispatch(eo_e, seat, EINA_TRUE);
}

EOLIAN static void
_evas_canvas_seat_focus_out(Eo *eo_e, Evas_Public_Data *e,
                            Efl_Input_Device *seat)
{
   if (!seat) seat = e->default_seat;
   if (!seat || efl_input_device_type_get(seat) != EFL_INPUT_DEVICE_CLASS_SEAT) return;
   _evas_canvas_focus_inout_dispatch(eo_e, seat, EINA_FALSE);
}

EOLIAN static void
_evas_canvas_focus_in(Eo *eo_e, Evas_Public_Data *e)
{
   _evas_canvas_seat_focus_in(eo_e, e, NULL);
}

EOLIAN static void
_evas_canvas_focus_out(Eo *eo_e, Evas_Public_Data *e)
{
   _evas_canvas_seat_focus_out(eo_e, e, NULL);
}

EOLIAN static Eina_Bool
_evas_canvas_seat_focus_state_get(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e,
                                  Efl_Input_Device *seat)
{
   if (!seat) seat = e->default_seat;
   return eina_list_data_find(e->focused_by, seat) ? EINA_TRUE : EINA_FALSE;
}

EOLIAN static Eina_Bool
_evas_canvas_focus_state_get(Eo *eo_e, Evas_Public_Data *e)
{
   return _evas_canvas_seat_focus_state_get(eo_e, e, NULL);
}

EOLIAN static Eina_Bool
_evas_canvas_changed_get(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e)
{
   return e->changed;
}

EOLIAN static void
_evas_canvas_nochange_push(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e)
{
   e->nochange++;
}

EOLIAN static void
_evas_canvas_nochange_pop(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e)
{
   e->nochange--;
}

void
_evas_walk(Evas_Public_Data *e)
{
   e->walking_list++;
}

void
_evas_unwalk(Evas_Public_Data *e)
{
   e->walking_list--;
   if ((e->walking_list == 0) && (e->delete_me)) evas_free(e->evas);
}

EAPI const char *
evas_load_error_str(Evas_Load_Error error)
{
   switch (error)
     {
      case EVAS_LOAD_ERROR_NONE:
	 return "No error on load";
      case EVAS_LOAD_ERROR_GENERIC:
	 return "A non-specific error occurred";
      case EVAS_LOAD_ERROR_DOES_NOT_EXIST:
	 return "File (or file path) does not exist";
      case EVAS_LOAD_ERROR_PERMISSION_DENIED:
	 return "Permission deinied to an existing file (or path)";
      case EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED:
	 return "Allocation of resources failure prevented load";
      case EVAS_LOAD_ERROR_CORRUPT_FILE:
	 return "File corrupt (but was detected as a known format)";
      case EVAS_LOAD_ERROR_UNKNOWN_FORMAT:
	 return "File is not a known format";
      default:
	 return "Unknown error";
     }
}

EAPI void
evas_color_hsv_to_rgb(float h, float s, float v, int *r, int *g, int *b)
{
   evas_common_convert_color_hsv_to_rgb(h, s, v, r, g, b);
}

EAPI void
evas_color_rgb_to_hsv(int r, int g, int b, float *h, float *s, float *v)
{
   evas_common_convert_color_rgb_to_hsv(r, g, b, h, s, v);
}

EAPI void
evas_color_argb_premul(int a, int *r, int *g, int *b)
{
   evas_common_convert_color_argb_premul(a, r, g, b);
}

EAPI void
evas_color_argb_unpremul(int a, int *r, int *g, int *b)
{
   evas_common_convert_color_argb_unpremul(a, r, g, b);
}

EAPI void
evas_data_argb_premul(unsigned int *data, unsigned int len)
{
   if (!data || (len < 1)) return;
   evas_common_convert_argb_premul(data, len);
}

EAPI void
evas_data_argb_unpremul(unsigned int *data, unsigned int len)
{
   if (!data || (len < 1)) return;
   evas_common_convert_argb_unpremul(data, len);
}

EOLIAN static Eo *
_evas_canvas_efl_object_provider_find(Eo *eo_e, Evas_Public_Data *e EINA_UNUSED,
                                   const Efl_Class *klass)
{
   if (klass == EVAS_CANVAS_CLASS)
     return eo_e;
   else if (klass == EFL_LOOP_CLASS)
     return ecore_main_loop_get();
   return efl_provider_find(efl_super(eo_e, MY_CLASS), klass);
}

EOLIAN static Efl_Loop *
_evas_canvas_efl_loop_user_loop_get(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e EINA_UNUSED)
{
   return ecore_main_loop_get();
}

Ector_Surface *
evas_ector_get(Evas_Public_Data *e)
{
   if (!e->engine.ector)
     e->engine.ector = e->engine.func->ector_create(e->engine.data.output);
   return e->engine.ector;
}

EAPI void
evas_language_reinit(void)
{
   evas_common_language_reinit();
}

static void
_image_data_unset(Evas_Object_Protected_Data *obj, Eina_List **list)
{
   if (obj->is_smart)
     {
        Evas_Object_Protected_Data *obj2;

        EINA_INLIST_FOREACH(evas_object_smart_members_get_direct(obj->object), obj2)
          _image_data_unset(obj2, list);
        return;
     }
#define CHECK(TYPE, STRUCT, FREE) \
   if (efl_isa(obj->object, TYPE))\
     {\
        STRUCT *data = efl_data_scope_get(obj->object, TYPE);\
        FREE; \
        data->engine_data = NULL;\
     }
   CHECK(EFL_CANVAS_IMAGE_INTERNAL_CLASS, Evas_Image_Data,
         ENFN->image_free(ENDT, data->engine_data))
   else CHECK(EVAS_VG_CLASS, Evas_VG_Data,
        obj->layer->evas->engine.func->ector_free(data->engine_data))
   else CHECK(EFL_CANVAS_POLYGON_CLASS, Efl_Canvas_Polygon_Data,
        data->engine_data =
          obj->layer->evas->engine.func->polygon_points_clear(obj->layer->evas->engine.data.output,
                                                              data->engine_data))
   else CHECK(EVAS_CANVAS3D_TEXTURE_CLASS, Evas_Canvas3D_Texture_Data,
        if (obj->layer->evas->engine.func->texture_free)
          obj->layer->evas->engine.func->texture_free(obj->layer->evas->engine.data.output, data->engine_data))
   else return;
#undef CHECK
   evas_object_ref(obj->object);
   *list = eina_list_append(*list, obj->object);
}

EAPI Eina_List *
_evas_canvas_image_data_unset(Evas *eo_e)
{
   Evas_Public_Data *e = efl_data_scope_get(eo_e, MY_CLASS);
   Evas_Layer *lay;
   Eina_List *list = NULL;

   EINA_INLIST_FOREACH(e->layers, lay)
     {
        Evas_Object_Protected_Data *o;
        EINA_INLIST_FOREACH(lay->objects, o)
          {
             if (!o->delete_me)
               _image_data_unset(o, &list);
          }
     }
   return list;
}

static void
_image_image_data_regenerate(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, Evas_Image_Data *data)
{
   unsigned int orient = data->cur->orient;

   _evas_image_load(eo_obj, obj, data);
   EINA_COW_IMAGE_STATE_WRITE_BEGIN(data, state_write)
     {
        state_write->has_alpha = !state_write->has_alpha;
        state_write->orient = -1;
     }
   EINA_COW_IMAGE_STATE_WRITE_END(data, state_write);
   evas_object_image_alpha_set(eo_obj, !data->cur->has_alpha);
   _evas_image_orientation_set(eo_obj, data, orient);
}

static void
_image_data_regenerate(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj;

   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   evas_object_change(eo_obj, obj);
#define CHECK(TYPE, STRUCT, REGEN) \
   if (efl_isa(eo_obj, TYPE))\
     {\
        STRUCT *data = efl_data_scope_get(eo_obj, TYPE);\
        REGEN; \
     }
   CHECK(EFL_CANVAS_IMAGE_INTERNAL_CLASS, Evas_Image_Data, _image_image_data_regenerate(eo_obj, obj, data))
   else CHECK(EFL_CANVAS_IMAGE_CLASS, Evas_Image_Data, _image_image_data_regenerate(eo_obj, obj, data))
   else CHECK(EFL_CANVAS_SCENE3D_CLASS, Evas_Image_Data, _image_image_data_regenerate(eo_obj, obj, data))
   //else CHECK(EVAS_VG_CLASS, Evas_VG_Data,)
   //else CHECK(EFL_CANVAS_POLYGON_CLASS, Efl_Canvas_Polygon_Data,)
   //else CHECK(EVAS_CANVAS3D_TEXTURE_CLASS, Evas_Canvas3D_Texture_Data,
}

EAPI void
_evas_canvas_image_data_regenerate(Eina_List *list)
{
   Evas_Object *eo_obj;

   EINA_LIST_FREE(list, eo_obj)
     {
        _image_data_regenerate(eo_obj);
        evas_object_unref(eo_obj);
     }
}

/* Legacy deprecated functions */

EAPI void
evas_output_framespace_set(Evas *eo_e, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   if ((x == e->framespace.x) && (y == e->framespace.y) &&
       (w == e->framespace.w) && (h == e->framespace.h)) return;
   evas_canvas_async_block(e);
   e->framespace.x = x;
   e->framespace.y = y;
   e->framespace.w = w;
   e->framespace.h = h;
   e->framespace.changed = 1;
   e->output_validity++;
   e->changed = 1;
}

EAPI void
evas_output_framespace_get(const Evas *eo_e, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   if (x) *x = e->framespace.x;
   if (y) *y = e->framespace.y;
   if (w) *w = e->framespace.w;
   if (h) *h = e->framespace.h;
}

EAPI void
evas_output_method_set(Evas *eo_e, int render_method)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   Evas_Module *em;

   /* if our engine to set it to is invalid - abort */
   if (render_method == RENDER_METHOD_INVALID) return;
   /* if the engine is already set up - abort */
   if (e->output.render_method != RENDER_METHOD_INVALID) return;
   /* Request the right engine. */
   em = evas_module_engine_get(render_method);
   if (!em) return;
   if (em->id_engine != render_method) return;
   if (!evas_module_load(em)) return;

   evas_canvas_async_block(e);
   /* set the correct render */
   e->output.render_method = render_method;
   e->engine.func = (em->functions);
   evas_module_use(em);
   if (e->engine.module) evas_module_unref(e->engine.module);
   e->engine.module = em;
   evas_module_ref(em);
   /* get the engine info struct */
   if (e->engine.func->info) e->engine.info = e->engine.func->info(eo_e);

   // Wayland already handles seats.
   if (em->definition &&
       (eina_streq(em->definition->name, "wayland_shm") ||
           eina_streq(em->definition->name, "wayland_egl") ||
           eina_streq(em->definition->name, "drm") ||
           eina_streq(em->definition->name, "gl_drm")))
     return;

   e->default_seat = evas_device_add_full(eo_e, "default", "The default seat",
                                          NULL, NULL, EVAS_DEVICE_CLASS_SEAT,
                                          EVAS_DEVICE_SUBCLASS_NONE);
   e->default_mouse = evas_device_add_full(eo_e, "keyboard",
                                           "The default mouse",
                                           e->default_seat, NULL,
                                           EVAS_DEVICE_CLASS_MOUSE,
                                           EVAS_DEVICE_SUBCLASS_NONE);
   e->default_keyboard = evas_device_add_full(eo_e, "keyboard",
                                              "The default keyboard",
                                              e->default_seat, NULL,
                                              EVAS_DEVICE_CLASS_KEYBOARD,
                                              EVAS_DEVICE_SUBCLASS_NONE);
}

EAPI int
evas_output_method_get(const Evas *eo_e)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return RENDER_METHOD_INVALID;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   return e->output.render_method;
}

EAPI void
evas_output_size_set(Evas *eo_e, int w, int h)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   if ((w == e->output.w) && (h == e->output.h)) return;
   if (w < 1) w = 1;
   if (h < 1) h = 1;

   evas_canvas_async_block(e);
   e->output.w = w;
   e->output.h = h;
   e->output.changed = 1;
   e->output_validity++;
   e->changed = 1;
   evas_render_invalidate(eo_e);
}

EAPI void
evas_output_size_get(const Evas *eo_e, int *w, int *h)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   if (w) *w = e->output.w;
   if (h) *h = e->output.h;
}

EAPI void
evas_output_viewport_set(Evas *eo_e, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   if ((x == e->viewport.x) && (y == e->viewport.y) &&
       (w == e->viewport.w) && (h == e->viewport.h)) return;
   if (w <= 0) return;
   if (h <= 0) return;
   if ((x != 0) || (y != 0))
     {
	ERR("Compat error. viewport x,y != 0,0 not supported");
	x = 0;
	y = 0;
     }
   evas_canvas_async_block(e);
   e->viewport.x = x;
   e->viewport.y = y;
   e->viewport.w = w;
   e->viewport.h = h;
   e->viewport.changed = 1;
   e->output_validity++;
   e->changed = 1;
   evas_event_callback_call(e->evas, EVAS_CALLBACK_CANVAS_VIEWPORT_RESIZE, NULL);
}

EAPI void
evas_output_viewport_get(const Evas *eo_e, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   if (x) *x = e->viewport.x;
   if (y) *y = e->viewport.y;
   if (w) *w = e->viewport.w;
   if (h) *h = e->viewport.h;
}

Evas_Pointer_Data *
_evas_pointer_data_by_device_get(Evas_Public_Data *edata, Efl_Input_Device *pointer)
{
   Eina_List *l;
   Evas_Pointer_Data *pdata;

   if (!pointer)
     pointer = edata->default_mouse;

   EINA_LIST_FOREACH(edata->pointers, l, pdata)
     {
        if (pdata->pointer == pointer)
          return pdata;
     }
   return NULL;
}

Eina_Bool
_evas_pointer_data_add(Evas_Public_Data *edata, Efl_Input_Device *pointer)
{
   Evas_Pointer_Data *pdata;

   pdata = calloc(1, sizeof(Evas_Pointer_Data));
   EINA_SAFETY_ON_NULL_RETURN_VAL(pdata, EINA_FALSE);

   pdata->pointer = pointer;
   edata->pointers = eina_list_append(edata->pointers, pdata);
   return EINA_TRUE;
}

void
_evas_pointer_data_remove(Evas_Public_Data *edata, Efl_Input_Device *pointer)
{
   Eina_List *l;
   Evas_Pointer_Data *pdata;

   EINA_LIST_FOREACH(edata->pointers, l, pdata)
     {
        if (pdata->pointer == pointer)
          {
             edata->pointers = eina_list_remove_list(edata->pointers, l);
             eina_list_free(pdata->object.in);
             free(pdata);
             break;
          }
     }
}

Eina_List *
_evas_pointer_list_in_rect_get(Evas_Public_Data *edata, Evas_Object *obj,
                               Evas_Object_Protected_Data *obj_data,
                               int w, int h)
{
   Eina_List *l, *list = NULL;
   Evas_Pointer_Data *pdata;

   EINA_LIST_FOREACH(edata->pointers, l, pdata)
     {
        if (evas_object_is_in_output_rect(obj, obj_data, pdata->x,
                                          pdata->y, w, h))
          list = eina_list_append(list, pdata);
     }

   return list;
}

#include "canvas/evas_canvas.eo.c"
