#pragma once

#include <gtk/gtk.h>

void init_params(void);
void gl_init(GtkWidget *gl_area);
void gl_fini(GtkWidget *gl_area);
void gl_resize(GtkWidget *gl_area, int width, int height);
gboolean gl_draw(GtkWidget *gl_area);
gboolean gl_tick(GtkWidget *gl_area, GdkFrameClock *frame_clock, gpointer user_data);
void gl_translate(double scale, double rotation, double dx, double dy, double cx, double cy);
void gl_reframe(double x0, double y0, double x1, double y1);
