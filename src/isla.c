#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cairo.h>
#include <math.h>
#include <stdlib.h>

double base_w = 105.0;
double base_h = 18.0;
double cur_x, cur_y, cur_w, cur_h;
double base_x, base_y;
double vx = 0, vy = 0, vw = 0, vh = 0;
double target_x, target_y, target_w, target_h;
gboolean dragging = FALSE;
gboolean animating = FALSE;
gboolean smooth_timer_running = FALSE;
double drag_start_x, drag_start_y;
double final_dx = 0;
int last_gap = 32;
Window win_id;

void update_shape(GtkWidget *widget) {
    int w = (int)cur_w;
    int h = (int)cur_h;
    if (w <= 1 || h <= 1) return;
    cairo_surface_t *mask = cairo_image_surface_create(CAIRO_FORMAT_A1, w, h);
    cairo_t *cr = cairo_create(mask);
    double r = h / 2.0;
    cairo_new_sub_path(cr);
    cairo_arc(cr, w - r, r, r, -M_PI/2, 0);
    cairo_arc(cr, w - r, h - r, r, 0, M_PI/2);
    cairo_arc(cr, r, h - r, r, M_PI/2, M_PI);
    cairo_arc(cr, r, r, r, M_PI, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_surface_flush(mask);
    cairo_region_t *region = gdk_cairo_region_create_from_surface(mask);
    gdk_window_shape_combine_region(gtk_widget_get_window(widget), region, 0, 0);
    cairo_region_destroy(region);
    cairo_surface_destroy(mask);
    cairo_destroy(cr);
}

double aplicar_resistencia(double dx) {
    double signo = (dx >= 0) ? 1.0 : -1.0;
    double dist = fabs(dx);
    if (dist <= 10.0) return dx;
    return (10.0 + 160.0 * (1.0 - exp(-(dist - 10.0) / 150.0))) * signo;
}

gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data) {
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    double w = cur_w;
    double h = cur_h;
    double r = h / 2.0;

    cairo_new_sub_path(cr);
    cairo_arc(cr, w - r, r, r, -M_PI/2, 0);
    cairo_arc(cr, w - r, h - r, r, 0, M_PI/2);
    cairo_arc(cr, r, h - r, r, M_PI/2, M_PI);
    cairo_arc(cr, r, r, r, M_PI, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_clip(cr);

    cairo_pattern_t *grad = cairo_pattern_create_linear(0, 0, 0, h);
    cairo_pattern_add_color_stop_rgba(grad, 0.0, 1.0, 1.0, 1.0, 0.85);
    cairo_pattern_add_color_stop_rgba(grad, 0.5, 1.0, 1.0, 1.0, 0.90);
    cairo_pattern_add_color_stop_rgba(grad, 1.0, 1.0, 1.0, 1.0, 0.95);
    cairo_set_source(cr, grad);
    cairo_paint(cr);
    cairo_pattern_destroy(grad);
    return FALSE;
}

void on_realize(GtkWidget *widget, gpointer data) {
    win_id = gdk_x11_window_get_xid(gtk_widget_get_window(widget));
    update_shape(widget);
}

void on_map(GtkWidget *widget, gpointer data) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "i3-msg '[id=\"%lu\"] floating enable, border none, sticky enable'", win_id);
    system(cmd);
    gtk_window_move(GTK_WINDOW(widget), (int)base_x, (int)base_y);
    gtk_window_resize(GTK_WINDOW(widget), (int)base_w, (int)base_h);
}

gboolean smooth_move(gpointer data) {
    GtkWidget *widget = GTK_WIDGET(data);
    if (!dragging) {
        smooth_timer_running = FALSE;
        return FALSE;
    }
    cur_x += (target_x - cur_x) * 0.3;
    cur_y += (target_y - cur_y) * 0.3;
    cur_w += (target_w - cur_w) * 0.3;
    cur_h += (target_h - cur_h) * 0.3;
    gtk_window_resize(GTK_WINDOW(widget), (int)cur_w, (int)cur_h);
    gtk_window_move(GTK_WINDOW(widget), (int)cur_x, (int)cur_y);
    update_shape(widget);
    gtk_widget_queue_draw(widget);
    return TRUE;
}

gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button == 1) {
        dragging = TRUE;
        animating = FALSE;
        drag_start_x = event->x_root;
        drag_start_y = event->y_root;
        final_dx = 0;
        
        GdkWindow *gdk_win = gtk_widget_get_window(widget);
        GdkDisplay *display = gdk_window_get_display(gdk_win);
        Display *xdisplay = GDK_DISPLAY_XDISPLAY(display);
        XGrabPointer(xdisplay, win_id, True, ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

        target_w = base_w + 2.0;
        target_h = base_h + 2.0;
        target_x = base_x - 1.0;
        target_y = base_y - 1.0;
        
        if (!smooth_timer_running) {
            smooth_timer_running = TRUE;
            g_timeout_add(16, smooth_move, widget);
        }
        return TRUE;
    }
    return FALSE;
}

gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    if (dragging) {
        double raw_dx = event->x_root - drag_start_x;
        double raw_dy = event->y_root - drag_start_y;

        if (raw_dx > 480) raw_dx = 480;
        if (raw_dx < -480) raw_dx = -480;
        if (raw_dy > 100) raw_dy = 100;
        if (raw_dy < 0) raw_dy = 0;

        // Magia de los gaps para la isla superior
        int nuevo_gap = 32 + (int)raw_dy; 
        if (abs(nuevo_gap - last_gap) > 5) {
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "i3-msg 'gaps top %d' > /dev/null 2>&1 &", nuevo_gap);
            system(cmd);
            last_gap = nuevo_gap;
        }

        double dx = aplicar_resistencia(raw_dx);
        final_dx = dx;

        double extra_w = (fabs(raw_dy) - 5.0) * 0.5;
        if (extra_w < 0) extra_w = 0;
        double extra_h = (fabs(raw_dy) - 5.0) * 0.05;
        if (extra_h < 0) extra_h = 0;

        target_w = base_w + 2.0 + extra_w;
        target_h = base_h + 2.0 + extra_h;
        target_x = base_x - 1.0 + dx - (extra_w / 2.0);
        target_y = base_y - 1.0 + raw_dy;
        return TRUE;
    }
    return FALSE;
}

gboolean animate_back(gpointer data) {
    GtkWidget *widget = GTK_WIDGET(data);
    if (!animating) return FALSE;

    vx += (base_x - cur_x) * 0.15;
    vy += (base_y - cur_y) * 0.15;
    vw += (base_w - cur_w) * 0.15;
    vh += (base_h - cur_h) * 0.15;

    vx *= 0.7; vy *= 0.7; vw *= 0.7; vh *= 0.7;

    cur_x += vx; cur_y += vy;
    cur_w += vw; cur_h += vh;

    if (fabs(vx) < 0.1 && fabs(vy) < 0.1 && fabs(vw) < 0.1 && fabs(vh) < 0.1) {
        cur_x = base_x; cur_y = base_y; cur_w = base_w; cur_h = base_h;
        gtk_window_resize(GTK_WINDOW(widget), (int)cur_w, (int)cur_h);
        gtk_window_move(GTK_WINDOW(widget), (int)cur_x, (int)cur_y);
        update_shape(widget);
        gtk_widget_queue_draw(widget);
        animating = FALSE;
        return FALSE;
    }

    gtk_window_resize(GTK_WINDOW(widget), (int)cur_w, (int)cur_h);
    gtk_window_move(GTK_WINDOW(widget), (int)cur_x, (int)cur_y);
    update_shape(widget);
    gtk_widget_queue_draw(widget);
    return TRUE;
}

gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button == 1 && dragging) {
        dragging = FALSE;
        
        GdkWindow *gdk_win = gtk_widget_get_window(widget);
        GdkDisplay *display = gdk_window_get_display(gdk_win);
        Display *xdisplay = GDK_DISPLAY_XDISPLAY(display);
        XUngrabPointer(xdisplay, CurrentTime);

        system("i3-msg 'gaps top 32' > /dev/null 2>&1 &");
        last_gap = 32;

        if (final_dx > 100) system("i3-msg workspace next_on_output");
        else if (final_dx < -100) system("i3-msg workspace prev_on_output");

        animating = TRUE;
        g_timeout_add(16, animate_back, widget);
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_widget_set_app_paintable(window, TRUE);
    
    GdkScreen *screen = gtk_widget_get_screen(window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual != NULL && gdk_screen_is_composited(screen)) {
        gtk_widget_set_visual(window, visual);
    }

    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = gdk_display_get_primary_monitor(display);
    GdkRectangle geometry;
    gdk_monitor_get_geometry(monitor, &geometry);

    base_x = (geometry.width - base_w) / 2.0;
    base_y = 6.0;
    cur_x = base_x; cur_y = base_y; cur_w = base_w; cur_h = base_h;

    gtk_window_move(GTK_WINDOW(window), (int)base_x, (int)base_y);
    gtk_window_resize(GTK_WINDOW(window), (int)base_w, (int)base_h);
    gtk_widget_set_size_request(window, (int)base_w, (int)base_h);

    g_signal_connect(window, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(window, "realize", G_CALLBACK(on_realize), NULL);
    g_signal_connect(window, "map-event", G_CALLBACK(on_map), NULL);
    g_signal_connect(window, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(window, "button-release-event", G_CALLBACK(on_button_release), NULL);
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    
    gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
