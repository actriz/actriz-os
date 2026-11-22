#include <gtk/gtk.h>
#include <cairo.h>
#include <glib-unix.h>
#include <math.h>

static int screen_w = 1920;
static int screen_h = 1080;
static int encendido = 0;

static gboolean on_sigusr1(gpointer data) {
    encendido = 1;
    gtk_widget_queue_draw(GTK_WIDGET(data));
    return G_SOURCE_CONTINUE;
}

static gboolean on_sigusr2(gpointer data) {
    encendido = 0;
    gtk_widget_queue_draw(GTK_WIDGET(data));
    return G_SOURCE_CONTINUE;
}

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);

    if (!encendido) return FALSE;

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    double cx = screen_w / 2.0;
    double alpha_bot = 0.55;

    /* EL TRUCO ESTA AQUI: El centro de la luz esta exactamente en Y=300 (el fondo de la ventana) */
    cairo_pattern_t *patb = cairo_pattern_create_radial(cx, 300.0, 0, cx, 300.0, 280.0);
    cairo_pattern_add_color_stop_rgba(patb, 0.00, 0.88, 0.95, 1.00, 0.90 * alpha_bot);
    cairo_pattern_add_color_stop_rgba(patb, 0.15, 0.62, 0.79, 1.00, 0.86 * alpha_bot);
    cairo_pattern_add_color_stop_rgba(patb, 0.32, 0.31, 0.56, 1.00, 0.76 * alpha_bot);
    cairo_pattern_add_color_stop_rgba(patb, 0.45, 0.10, 0.40, 1.00, 0.55 * alpha_bot);
    cairo_pattern_add_color_stop_rgba(patb, 0.65, 0.10, 0.40, 1.00, 0.30 * alpha_bot);
    cairo_pattern_add_color_stop_rgba(patb, 0.82, 0.10, 0.40, 1.00, 0.10 * alpha_bot);
    cairo_pattern_add_color_stop_rgba(patb, 1.00, 0.10, 0.40, 1.00, 0.00);
    
    cairo_set_source(cr, patb);
    cairo_paint(cr);
    cairo_pattern_destroy(patb);

    return FALSE;
}

static void on_realize(GtkWidget *widget, gpointer data) {
    GdkWindow *gw = gtk_widget_get_window(widget);
    if (!gw) return;
    cairo_region_t *empty = cairo_region_create();
    gdk_window_input_shape_combine_region(gw, empty, 0, 0);
    cairo_region_destroy(empty);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GdkScreen *scr = gdk_screen_get_default();
    screen_w = gdk_screen_get_width(scr);
    screen_h = gdk_screen_get_height(scr);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);
    gtk_widget_set_app_paintable(window, TRUE);
    gtk_window_set_wmclass(GTK_WINDOW(window), "Borde", "Borde");

    GdkVisual *v = gdk_screen_get_rgba_visual(scr);
    if (v && gdk_screen_is_composited(scr)) gtk_widget_set_visual(window, v);

    /* Colocar la ventana EXACTAMENTE en la parte de abajo, 300px de alto */
    gtk_window_move(GTK_WINDOW(window), 0, screen_h - 300);
    gtk_window_resize(GTK_WINDOW(window), screen_w, 300);

    g_signal_connect(window, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(window, "realize", G_CALLBACK(on_realize), NULL);

    g_unix_signal_add(SIGUSR1, on_sigusr1, window);
    g_unix_signal_add(SIGUSR2, on_sigusr2, window);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
