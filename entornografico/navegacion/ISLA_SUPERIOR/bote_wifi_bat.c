#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cairo.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <libudev.h>

double w = 400.0;
double h = 120.0;
double base_x = 1012.0;
double base_y = -40.0;
Window win_id;

int burst_ticks = 0;
double pulse_phase = 0.0;
int prev_charging = 0;

static char bat_capacity_path[256] = {0};
static char bat_status_path[256] = {0};
static char ac_online_path[256] = {0};

static int ac_online_cache = 0;
static int bat_full_cache = 0;

gboolean update_info(gpointer data);

static struct udev *udev_ctx = NULL;
static struct udev_monitor *udev_mon = NULL;
static guint udev_watch_id = 0;

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) { return TRUE; }

void hacer_click_through(GtkWidget *widget) {
    GdkWindow *gw = gtk_widget_get_window(widget);
    if (gw) {
        cairo_region_t *empty = cairo_region_create();
        gdk_window_input_shape_combine_region(gw, empty, 0, 0);
        cairo_region_destroy(empty);
    }
}

static void redraw_inmediato(GtkWidget *w) {
    GdkWindow *gw = gtk_widget_get_window(w);
    if (gw) {
        gdk_window_invalidate_rect(gw, NULL, TRUE);
        gdk_window_process_updates(gw, TRUE);
    }
}

void detectar_hardware() {
    DIR *d = opendir("/sys/class/power_supply");
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char base[256];
        snprintf(base, sizeof(base), "/sys/class/power_supply/%s", e->d_name);

        char cap[300], sta[300], typ[300];
        snprintf(cap, sizeof(cap), "%s/capacity", base);
        snprintf(sta, sizeof(sta), "%s/status", base);
        snprintf(typ, sizeof(typ), "%s/type", base);

        FILE *f = fopen(typ, "r");
        if (!f) continue;
        char type[32] = {0};
        fgets(type, sizeof(type), f);
        fclose(f);

        if (strncmp(type, "Battery", 7) == 0 && bat_capacity_path[0] == 0) {
            strncpy(bat_capacity_path, cap, sizeof(bat_capacity_path) - 1);
            strncpy(bat_status_path, sta, sizeof(bat_status_path) - 1);
        }
        if (strncmp(type, "Mains", 5) == 0 && ac_online_path[0] == 0) {
            char online[300];
            snprintf(online, sizeof(online), "%s/online", base);
            strncpy(ac_online_path, online, sizeof(ac_online_path) - 1);
        }
    }
    closedir(d);
}

int leer_entero(const char *path, int fallback) {
    if (!path || path[0] == 0) return fallback;
    FILE *f = fopen(path, "r");
    if (!f) return fallback;
    int v = fallback;
    if (fscanf(f, "%d", &v) != 1) v = fallback;
    fclose(f);
    return v;
}

int get_battery_level() { return leer_entero(bat_capacity_path, 100); }
int get_charging_efectivo() { return (ac_online_cache == 1 && !bat_full_cache) ? 1 : 0; }

int get_wifi_strength() {
    FILE *f = fopen("/proc/net/wireless", "r");
    if (!f) return 0;
    char line[256]; int strength = 0;
    fgets(line, sizeof(line), f); fgets(line, sizeof(line), f);
    while (fgets(line, sizeof(line), f)) {
        char iface[32]; int val;
        if (sscanf(line, "%31[^:]: %*d %*d %d", iface, &val) == 2) { strength = val; break; }
    }
    fclose(f);
    if (strength > -50) return 4; if (strength > -60) return 3;
    if (strength > -70) return 2; if (strength > -80) return 1;
    return 0;
}

static gboolean on_udev_event(GIOChannel *source, GIOCondition condition, gpointer data) {
    if (!(condition & G_IO_IN)) return TRUE;
    int eventos = 0;
    struct udev_device *dev;
    while ((dev = udev_monitor_receive_device(udev_mon))) {
        const char *online = udev_device_get_sysattr_value(dev, "online");
        const char *status = udev_device_get_sysattr_value(dev, "status");
        if (online) { ac_online_cache = atoi(online); eventos++; }
        if (status) { bat_full_cache = (strncmp(status, "Full", 4) == 0) ? 1 : 0; eventos++; }
        udev_device_unref(dev);
    }
    if (eventos == 0) return TRUE;
    int charging = get_charging_efectivo();
    if (prev_charging != charging) {
        if (prev_charging == 0 && charging == 1) {
            burst_ticks = 12;
            pulse_phase = 0.0;
        }
        prev_charging = charging;
        redraw_inmediato(GTK_WIDGET(data));
    }
    return TRUE;
}

void setup_udev_monitor(GtkWidget *window) {
    udev_ctx = udev_new();
    if (!udev_ctx) return;
    udev_mon = udev_monitor_new_from_netlink(udev_ctx, "udev");
    if (!udev_mon) return;
    udev_monitor_filter_add_match_subsystem_devtype(udev_mon, "power_supply", NULL);
    udev_monitor_enable_receiving(udev_mon);
    GIOChannel *channel = g_io_channel_unix_new(udev_monitor_get_fd(udev_mon));
    g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, NULL);
    udev_watch_id = g_io_add_watch(channel, G_IO_IN, on_udev_event, window);
    g_io_channel_unref(channel);
}

void dibujar_bateria(cairo_t *cr, double x, double y, int level, double alpha) {
    double b_w = 20.0, b_h = 10.0, nub_w = 2.0, nub_h = 4.0, padding = 1.5;
    double r = 4.0;
    cairo_set_line_width(cr, 1.2);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha);
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + r, y + r, r, M_PI, 1.5 * M_PI);
    cairo_arc(cr, x + b_w - r, y + r, r, 1.5 * M_PI, 2 * M_PI);
    cairo_arc(cr, x + b_w - r, y + b_h - r, r, 0, 0.5 * M_PI);
    cairo_arc(cr, x + r, y + b_h - r, r, 0.5 * M_PI, M_PI);
    cairo_close_path(cr);
    cairo_stroke(cr);
    cairo_rectangle(cr, x + b_w + 0.6, y + (b_h - nub_h)/2, nub_w, nub_h);
    cairo_fill(cr);
    if (level > 0) {
        double fill_w = (b_w - padding * 2) * (level / 100.0);
        if (fill_w > 0) {
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha);
            cairo_rectangle(cr, x + padding, y + padding, fill_w, b_h - padding * 2);
            cairo_fill(cr);
        }
    }
}

void dibujar_wifi(cairo_t *cr, double x, double y, int bars, double alpha) {
    double cx = x + 7.0; double cy = y + 9.0;
    cairo_set_line_width(cr, 1.7); cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, bars >= 1 ? alpha : alpha * 0.3);
    cairo_arc(cr, cx, cy, 1.1, 0, 2*M_PI); cairo_fill(cr);
    double radios[] = {4.2, 8.5};
    for (int i = 0; i < 2; i++) {
        cairo_new_sub_path(cr);
        cairo_arc(cr, cx, cy, radios[i], -3.1 * M_PI / 4.0, -0.9 * M_PI / 4.0);
        double current_alpha = (bars >= i + 3) ? alpha : alpha * 0.3;
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, current_alpha);
        cairo_stroke(cr);
    }
}

void dibujar_rayo(cairo_t *cr, double cx, double cy, double pulse, double alpha) {
    double s = 1.40 + 0.30 * pulse;
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, -s, s);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, alpha);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_width(cr, 1.2);
    cairo_move_to(cr, -1.4, -3.6);
    cairo_line_to(cr, 1.1, -0.6);
    cairo_line_to(cr, -0.4, -0.6);
    cairo_line_to(cr, 1.6, 3.6);
    cairo_line_to(cr, -1.1, 0.6);
    cairo_line_to(cr, 0.4, 0.6);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void dibujar_burst(cairo_t *cr, double x, double y, double b_w, double b_h, double progress, double alpha) {
    double cx = x + b_w / 2.0 + 1.0;
    double cy = y + b_h / 2.0;
    double r = 8.0 + 22.0 * progress;
    double a = alpha * (1.0 - progress * 0.5);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, a);
    cairo_set_line_width(cr, 2.5);
    cairo_arc(cr, cx, cy, r, 0, 2*M_PI);
    cairo_stroke(cr);
    if (progress > 0.25) {
        double p2 = (progress - 0.25) / 0.75;
        double r2 = 8.0 + 22.0 * p2;
        double a2 = alpha * (1.0 - p2) * 0.9;
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, a2);
        cairo_set_line_width(cr, 1.5);
        cairo_arc(cr, cx, cy, r2, 0, 2*M_PI);
        cairo_stroke(cr);
    }
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

    int bat = get_battery_level();
    int wifi = get_wifi_strength();
    int charging = prev_charging;
    double pulse = charging ? (sin(pulse_phase * 2.0 * M_PI) + 1.0) / 2.0 : 0.0;
    double burst_progress = (burst_ticks > 0) ? (1.0 - (double)burst_ticks / 12.0) : -1.0;
    double bx = (charging ? 35.0 : 36.0), by = (h - 10)/2.0 - 1.0, bw = 20.0, bh = 10.0;
    double rayo_cx = (charging ? 64.0 : 67.0);
    double rayo_cy = h / 2.0;

    cairo_surface_t *glow = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)w, (int)h);
    cairo_t *cr_glow = cairo_create(glow);
    cairo_set_antialias(cr_glow, CAIRO_ANTIALIAS_BEST);
    dibujar_wifi(cr_glow, 13.0, (h - 12)/2.0, wifi, 0.95);
    dibujar_bateria(cr_glow, bx, by, bat, charging ? 1.0 : 0.55);
    if (charging) dibujar_rayo(cr_glow, rayo_cx, rayo_cy, pulse, 1.0);
    if (burst_progress >= 0) dibujar_burst(cr_glow, bx, by, bw, bh, burst_progress, 1.0);
    blur_surface(glow, 5);
    blur_surface(glow, 5);
    cairo_set_source_surface(cr, glow, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr_glow);
    cairo_surface_destroy(glow);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
    dibujar_wifi(cr, 13.0, (h - 12)/2.0, wifi, 0.83);
    dibujar_bateria(cr, bx, by, bat, 0.65);
    if (charging) dibujar_rayo(cr, rayo_cx, rayo_cy, pulse, 0.75);

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

gboolean update_info(gpointer data) {
    if (prev_charging == 1) {
        pulse_phase += 0.011;
        if (pulse_phase >= 1.0) pulse_phase -= 1.0;
        gtk_widget_queue_draw(GTK_WIDGET(data));
    }
    if (burst_ticks > 0) {
        burst_ticks--;
        gtk_widget_queue_draw(GTK_WIDGET(data));
    }
    static int last_bat = -1;
    static int last_wifi = -1;
    int bat = get_battery_level();
    int wifi = get_wifi_strength();
    if (bat != last_bat || wifi != last_wifi) {
        last_bat = bat;
        last_wifi = wifi;
        gtk_widget_queue_draw(GTK_WIDGET(data));
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    detectar_hardware();
    ac_online_cache = leer_entero(ac_online_path, 0);
    if (bat_status_path[0]) {
        FILE *f = fopen(bat_status_path, "r");
        if (f) {
            char buf[32] = {0};
            if (fgets(buf, sizeof(buf), f) && strncmp(buf, "Full", 4) == 0) bat_full_cache = 1;
            fclose(f);
        }
    }
    prev_charging = get_charging_efectivo();

    gtk_init(&argc, &argv);
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_wmclass(GTK_WINDOW(window), "bote_wifi_bat", "BoteWifiBat");

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
    setup_udev_monitor(window);
    g_timeout_add(16, update_info, window);
    gtk_widget_show_all(window);
    gtk_main();

    if (udev_watch_id > 0) g_source_remove(udev_watch_id);
    if (udev_mon) udev_monitor_unref(udev_mon);
    if (udev_ctx) udev_unref(udev_ctx);
    return 0;
}
