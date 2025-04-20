#include <gtk/gtk.h>
#include <epoxy/gl.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "common.h"
#include "client.h"
#include "rendering.h"
#include "fragment.glsl.h"
#include "vertex.glsl.h"

#define USE_DOUBLES

/* TODO: get rid of global state... */
static GLuint program, vao;
static double t0;
static double tPass;
#ifdef BENCHMARK
static double tTotal = 0;
static int benchi = 0;
static int benchPhase = 0;
#endif

double render_scale;
double render_angle;
double render_x, render_y;
double render_width, render_height;

double render_window_start, render_window_size;

void init_window(void)
{
	render_window_start = 0.0;
	render_window_size = 1.0;
	tPass = 0.0;
}

void init_params(void)
{
	render_scale = 3.0;
	render_angle = 0.0;
	render_x = -0.75;
	render_y = 0.0;

	init_window();
}

/* The only polygon we ever render is a square */
static const float square_vertices[][2] = {
	{-1.0f,  0.0f},
	{ 1.0f,  0.0f},
	{-1.0f, -1.0f},
	{ 1.0f, -1.0f},
};

static GLuint create_shader(GLenum type, const GLchar *source, GLint length)
{
	GLuint shader;
	GLint status;
	int log_len;
	char *log_buf;

	shader = glCreateShader(type);
	assert(shader != 0);
	glShaderSource(shader, 1, &source, &length);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	/* Print shader compilation error log */
	if (status != GL_TRUE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		log_buf = g_malloc(log_len + 1);
		glGetShaderInfoLog(shader, log_len, NULL, log_buf);
		puts(log_buf);
		g_free(log_buf);
		assert(false); /* panic */
	}

	return shader;
}

void gl_init(GtkWidget *gl_area)
{
	GLuint fsh, vsh, vbo;
	GLint status;

	fprintf(stderr, "gl_init()\n");
	
	t0 = get_time_ms();
	init_params();
	
	gtk_gl_area_make_current(GTK_GL_AREA(gl_area));

	assert(gtk_gl_area_get_error(GTK_GL_AREA(gl_area)) == NULL);

	printf("Using OpenGL renderer: %s\n", glGetString(GL_RENDERER));



	fsh = create_shader(GL_FRAGMENT_SHADER,
	                    (GLchar*)&shaders_fragment_glsl,
	                    (GLint)shaders_fragment_glsl_len);

	vsh = create_shader(GL_VERTEX_SHADER,
	                    (GLchar*)&shaders_vertex_glsl,
	                    (GLint)shaders_vertex_glsl_len);

	program = glCreateProgram();
	assert(program != 0);
	glAttachShader(program, vsh);
	glAttachShader(program, fsh);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	assert(status == GL_TRUE); // TODO: print link error

	GLuint position_index = glGetAttribLocation(program, "position");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(position_index);
	glVertexAttribPointer(position_index, 2, GL_FLOAT, GL_FALSE,
	                      sizeof(float)*2,
	                      0);
}

void gl_fini(GtkWidget *gl_area)
{
	fprintf(stderr, "gl_fini()\n");
}

gboolean gl_draw(GtkWidget *gl_area)
{
	double start;
	start = get_time_ms();
	
	//fprintf(stderr, "gl_draw()\n");
	
	
	//glClearColor(0.5, 0.5, 0.5, 1.0);
	//glClear(GL_COLOR_BUFFER_BIT);
	
	if (render_window_size < (1.0/64.0)) {
		return TRUE;
	}
	
	glUseProgram(program);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glUniform1f(glGetUniformLocation(program, "time"), (start-t0)/500);
	glUniform1f(glGetUniformLocation(program, "angle"), render_angle);
	glUniform2f(glGetUniformLocation(program, "bounds"), render_window_start, render_window_size);
#ifdef USE_DOUBLES
	glUniform1d(glGetUniformLocation(program, "scale"), render_scale);
	glUniform2d(glGetUniformLocation(program, "midpoint"), render_x, render_y);
	glUniform2d(glGetUniformLocation(program, "resolution"), render_width, render_height);
#else
	glUniform1f(glGetUniformLocation(program, "scale"), render_scale);
	glUniform2f(glGetUniformLocation(program, "midpoint"), render_x, render_y);
	glUniform2f(glGetUniformLocation(program, "resolution"), render_width, render_height);
#endif

	glUseProgram(0);
	
	//glFlush();
	glFinish(); // blocks, required for accurate timing, may hang UI
	
	char *msg;
	double tFrame = get_time_ms()-start;
	tPass += tFrame;
#ifdef BENCHMARK
	tTotal += tFrame;
#endif
	msg = g_strdup_printf("scale=%le, x=%le, y=%le, res=%ux%u, t=%5.2fms, tp=%5.2fms", render_scale, render_x, render_y, (int)render_width, (int)render_height, tFrame, tPass);
	update_subtitle(msg);
	g_free(msg);
	
#ifdef PROGRESSIVE_ENHANCE
	render_window_start += render_window_size;
	if (render_window_size == 1.0 || render_window_start >= 1.00001) {
		render_window_start = 0.0;
		render_window_size /= 4.0;
		tPass = 0.0;
	}
#endif
	
	return TRUE;
}

/* callback to update gl_area screen pixel size */
void gl_resize(GtkWidget *gl_area, int width, int height)
{
	render_width = width;
	render_height = height;
	init_window(); // trigger a full redraw
}

/* enqueue a redraw every tick */
gboolean gl_tick(GtkWidget *gl_area, GdkFrameClock *frame_clock, gpointer user_data)
{
#ifdef BENCHMARK
	benchi++;
	if (benchi > 100) {
		benchi = 0;
		switch(benchPhase++) {
		case 0:
			render_scale=0.023743; render_angle=-0.103407; render_x=-0.744134; render_y=0.113833;
			break;
		case 1:
			render_scale=0.000022; render_angle=-0.274536; render_x=-0.251669; render_y=0.849788;
			break;
		case 2:
			render_scale=0.000001; render_angle=0.154032; render_x=-1.783608; render_y=0.000101;
			break;
		default:
			printf("Benchmark result: %lfms\n", tTotal);
			exit(0);
		}
	}
#endif
	gtk_widget_queue_draw(GTK_WIDGET(gl_area));
	return G_SOURCE_CONTINUE;
}

/* XXX: maaaaaybe this should be split in to seperate functions */
void gl_translate(double scale, double rotation, double dx, double dy, double cx, double cy)
{
	dx *= -render_scale / render_height;
	dy *= render_scale / render_height;
	
	if (cx == -1.0 && cy == -1.0) {
		cx = cy = 0.0;
	} else {
		cx /= render_height;
		cy /= render_height;
		cx -= render_width/render_height/2.0;
		cy -= 0.5;
		cx *= -1.0;
	}
	
	// scale relative to mouse cursor
	dx -= (cx-cx*scale) * render_scale;
	dy -= (cy-cy*scale) * render_scale;
	
	// make rotation relative to mouse cursor
	dx -= (cx-(cx * cos(rotation) - cy * sin(rotation))) * render_scale;
	dy -= (cy-(cx * sin(rotation) + cy * cos(rotation))) * render_scale;
	
	render_scale *= scale;
	
	render_angle += rotation;
	render_x += dx * cos(render_angle) - dy * sin(render_angle);
	render_y += dx * sin(render_angle) + dy * cos(render_angle);
	
	init_window();
}

/* (x0, y0) will be the new top-left corner, (x1, y1) bottom-right */
void gl_reframe(double x0, double y0, double x1, double y1)
{
	double midx, midy, angle, diagonal_angle, len, diagonal_len;
	
	midx = (render_width-x0-x1)/2.0;
	midy = (render_height-y0-y1)/2.0;
	
	angle = atan2(y1-y0, x1-x0);
	diagonal_angle = atan2(render_height, render_width);
	
	len = dist(x0, y0, x1, y1);
	if (len < 3.0) return; // probably an accidental click
	diagonal_len = dist(0, 0, render_width, render_height);
	
	// XXX: apparently my transformation function can't handle everything at once
	gl_translate(1.0, 0, midx, midy, -1.0, -1.0);
	gl_translate(len/diagonal_len, diagonal_angle-angle, 0, 0, -1.0, -1.0);
	
	fprintf(stderr, "render_scale=%lg; render_angle=%lg; render_x=%lg; render_y=%lg;\n", render_scale, render_angle, render_x, render_y);
}
