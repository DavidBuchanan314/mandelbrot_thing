#include <gtk/gtk.h>
#include <math.h>

#include "common.h"
#include "rendering.h"

static GtkWidget *header, *gl_area, *canvas;

static gboolean button_down = FALSE;
static gboolean ctrl_drag = FALSE;
static gboolean take_screenshot_next = FALSE;
static double prev_x, prev_y, drag_x, drag_y;
char *stats;

void update_subtitle(char *msg)
{
	if (stats) free(stats);
	stats = strdup(msg);
	//gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header), msg);
}

static gboolean on_scroll(GtkWidget *widget, GdkEventScroll *e, gpointer user_data)
{
	switch (e->direction) {
	case GDK_SCROLL_UP:
		gl_translate(0.99, 0, 0, 0, prev_x, prev_y);
		break;
	case GDK_SCROLL_DOWN:
		gl_translate(1.0/0.99, 0, 0, 0, prev_x, prev_y);
		break;
	case GDK_SCROLL_LEFT:
		gl_translate(1.0, -0.01, 0, 0, prev_x, prev_y);
		break;
	case GDK_SCROLL_RIGHT:
		gl_translate(1.0, 0.01, 0, 0, prev_x, prev_y);
		break;
	/*case GDK_SCROLL_SMOOTH:
		render_angle += e->delta_x * 0.01;
		render_scale -= e->delta_y;
		break;*/
	default:
		break;
	}

	return TRUE;
}

static gboolean on_move(GtkWidget *widget, GdkEventMotion *e, gpointer user_data)
{
	double dx, dy;
	dx = e->x - prev_x;
	dy = e->y - prev_y;
	prev_x = e->x;
	prev_y = e->y;

	if (button_down && ctrl_drag) {
		gl_translate(1.0, 0, dx, dy, -1.0, -1.0);
	}

	return TRUE;
}

static gboolean on_press(GtkWidget *widget, GdkEventButton *e, gpointer user_data)
{
	if (e->button != 1) return TRUE;

	switch (e->type) {
	case GDK_BUTTON_PRESS:
		ctrl_drag = (e->state & GDK_CONTROL_MASK) != 0;
		drag_x = e->x;
		drag_y = e->y;
		button_down = TRUE;
		break;
	case GDK_BUTTON_RELEASE:
		if (!ctrl_drag) {
			gl_reframe(drag_x, drag_y, e->x, e->y);
		}
		button_down = FALSE;
		break;
	default:
		break;
	}
	
	return TRUE;
}

static void home_clicked(GtkWidget *widget)
{
	init_params();
}

static void zoom_in_clicked(GtkWidget *widget)
{
	gl_translate(0.90, 0, 0, 0, -1.0, -1.0);
}

static void zoom_out_clicked(GtkWidget *widget)
{
	gl_translate(1.0/0.90, 0, 0, 0, -1.0, -1.0);
}

static void do_screenshot(void)
{
	GdkPixbuf *pixbuf;
	GdkWindow *window;
	cairo_surface_t *surface;
	cairo_t *cr;
	gint posx, posy, width, height;
	
	window = gtk_widget_get_window(gl_area);
	
	gtk_widget_translate_coordinates(gl_area, gtk_widget_get_toplevel(gl_area), 0, 0, &posx, &posy);
	width = gtk_widget_get_allocated_width(gl_area);
	height = gtk_widget_get_allocated_height(gl_area);
	
	surface = gdk_window_create_similar_surface(window, CAIRO_CONTENT_COLOR, width, height);
	cr = cairo_create(surface);
	gdk_cairo_set_source_window(cr, window, -posx, -posy);
	cairo_paint(cr);
	cairo_destroy(cr);
	
	pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
	gdk_pixbuf_save(pixbuf, "screenshot.png", "png", NULL, NULL);
	printf("saved screenshot\n");
	
	take_screenshot_next = FALSE;
}

static void screenshot_clicked(GtkWidget *widget)
{
	take_screenshot_next = TRUE;
	do_screenshot();
}

static gboolean canvas_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	guint width, height;
	double diag_angle, top_angle, top_len, dx, dy;
	
	//if (take_screenshot_next) do_screenshot();
	
	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);
	
	
	if (button_down && !ctrl_drag) {
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_set_line_width(cr, 2.0);
		
		/* I had to get out a pencil and paper to derive this mess... */
		diag_angle = atan2(prev_y-drag_y, prev_x-drag_x);
		top_angle = atan2(height, width);
		top_len = cos(top_angle)*dist(drag_x, drag_y, prev_x, prev_y);
		dx = cos(diag_angle-top_angle)*top_len;
		dy = sin(diag_angle-top_angle)*top_len;
		
		/* comments assume a down-right drag direction */
		cairo_move_to(cr, drag_x, drag_y); // top left
		cairo_line_to(cr, prev_x, prev_y); // bottom right
		
		cairo_line_to(cr, drag_x+dx, drag_y+dy); // top right
		cairo_line_to(cr, drag_x, drag_y); // top left (again)
		cairo_line_to(cr, prev_x-dx, prev_y-dy); // bottom left
		cairo_line_to(cr, prev_x, prev_y); // bottom right (again)
		
		cairo_stroke(cr);
	}
	
	if (stats != NULL) {
		cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr, 14);
		cairo_move_to(cr, 10, 20);
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_show_text(cr, stats);
		cairo_move_to(cr, 10-1, 20-1);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_show_text(cr, stats);
	}
	
	return FALSE;
}

static void activate(GtkApplication* app, gpointer user_data)
{
	GtkWidget *window, *button, *overlay;

	window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "Window");
	//gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

	/* Header bar */
	header = gtk_header_bar_new();
	gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Mandelbrot Microscope");
	gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
	gtk_window_set_titlebar(GTK_WINDOW(window), header);


	/* Control buttons */
	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("go-home", GTK_ICON_SIZE_BUTTON));
	g_signal_connect(button, "clicked", G_CALLBACK(home_clicked), NULL);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), button);

	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("zoom-in", GTK_ICON_SIZE_BUTTON));
	g_signal_connect(button, "clicked", G_CALLBACK(zoom_in_clicked), NULL);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), button);

	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("zoom-out", GTK_ICON_SIZE_BUTTON));
	g_signal_connect(button, "clicked", G_CALLBACK(zoom_out_clicked), NULL);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), button);
	
	button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_icon_name("camera-photo", GTK_ICON_SIZE_BUTTON));
	g_signal_connect(button, "clicked", G_CALLBACK(screenshot_clicked), NULL);
	gtk_header_bar_pack_start(GTK_HEADER_BAR(header), button);

	/* Overlay which will contain the GL Area and Canvas */
	overlay = gtk_overlay_new();
	gtk_container_add(GTK_CONTAINER(window), overlay);

	/* GL Area */
	gl_area = gtk_gl_area_new();
	//gtk_widget_set_size_request(GTK_WIDGET(gl_area), 768, 768);
	g_signal_connect(gl_area, "realize", G_CALLBACK(gl_init), NULL);
	g_signal_connect(gl_area, "unrealize", G_CALLBACK(gl_fini), NULL);
	g_signal_connect(gl_area, "render", G_CALLBACK(gl_draw), NULL);
	g_signal_connect(gl_area, "resize", G_CALLBACK(gl_resize), NULL);
	gtk_container_add(GTK_CONTAINER(overlay), gl_area);
	gtk_widget_add_tick_callback(gl_area, gl_tick, NULL, NULL);

	/* Canvas */
	// Since the canvas is "on top", it must handle the input events
	canvas = gtk_drawing_area_new();
	gtk_widget_add_events(canvas, GDK_SCROLL_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
	g_signal_connect(canvas, "scroll-event", G_CALLBACK(on_scroll), NULL);
	g_signal_connect(canvas, "motion-notify-event", G_CALLBACK(on_move), NULL);
	g_signal_connect(canvas, "button-press-event", G_CALLBACK(on_press), NULL);
	g_signal_connect(canvas, "button-release-event", G_CALLBACK(on_press), NULL);
	g_signal_connect(canvas, "draw", G_CALLBACK(canvas_draw), NULL);
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), canvas);

	gtk_widget_show_all(GTK_WIDGET(window));

}

int main (int argc, char *argv[])
{
	GtkApplication *app;
	int status;

	app = gtk_application_new("com.example.mandelbrot-microscope", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}

