#ifndef __nornenjs_H__
#define __nornenjs_H__

#include <app.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "nornenjs"

#if !defined(PACKAGE)
#define PACKAGE "org.tizen.nornenjs"
#endif

#define APPDATA_KEY "AppData"

#define SOCKET_URI "http://112.108.40.14:5000"
#define VOLUME_LIST_URL "http://112.108.40.14:10000/tizen/list"
#define VOLUME_DATA_URL "http://112.108.40.14:10000/tizen/data/"

#define ICON_DIR "/opt/usr/apps/org.tizen.nornenjs/res/images"

#define SOCKET_IMAGE_BIND_ERROR -9

typedef struct appdata {
	// Default Wrapper
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *nf;

	// First Loading View
	Evas_Object *image;
	Ecore_Timer *timer;

	// Box (Volume Render View)
	Ecore_Animator *anim;
	Evas_Object *box;
	Evas_Object *render_view;

	// Touch event
	Eina_Bool mouse_down :1;
	float rotation_x;
	float rotation_y;

	// Multi touch event
	Eina_Bool multi_mouse_down :1;
	float old_vector_x1;
	float old_vector_y1;
	float old_vector_x2;
	float old_vector_y2;
	float new_dist;
	float old_dist;
	float position_z;

	// Brightness
	Eina_Bool is_brightness :1;
	Evas_Object *bright_slider;

	// OTF
	Eina_Bool is_otf :1;
	Evas_Object *otf_slider;

	// Data
	int volume_number;
	char* volumeDataPn;
} appdata_s;

void volume_render_cb(void *data, Evas_Object *obj, void *event_info);

#endif /* __nornenjs_H__ */
