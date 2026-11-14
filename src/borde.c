#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cairo.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

int screen_w = 1920;
int screen_h = 1080;
double phase = 0.0;
Window win_id;

#define BORDE_GROSOR 96
#define VELOCIDAD 0.0015

void get_hotspot(double p, double *x, double *y) {
    p = fmod(p, 1.0);
    if (p < 0) p += 1.0;
    double L = screen_w, H = screen_h;
    double d = p * 2.0 * (L + H);
    if (d < L)              { *x = d;               *y = 0; }
    else if (d < L + H)     { *x = L;               *y = d - L; }
    else if (d < 2.0*L + H) { *x = L - (d - L - H); *y = H; }
    else                    { *x = 0;               *y = H - (d - 2.0*L - H); }
}

void dibujar_marco_base(cairo_t *cr) {
    double t = BORDE_GROSOR;
    cairo_pattern_t *pat;

    pat = cairo_pattern_create_linear(0, 0, 0, t);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 1, 1, 1, 0.55);
    cairo_pattern_add_color_stop_rgba(pat, 0.15, 1, 1, 1, 0.10);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 1, 1, 1, 0.00);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, 0, 0, screen_w, t);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);

    pat = cairo_pattern_create_linear(0, screen_h - t, 0, screen_h);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 1, 1, 1, 0.00);
    cairo_pattern_add_color_stop_rgba(pat, 0.85, 1, 1, 1, 0.10);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 1, 1, 1, 0.55);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, 0, screen_h - t, screen_w, t);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);

    pat = cairo_pattern_create_linear(0, 0, t, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 1, 1, 1, 0.55);
    cairo_pattern_add_color_stop_rgba(pat, 0.15, 1, 1, 1, 0.10);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 1, 1, 1, 0.00);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, 0, 0, t, screen_h);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);

    pat = cairo_pattern_create_linear(screen_w - t, 0, screen_w, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 1, 1, 1, 0.00);
    cairo_pattern_add_color_stop_rgba(pat, 0.85, 1, 1, 1, 0.10);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 1, 1, 1, 0.55);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, screen_w - t, 0, t, screen_h);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
}

void dibujar_hotspot(cairo_t *cr, double hx, double hy, double r, double alpha, double red, double green, double blue) {
    cairo_save(cr);
    double t = BORDE_GROSOR;
    cairo_rectangle(cr, 0, 0, screen_w, t);
    cairo_rectangle(cr, 0, screen_h - t, screen_w, t);
    cairo_rectangle(cr, 0, 0, t, screen_h);
    cairo_rectangle(cr, screen_w - t, 0, t, screen_h);
    cairo_clip(cr);

    cairo_translate(cr, hx, hy);
    if (hy < 1.0 || hy > screen_h - 1.0) {
        cairo_scale(cr, 10.0, 1.0);
    } else {
        cairo_scale(cr, 1.0, 10.0);
    }

    cairo_pattern_t *pat = cairo_pattern_create_radial(0, 0, 0, 0, 0, r);
    cairo_pattern_add_color_stop_rgba(pat, 0.00, red, green, blue, alpha);
    cairo_pattern_add_color_stop_rgba(pat, 0.20, red, green, blue, alpha * 0.75);
    cairo_pattern_add_color_stop_rgba(pat, 0.45, red, green, blue, alpha * 0.40);
    cairo_pattern_add_color_stop_rgba(pat, 0.70, red, green, blue, alpha * 0.15);
    cairo_pattern_add_color_stop_rgba(pat, 1.00, red, green, blue, 0.00);
    cairo_set_source(cr, pat);
    cairo_paint(cr);
    cairo_pattern_destroy(pat);
    cairo_restore(cr);
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    dibujar_marco_base(cr);

    double hx, hy;
    get_hotspot(phase, &hx, &hy);
    dibujar_hotspot(cr, hx, hy, 35.0, 0.95, 1.0, 1.0, 1.0);

    get_hotspot(phase + 0.333, &hx, &hy);
    dibujar_hotspot(cr, hx, hy, 45.0, 0.85, 0.55, 0.30, 1.0);

    get_hotspot(phase + 0.666, &hx, &hy);
    dibujar_hotspot(cr, hx, hy, 45.0, 0.85, 0.30, 0.55, 1.0);

    return FALSE;
}

void aplicar_shape_y_input(GtkWidget *widget) {
    GdkWindow *gw = gtk_widget_get_window(widget);
    if (!gw) return;
    int t = BORDE_GROSOR;
    cairo_rectangle_int_t rects[4] = {
        {0, 0, screen_w, t},
        {0, screen_h - t, screen_w, t},
        {0, 0, t, screen_h},
        {screen_w - t, 0, t, screen_h}
    };
    cairo_region_t *shape = cairo_region_create_rectangles(rects, 4);
    gdk_window_shape_combine_region(gw, shape, 0, 0);
    cairo_region_destroy(shape);
    cairo_region_t *empty = cairo_region_create();
    gdk_window_input_shape_combine_region(gw, empty, 0, 0);
    cairo_region_destroy(empty);
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    if (dpy && win_id) {
        XShapeCombineRectangles(dpy, win_id, ShapeInput, 0, 0, NULL, 0, ShapeSet, Unsorted);
        XFlush(dpy);
    }
}

void on_realize(GtkWidget *widget, gpointer data) {
    GdkWindow *gw = gtk_widget_get_window(widget);
    if (!gw) return;
    win_id = gdk_x11_window_get_xid(gw);
    aplicar_shape_y_input(widget);
}

void on_map(GtkWidget *widget, gpointer data) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "i3-msg '[id=\"%lu\"] floating enable, border none, sticky enable, move position 0 0'", win_id);
    system(cmd);
    gtk_window_move(GTK_WINDOW(widget), 0, 0);
    aplicar_shape_y_input(widget);
}

gboolean tick(gpointer data) {
    phase += VELOCIDAD;
    if (phase >= 1.0) phase -= 1.0;
    gtk_widget_queue_draw(GTK_WIDGET(data));
    return TRUE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GdkScreen *gdk_screen = gdk_screen_get_default();
    int sw = gdk_screen_get_width(gdk_screen);
    int sh = gdk_screen_get_height(gdk_screen);
    if (sw > 0) screen_w = sw;
    if (sh > 0) screen_h = sh;

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_wmclass(GTK_WINDOW(window), "borde", "Borde");
    gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);
    gtk_window_set_focus_on_map(GTK_WINDOW(window), FALSE);
    gtk_widget_set_can_focus(window, FALSE);
    gtk_widget_set_app_paintable(window, TRUE);
    GdkVisual *visual = gdk_screen_get_rgba_visual(gdk_screen);
    if (visual != NULL && gdk_screen_is_composited(gdk_screen)) {
        gtk_widget_set_visual(window, visual);
    }
    gtk_window_move(GTK_WINDOW(window), 0, 0);
    gtk_window_resize(GTK_WINDOW(window), screen_w, screen_h);
    gtk_window_set_default_size(GTK_WINDOW(window), screen_w, screen_h);
    g_signal_connect(window, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(window, "realize", G_CALLBACK(on_realize), NULL);
    g_signal_connect(window, "map-event", G_CALLBACK(on_map), NULL);
    g_timeout_add(33, tick, window);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
