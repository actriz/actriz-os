#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cairo.h>
#include <math.h>
#include "ojos.h"
#include <stdlib.h>
gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);

#define GLOW 20
#define AGENT_EYE_DIAMETER 28.512
#define AGENT_H (AGENT_EYE_DIAMETER + 2.0 * OJOS_MARGEN)
#define AGENT_W (AGENT_H + AGENT_EYE_DIAMETER + OJOS_MARGEN)

double base_w = 105.0;
double base_h = 18.0;
double cur_x, cur_y, cur_w, cur_h;
double base_x, base_y;
double vx = 0, vy = 0, vw = 0, vh = 0;
double target_x, target_y, target_w, target_h;
double cur_bend = 0, target_bend = 0;
gboolean dragging = FALSE;
gboolean animating = FALSE;
gboolean smooth_timer_running = FALSE;
double drag_start_x, drag_start_y;
double final_dx = 0;
int last_gap = 33;
Window win_id;

gboolean estado_agente = FALSE;

/* Parpadeo breve del cuerpo blanco al activar el agente. */
double alpha_blanco = 1.0;
guint flash_timer = 0;
gboolean press_active = FALSE;
gboolean long_press_triggered = FALSE;
guint hold_timer = 0;

double press_x, press_y;

#define HOLD_DURATION_MS 550
#define HOLD_MOVE_LIMIT 8.0

void draw_pill(cairo_t *cr, double w, double h, double bend);
gboolean animate_back(gpointer data);

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

void draw_pill(cairo_t *cr, double w, double h, double bend) {
    double r = h / 2.0;
    if (bend > h) bend = h;
    if (bend < -h) bend = -h;

    cairo_new_sub_path(cr);
    cairo_move_to(cr, r, 0);
    cairo_curve_to(cr, w * 0.25, bend, w * 0.75, bend, w - r, 0);
    cairo_arc(cr, w - r, r, r, -M_PI/2, M_PI/2);
    cairo_curve_to(cr, w * 0.75, h + bend, w * 0.25, h + bend, r, h);
    cairo_arc(cr, r, h - r, r, M_PI/2, 3*M_PI/2);
    cairo_close_path(cr);
}

void update_shape(GtkWidget *widget) {
    int w = (int)cur_w + (GLOW * 2);
    int h = (int)cur_h + (GLOW * 2);
    if (w <= 1 || h <= 1) return;
    cairo_surface_t *mask = cairo_image_surface_create(CAIRO_FORMAT_A1, w, h);
    cairo_t *cr = cairo_create(mask);
    cairo_translate(cr, GLOW, GLOW);
    draw_pill(cr, cur_w, cur_h, cur_bend);
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

    cairo_surface_t *glow = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)(w + GLOW*2), (int)(h + GLOW*2));
    cairo_t *cr_glow = cairo_create(glow);
    cairo_translate(cr_glow, GLOW, GLOW);
    draw_pill(cr_glow, w + 4.0, h + 4.0, cur_bend);
    cairo_set_source_rgba(cr_glow, 1.0, 1.0, 1.0, alpha_blanco);
    cairo_fill(cr_glow);
    blur_surface(glow, 12);
    cairo_set_source_surface(cr, glow, -2, -2);
    cairo_paint(cr);
    cairo_destroy(cr_glow);
    cairo_surface_destroy(glow);

    cairo_save(cr);
    cairo_translate(cr, GLOW, GLOW);
    draw_pill(cr, w, h, cur_bend);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha_blanco);
    cairo_fill(cr);

    if (estado_agente)
        dibujar_ojos(cr, w, h);

    cairo_restore(cr);

    return FALSE;
}

void on_realize(GtkWidget *widget, gpointer data) {
    win_id = gdk_x11_window_get_xid(gtk_widget_get_window(widget));
    GdkWindow *gw_ov = gtk_widget_get_window(widget);
    if (gw_ov) gdk_window_set_override_redirect(gw_ov, TRUE);
    update_shape(widget);
}

void on_map(GtkWidget *widget, gpointer data) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "i3-msg '[id=\"%lu\"] floating enable, border none, sticky enable'", win_id);
    system(cmd);
    gtk_window_move(GTK_WINDOW(widget), (int)(base_x - GLOW), (int)(base_y - GLOW));
    gtk_window_resize(GTK_WINDOW(widget), (int)(base_w + GLOW*2), (int)(base_h + GLOW*2));
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) { return TRUE; }

gboolean smooth_move(gpointer data) {

    GtkWidget *widget = GTK_WIDGET(data);
    if (!dragging) { smooth_timer_running = FALSE; return FALSE; }
    cur_x += (target_x - cur_x) * 0.3; cur_y += (target_y - cur_y) * 0.3;
    cur_w += (target_w - cur_w) * 0.3; cur_h += (target_h - cur_h) * 0.3;
    gtk_window_resize(GTK_WINDOW(widget), (int)(cur_w + GLOW*2), (int)(cur_h + GLOW*2));
    gtk_window_move(GTK_WINDOW(widget), (int)(cur_x - GLOW), (int)(cur_y - GLOW));
    update_shape(widget); gtk_widget_queue_draw(widget);
    return TRUE;
}

gboolean restaurar_blanco(gpointer data) {
    GtkWidget *widget = GTK_WIDGET(data);

    alpha_blanco = 1.0;
    flash_timer = 0;

    gtk_widget_queue_draw(widget);

    return G_SOURCE_REMOVE;
}

gboolean activar_estado_agente(gpointer data) {
    GtkWidget *widget = GTK_WIDGET(data);

    if (!press_active || dragging)
        return G_SOURCE_REMOVE;

    hold_timer = 0;
    long_press_triggered = TRUE;

    estado_agente = !estado_agente;

    if (estado_agente) {
        if (flash_timer) {
            g_source_remove(flash_timer);
            flash_timer = 0;
        }

        alpha_blanco = 0.0;
        gtk_widget_queue_draw(widget);

        flash_timer = g_timeout_add(
            80,
            restaurar_blanco,
            widget
        );
    }

    if (estado_agente)
        system("pkill -USR1 -x borde >/dev/null 2>&1");
    else
        system("pkill -USR2 -x borde >/dev/null 2>&1");

    target_bend = 0;
    vx = vy = vw = vh = 0;

    animating = TRUE;
    g_timeout_add(16, animate_back, widget);

    return G_SOURCE_REMOVE;
}

gboolean on_button_press(GtkWidget *widget,
                         GdkEventButton *event,
                         gpointer data) {
    if (event->button != 1 || press_active)
        return FALSE;

    press_active = TRUE;
    dragging = FALSE;
    animating = FALSE;
    long_press_triggered = FALSE;

    vx = vy = vw = vh = 0;

    press_x = event->x_root;
    press_y = event->y_root;

    drag_start_x = press_x;
    drag_start_y = press_y;
    final_dx = 0;

    GdkWindow *gdk_win = gtk_widget_get_window(widget);
    Display *xdisplay =
        GDK_DISPLAY_XDISPLAY(gdk_window_get_display(gdk_win));

    XGrabPointer(
        xdisplay, win_id, True,
        ButtonReleaseMask | PointerMotionMask,
        GrabModeAsync, GrabModeAsync,
        None, None, CurrentTime
    );

    hold_timer = g_timeout_add(
        HOLD_DURATION_MS,
        activar_estado_agente,
        widget
    );

    return TRUE;
}

gboolean on_motion_notify(GtkWidget *widget,
                          GdkEventMotion *event,
                          gpointer data) {
    if (!press_active)
        return FALSE;

    if (long_press_triggered)
        return TRUE;

    if (!dragging) {
        double desplazamiento = hypot(
            event->x_root - press_x,
            event->y_root - press_y
        );

        if (desplazamiento <= HOLD_MOVE_LIMIT)
            return TRUE;

        if (hold_timer) {
            g_source_remove(hold_timer);
            hold_timer = 0;
        }

        dragging = TRUE;
        animating = FALSE;

        if (!smooth_timer_running) {
            smooth_timer_running = TRUE;
            g_timeout_add(16, smooth_move, widget);
        }
    }

    double raw_dx = event->x_root - drag_start_x;
    double raw_dy = event->y_root - drag_start_y;

    if (raw_dx > 480) raw_dx = 480;
    if (raw_dx < -480) raw_dx = -480;
    if (raw_dy > 0) raw_dy = 0;
    if (raw_dy < -100) raw_dy = -100;

    target_bend = raw_dy * 0.4;

    int nuevo_gap = 33 + (int)fabs(raw_dy);

    if (abs(nuevo_gap - last_gap) > 5) {
        char cmd[128];
        snprintf(
            cmd, sizeof(cmd),
            "i3-msg 'gaps bottom %d' > /dev/null 2>&1 &",
            nuevo_gap
        );
        system(cmd);
        last_gap = nuevo_gap;
    }

    double dx = aplicar_resistencia(raw_dx);
    final_dx = dx;

    double extra_w = (fabs(raw_dy) - 5.0) * 0.5;
    if (extra_w < 0) extra_w = 0;

    double extra_h = (fabs(raw_dy) - 5.0) * 0.05;
    if (extra_h < 0) extra_h = 0;

    double drag_base_w = estado_agente ? AGENT_W : base_w;
    double drag_base_h = estado_agente ? AGENT_H : base_h;

    double drag_base_x = estado_agente
        ? base_x + (base_w - AGENT_W) / 2.0
        : base_x;

    double drag_base_y = estado_agente
        ? base_y + base_h - AGENT_H
        : base_y;

    target_w = drag_base_w + 2.0 + extra_w;
    target_h = drag_base_h + 2.0 + extra_h;

    target_x = drag_base_x - 1.0 + dx - extra_w / 2.0;
    target_y = drag_base_y - 1.0 + raw_dy;

    return TRUE;
}

gboolean animate_back(gpointer data) {
    GtkWidget *widget = GTK_WIDGET(data);

    if (!animating)
        return FALSE;

    double destino_w = estado_agente ? AGENT_W : base_w;
    double destino_h = estado_agente ? AGENT_H : base_h;

    double destino_x = estado_agente
        ? base_x + (base_w - AGENT_W) / 2.0
        : base_x;

    double destino_y = estado_agente
        ? base_y + base_h - AGENT_H
        : base_y;

    vx += (destino_x - cur_x) * 0.10;
    vy += (destino_y - cur_y) * 0.10;
    vw += (destino_w - cur_w) * 0.10;
    vh += (destino_h - cur_h) * 0.10;

    vx *= 0.75;
    vy *= 0.75;
    vw *= 0.75;
    vh *= 0.75;

    cur_x += vx;
    cur_y += vy;
    cur_w += vw;
    cur_h += vh;

    if ((vy > 0 && cur_y > destino_y) ||
        (vy < 0 && cur_y < destino_y)) {
        cur_y = destino_y;
        vy = 0;
    }

    cur_bend += (target_bend - cur_bend) * 0.2;

    if (fabs(vx) < 0.1 && fabs(vy) < 0.1 &&
        fabs(vw) < 0.1 && fabs(vh) < 0.1) {

        cur_x = destino_x;
        cur_y = destino_y;
        cur_w = destino_w;
        cur_h = destino_h;
        cur_bend = 0;

        gtk_window_resize(
            GTK_WINDOW(widget),
            (int)(cur_w + GLOW * 2),
            (int)(cur_h + GLOW * 2)
        );

        gtk_window_move(
            GTK_WINDOW(widget),
            (int)(cur_x - GLOW),
            (int)(cur_y - GLOW)
        );

        update_shape(widget);
        gtk_widget_queue_draw(widget);
        animating = FALSE;

        return FALSE;
    }

    gtk_window_resize(
        GTK_WINDOW(widget),
        (int)(cur_w + GLOW * 2),
        (int)(cur_h + GLOW * 2)
    );

    gtk_window_move(
        GTK_WINDOW(widget),
        (int)(cur_x - GLOW),
        (int)(cur_y - GLOW)
    );

    update_shape(widget);
    gtk_widget_queue_draw(widget);

    return TRUE;
}

gboolean on_button_release(GtkWidget *widget,
                           GdkEventButton *event,
                           gpointer data) {
    if (event->button != 1 || !press_active)
        return FALSE;

    if (hold_timer) {
        g_source_remove(hold_timer);
        hold_timer = 0;
    }

    press_active = FALSE;

    GdkWindow *gdk_win = gtk_widget_get_window(widget);
    Display *xdisplay =
        GDK_DISPLAY_XDISPLAY(gdk_window_get_display(gdk_win));

    XUngrabPointer(xdisplay, CurrentTime);

    if (long_press_triggered) {
        long_press_triggered = FALSE;
        return TRUE;
    }

    if (!dragging) {
        system("/opt/actriz/bin/toggle-dock.sh &");

        target_bend = 0;
        vx = vy = vw = vh = 0;
        animating = TRUE;
        g_timeout_add(16, animate_back, widget);

        return TRUE;
    }

    dragging = FALSE;

    system("i3-msg 'gaps bottom 33' > /dev/null 2>&1 &");
    last_gap = 33;

    target_bend = 0;

    if (fabs(final_dx) < 5.0)
        system("/opt/actriz/bin/toggle-dock.sh &");

    if (final_dx > 100)
        system("i3-msg workspace next_on_output");
    else if (final_dx < -100)
        system("i3-msg workspace prev_on_output");

    vx = vy = vw = vh = 0;
    animating = TRUE;

    g_timeout_add(16, animate_back, widget);

    return TRUE;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_widget_set_app_paintable(window, TRUE);
    gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);
    gtk_window_set_focus_on_map(GTK_WINDOW(window), FALSE);
    gtk_widget_set_can_focus(window, FALSE);
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    GdkScreen *screen = gtk_widget_get_screen(window);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual != NULL && gdk_screen_is_composited(screen)) gtk_widget_set_visual(window, visual);

    GdkMonitor *monitor = gdk_display_get_primary_monitor(gdk_display_get_default());
    GdkRectangle geometry; gdk_monitor_get_geometry(monitor, &geometry);

    base_x = (geometry.width - base_w) / 2.0;
    base_y = geometry.height - base_h - 9.0;
    cur_x = base_x; cur_y = base_y; cur_w = base_w; cur_h = base_h;

    gtk_window_move(GTK_WINDOW(window), (int)(base_x - GLOW), (int)(base_y - GLOW));
    gtk_window_resize(GTK_WINDOW(window), (int)(base_w + GLOW*2), (int)(base_h + GLOW*2));
    gtk_widget_set_size_request(window, 1, 1);

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
