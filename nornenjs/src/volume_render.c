#include "nornenjs.h"
#include "opengles.h"
#include "socket_io_client.hpp"

static pthread_t thread_id;

// ~ mouse event
static void mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info) {
	Evas_Event_Mouse_Move *ev = (Evas_Event_Mouse_Move *) event_info;
	appdata_s *ad = data;

	ad->mouse_down = EINA_TRUE;

	ad->oldVectorX1 = ev->cur.canvas.x;
	ad->oldVectorY1 = ev->cur.canvas.y;
}

static void mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info) {
	Evas_Event_Mouse_Move *ev = (Evas_Event_Mouse_Move *) event_info;
	appdata_s *ad = data;

	if (ad->mouse_down && !ad->multi_mouse_down) {
		dlog_print(DLOG_VERBOSE, LOG_TAG, "single mouse move");
		ad->rotationX += (ev->cur.canvas.x - ev->prev.canvas.x) / 10.0;
		ad->rotationY += (ev->cur.canvas.y - ev->prev.canvas.y) / 10.0;
		emit_rotation(ad->rotationX, ad->rotationY);
	}

	if (ad->multi_mouse_down) {
		ad->oldVectorX1 = ev->cur.canvas.x;
		ad->oldVectorY1 = ev->cur.canvas.y;
	}
}

static void mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info) {
	appdata_s *ad = data;

	if (ad->mouse_down) {
		emit_quality();
		ad->mouse_down = EINA_FALSE;
	}
}

// ~ Multi Mouse event
static float spacing(float x1, float y1, float x2, float y2) {

	float x = x1 - x2;
	float y = y1 - y2;

	return sqrt(x * x + y * y);
}

static void multi_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info) {
	Evas_Event_Multi_Move *ev = (Evas_Event_Multi_Move *) event_info;
	appdata_s *ad = data;

	dlog_print(DLOG_VERBOSE, LOG_TAG, "multi mouse down");

	ad->multi_mouse_down = EINA_TRUE;

	ad->oldVectorX2 = ev->cur.canvas.x;
	ad->oldVectorY2 = ev->cur.canvas.y;

	ad->oldDist = spacing(ad->oldVectorX1, ad->oldVectorY1, ad->oldVectorX2, ad->oldVectorY2);
}

static void multi_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info) {

	Evas_Event_Multi_Move *ev = (Evas_Event_Multi_Move *) event_info;
	appdata_s *ad = data;

	if (ad->multi_mouse_down) {

		ad->oldVectorX2 = ev->cur.canvas.x;
		ad->oldVectorY2 = ev->cur.canvas.y;

		ad->newDist = spacing(ad->oldVectorX1, ad->oldVectorY1, ad->oldVectorX2, ad->oldVectorY2);

		// zoom in
		if (ad->newDist - ad->oldDist > 15) {

			ad->oldDist = ad->newDist;
			ad->div -= (((ad->newDist / ad->oldDist) / 50) * 10);

			if (ad->div <= 0.2f) {
				ad->div = 0.2f;
			}

			emit_zoom(ad->div);
		// zoom out
		} else if (ad->oldDist - ad->newDist > 15) {

			ad->oldDist = ad->newDist;
			ad->div += (((ad->newDist / ad->oldDist) / 50) * 10);
			if (ad->div >= 10.0f) {
				ad->div = 10.0f;
			}
			emit_zoom(ad->div);
		}
	}
}

static void multi_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info) {
	appdata_s *ad = data;
	if (ad->multi_mouse_down) {
		ad->multi_mouse_down = EINA_FALSE;
		emit_quality();
	}
}

// ~ Animator
static Eina_Bool anim_cb(void *data){
   appdata_s *ad = data;
   elm_glview_changed_set(ad->render_view);
   return ECORE_CALLBACK_RENEW;
}

static void destroy_anim(void *data, Evas *evas, Evas_Object *obj, void *event_info){
   Ecore_Animator *ani = data;
   ecore_animator_del(ani);
}

static void glview_create(appdata_s *ad){
   Evas_Object *render_view;

   ad->render_view = render_view = elm_glview_version_add(ad->box, EVAS_GL_GLES_1_X);
   evas_object_data_set(render_view, APPDATA_KEY, ad);

   elm_glview_mode_set(render_view, ELM_GLVIEW_ALPHA | ELM_GLVIEW_DEPTH);
   elm_glview_resize_policy_set(render_view, ELM_GLVIEW_RESIZE_POLICY_SCALE);
   elm_glview_render_policy_set(render_view, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);

   elm_glview_init_func_set(render_view, init_gles);
   elm_glview_del_func_set(render_view, destroy_gles);
   elm_glview_render_func_set(render_view, draw_gl);
   elm_glview_resize_func_set(render_view, resize_gl);

   evas_object_size_hint_align_set(render_view, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(render_view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	// ~ touch event add
	evas_object_event_callback_add(render_view, EVAS_CALLBACK_MOUSE_DOWN, mouse_down_cb, ad);
	evas_object_event_callback_add(render_view, EVAS_CALLBACK_MOUSE_UP, mouse_up_cb, ad);
	evas_object_event_callback_add(render_view, EVAS_CALLBACK_MOUSE_MOVE, mouse_move_cb, ad);

	// ~ multi touch event
	evas_object_event_callback_add(render_view, EVAS_CALLBACK_MULTI_DOWN, multi_mouse_down_cb, ad);
	evas_object_event_callback_add(render_view, EVAS_CALLBACK_MULTI_MOVE, multi_mouse_move_cb, ad);
	evas_object_event_callback_add(render_view, EVAS_CALLBACK_MULTI_UP, multi_mouse_up_cb, ad);
}

static void create_volume_render_view(appdata_s *ad){
	Evas_Object *box;

	ad->box = box = elm_box_add(ad->nf);
	evas_object_show(ad->box);

	glview_create(ad);
	elm_box_pack_end(ad->box, ad->render_view);
	evas_object_show(ad->render_view);

	ad->anim = ecore_animator_add(anim_cb, ad);
	evas_object_event_callback_add(ad->render_view, EVAS_CALLBACK_DEL, destroy_anim, ad->anim);
}

void volume_render_cb(void *data, Evas_Object *obj, void *event_info){
	appdata_s *ad = (appdata_s *)data;
	Elm_Object_Item *nav_item;

	int thread_error_number = 0;
	if ((thread_error_number = pthread_create(&thread_id, NULL, socket_io_client, NULL))) {
		dlog_print(DLOG_FATAL, LOG_TAG, "thread_error_number %d", thread_error_number);
	}

	create_volume_render_view(ad);
	nav_item = elm_naviframe_item_push(ad->nf, "Bighead", NULL, NULL, ad->box, NULL);
	elm_naviframe_item_title_visible_set(nav_item, EINA_FALSE);
}
