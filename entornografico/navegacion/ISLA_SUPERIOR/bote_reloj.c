#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cairo.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

double w = 110.0;
double h = 30.0;
double base_x = 807.0;
double base_y = 5.0;
Window win_id;

// Bloquear el teclado (hacer que la ventana ignore todas las teclas)
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    return TRUE; 
}

// Hacer la ventana click-through (ignorar el mouse)
void hacer_click_through(GtkWidget *widget) {
    GdkWindow *gw = gtk_widget_get_window(widget);
    if (gw) {
        cairo_region_t *empty = cairo_region_create();
        gdk_window_input_shape_combine_region(gw, empty, 0, 0);
        cairo_region_destroy(empty);
    }
}

void dibujar_contenido(cairo_t *cr, double alpha) {
    double icon_x = 79.5;
    double icon_y = (h / 2.0) - 2.0;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char time_str[16];
    strftime(time_str, sizeof(time_str), "%-I:%M", tm);

    cairo_select_font_face(cr, "DejaVu Serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14.0);
    cairo_text_extents_t extents;
    cairo_text_extents(cr, time_str, &extents);

    double scale_x = 0.85; 
    double scale_y = 1.07;
    double arrow_left_edge = icon_x - 4.0;
    double text_x = arrow_left_edge - 3.5 - (extents.x_bearing + extents.width) * scale_x;
    if (text_x < 2.0) text_x = 2.0;

    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha);
    cairo_save(cr);
    cairo_translate(cr, text_x, (h + extents.height) / 2.0 - 1.0);
    cairo_scale(cr, scale_x, scale_y);
    cairo_move_to(cr, 0, 0);
    cairo_show_text(cr, time_str);
    cairo_restore(cr);

    cairo_save(cr);
    cairo_translate(cr, icon_x, icon_y);
    cairo_rotate(cr, -0.4722);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_width(cr, 2.5);
    cairo_move_to(cr, 0, -5.2);
    cairo_line_to(cr, 4.54, 4.54);
    cairo_line_to(cr, 0, 1.89);
    cairo_line_to(cr, -4.54, 4.54);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void blur_surface(cairo_surface_t *surf, int radius) {
    if (radius <= 0) return;
    cairo_surface_flush(surf);
    int bw = cairo_image_surface_get_width(surf);
    int bh = cairo_image_surface_get_height(surf);
    int stride = cairo_image_surface_get_stride(surf);
    unsigned char *data = cairo_image_surface_get_data(surf);
    unsigned char *tmp = malloc(stride * bh);
    if (!tmp) return;
    for (int y = 0; y < bh; y++) {
        for (int x = 0; x < bw; x++) {
            int r=0,g=0,b=0,a=0,count=0;
            for (int dx = -radius; dx <= radius; dx++) {
                int nx = x + dx;
                if (nx < 0 || nx >= bw) continue;
                unsigned char *p = data + y*stride + nx*4;
                b += p[0]; g += p[1]; r += p[2]; a += p[3]; count++;
            }
            unsigned char *d = tmp + y*stride + x*4;
            d[0] = b/count; d[1] = g/count; d[2] = r/count; d[3] = a/count;
        }
    }
    for (int y = 0; y < bh; y++) {
        for (int x = 0; x < bw; x++) {
            int r=0,g=0,b=0,a=0,count=0;
            for (int dy = -radius; dy <= radius; dy++) {
                int ny = y + dy;
                if (ny < 0 || ny >= bh) continue;
                unsigned char *p = tmp + ny*stride + x*4;
                b += p[0]; g += p[1]; r += p[2]; a += p[3]; count++;
            }
            unsigned char *d = data + y*stride + x*4;
            d[0] = b/count; d[1] = g/count; d[2] = r/count; d[3] = a/count;
        }
    }
    free(tmp);
    cairo_surface_mark_dirty(surf);
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_surface_t *glow = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)w, (int)h);
    cairo_t *cr_glow = cairo_create(glow);
    cairo_set_antialias(cr_glow, CAIRO_ANTIALIAS_BEST);
    dibujar_contenido(cr_glow, 0.85);
    blur_surface(glow, 5);
    blur_surface(glow, 5);
    cairo_set_source_surface(cr, glow, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr_glow);
    cairo_surface_destroy(glow);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
    dibujar_contenido(cr, 0.90);

    return FALSE;
}

void on_realize(GtkWidget *widget, gpointer data) {
    win_id = gdk_x11_window_get_xid(gtk_widget_get_window(widget));
    hacer_click_through(widget);
}

void on_map(GtkWidget *widget, gpointer data) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "i3-msg '[id=\"%lu\"] floating enable, border none, sticky enable'", win_id);
    system(cmd);
    gtk_window_move(GTK_WINDOW(widget), (int)base_x, (int)base_y);
    hacer_click_through(widget);
}

gboolean update_clock(gpointer data) { gtk_widget_queue_draw(GTK_WIDGET(data)); return TRUE; }

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_wmclass(GTK_WINDOW(window), "bote_reloj", "BoteReloj");

    // Bloquear foco de teclado
    gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);
    gtk_window_set_focus_on_map(GTK_WINDOW(window), FALSE);
    gtk_widget_set_can_focus(window, FALSE);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    gtk_widget_set_app_paintable(window, TRUE);
    GdkScreen *screen = gtk_widget_get_screen(window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual != NULL && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(window, visual);
    }

    gtk_window_move(GTK_WINDOW(window), (int)base_x, (int)base_y);
    gtk_window_resize(GTK_WINDOW(window), (int)w, (int)h);
    gtk_widget_set_size_request(window, (int)w, (int)h);

    g_signal_connect(window, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(window, "realize", G_CALLBACK(on_realize), NULL);
    g_signal_connect(window, "map-event", G_CALLBACK(on_map), NULL);
    g_timeout_add(1000, update_clock, window);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
