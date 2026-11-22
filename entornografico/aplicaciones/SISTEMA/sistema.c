#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static GtkWidget *sunmoon_area;
static GtkWidget *aa_area;
static GtkWidget *status_lbl;
static int theme_level = 1;
static int zoom_level  = 3;

static const char* state_path(const char *n) {
    static char p[512];
    snprintf(p, sizeof(p), "%s/.local/state/actriz/%s",
             getenv("HOME") ? getenv("HOME") : ".", n);
    return p;
}
static int read_int(const char *n, int def, int lo, int hi) {
    FILE *f = fopen(state_path(n), "r");
    int v = def;
    if (f) { if (fscanf(f, "%d", &v) != 1) v = def; fclose(f); }
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return v;
}

/* Sol + Luna superpuestos, sol dentro de la concavidad lunar */
static gboolean draw_sunmoon(GtkWidget *w, cairo_t *cr, gpointer d) {
    int height = gtk_widget_get_allocated_height(w);
    double pct = theme_level / 5.0;
    double moon_a = 1.0 - pct;
    double sun_a  = pct;
    double cy = height / 2.0;

    if (sun_a > 0.01) {
        cairo_set_source_rgba(cr, 1.0, 0.82, 0.0, sun_a);
        cairo_set_line_width(cr, 1.2);
        for (int i = 0; i < 8; i++) {
            double ang = i * M_PI / 4.0;
            double x1 = 24 + cos(ang) * 6;
            double y1 = cy + sin(ang) * 6;
            double x2 = 24 + cos(ang) * 9;
            double y2 = cy + sin(ang) * 9;
            cairo_move_to(cr, x1, y1);
            cairo_line_to(cr, x2, y2);
        }
        cairo_stroke(cr);
        cairo_arc(cr, 24, cy, 5, 0, 2*M_PI);
        cairo_fill(cr);
    }

    if (moon_a > 0.01) {
        cairo_set_source_rgba(cr, 0.88, 0.88, 0.98, moon_a);
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
        cairo_arc(cr, 28, cy, 7, 0, 2*M_PI);
        cairo_new_sub_path(cr);
        cairo_arc(cr, 22, cy, 6, 0, 2*M_PI);
        cairo_fill(cr);
    }
    return FALSE;
}

/* Dos A mayusculas: 10 px + 16 px */
static gboolean draw_aa(GtkWidget *w, cairo_t *cr, gpointer d) {
    int height = gtk_widget_get_allocated_height(w);
    cairo_set_source_rgb(cr, 0.92, 0.92, 0.92);

    PangoLayout *layout = pango_cairo_create_layout(cr);

    PangoFontDescription *fd = pango_font_description_from_string("Sans Bold");
    pango_font_description_set_absolute_size(fd, 10 * PANGO_SCALE);
    pango_layout_set_font_description(layout, fd);
    pango_font_description_free(fd);
    pango_layout_set_text(layout, "A", -1);
    int tw, th;
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(cr, 6, (height - th) / 2.0);
    pango_cairo_show_layout(cr, layout);

    fd = pango_font_description_from_string("Sans Bold");
    pango_font_description_set_absolute_size(fd, 16 * PANGO_SCALE);
    pango_layout_set_font_description(layout, fd);
    pango_font_description_free(fd);
    pango_layout_set_text(layout, "A", -1);
    pango_layout_get_pixel_size(layout, &tw, &th);
    cairo_move_to(cr, 22, (height - th) / 2.0);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    return FALSE;
}

/* Slider custom: 100x40, sin bola, estilo iOS */
static gboolean on_slider_draw(GtkWidget *w, cairo_t *cr, gpointer data) {
    int val = *(int*)data;
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);
    double radius = height / 2.0;

    /* Fondo solido oscuro (#1E2633) */
    cairo_set_source_rgb(cr, 45.0/255.0, 50.0/255.0, 56.0/255.0);
    cairo_new_sub_path(cr);
    cairo_arc(cr, width - radius, radius, radius, -M_PI/2, M_PI/2);
    cairo_arc(cr, radius, radius, radius, M_PI/2, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_fill(cr);

    /* Borde sutil */
    cairo_set_source_rgba(cr, 1, 1, 1, 0.15);
    cairo_set_line_width(cr, 1.5);
    cairo_new_sub_path(cr);
    cairo_arc(cr, width - radius, radius, radius - 1, -M_PI/2, M_PI/2);
    cairo_arc(cr, radius, radius, radius - 1, M_PI/2, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_stroke(cr);

    /* Recorte para el relleno izquierdo */
    cairo_save(cr);
    cairo_new_sub_path(cr);
    cairo_arc(cr, width - radius, radius, radius, -M_PI/2, M_PI/2);
    cairo_arc(cr, radius, radius, radius, M_PI/2, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_clip(cr);

    /* Relleno izquierdo: azul grisáceo claro */
    double seg_w = width / 9.0;
    double fill_w = round((val + 1) * seg_w);
    cairo_set_source_rgb(cr, 168.0/255.0, 199.0/255.0, 250.0/255.0);
    cairo_rectangle(cr, 0, 0, fill_w, height);
    cairo_fill(cr);

    /* Líneas divisorias */
    cairo_set_source_rgba(cr, 1, 1, 1, 0.15);
    cairo_set_line_width(cr, 1);
    for (int i = 1; i < 9; i++) {
        double x = (width * i) / 6.0;
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, height);
        cairo_stroke(cr);
    }
    cairo_restore(cr);

    return FALSE;
}

static gboolean on_slider_click(GtkWidget *w, GdkEventButton *e, gpointer data) {
    int *val_ptr = (int*)data;
    int width = gtk_widget_get_allocated_width(w);
    int val = (int)((e->x / (double)width) * 9);
    if (val < 0) val = 0;
    if (val > 8) val = 8;
    if (*val_ptr != val) {
        *val_ptr = val;
        gtk_widget_queue_draw(w);
        if (data == &theme_level) gtk_widget_queue_draw(sunmoon_area);
        if (data == &zoom_level) gtk_widget_queue_draw(aa_area);
    }
    return TRUE;
}

static gboolean on_slider_drag(GtkWidget *w, GdkEventMotion *e, gpointer data) {
    if (!(e->state & GDK_BUTTON1_MASK)) return FALSE;
    int *val_ptr = (int*)data;
    int width = gtk_widget_get_allocated_width(w);
    int val = (int)((e->x / (double)width) * 6);
    if (val < 0) val = 0;
    if (val > 5) val = 5;
    if (*val_ptr != val) {
        *val_ptr = val;
        gtk_widget_queue_draw(w);
        if (data == &theme_level) gtk_widget_queue_draw(sunmoon_area);
        if (data == &zoom_level) gtk_widget_queue_draw(aa_area);
    }
    return TRUE;
}

static void on_apply(GtkWidget *w, gpointer d) {
    int pct  = (theme_level * 100) / 8;
    int zoom = zoom_level + 1;
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "/opt/actriz/bin/set-theme.sh %d && /opt/actriz/bin/set-zoom.sh %d",
        pct, zoom);
    int rc = system(cmd);
    if (rc == 0)
        gtk_label_set_markup(GTK_LABEL(status_lbl),
            "<span foreground='#27AE60'>Aplicado.</span>");
    else
        gtk_label_set_markup(GTK_LABEL(status_lbl),
            "<span foreground='#C0392B'>Error al aplicar.</span>");
}

static void abrir_wifi(GtkWidget *w, gpointer d) {
    g_spawn_command_line_async("alacritty -e nmtui", NULL);
}
static void abrir_bluetooth(GtkWidget *w, gpointer d) {
    g_spawn_command_line_async("alacritty -e bluetoothctl", NULL);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    int pct_saved = read_int("tema.txt", 20, 0, 100);
    theme_level = (pct_saved * 8) / 100;
    if (theme_level > 8) theme_level = 8;
    int z_saved = read_int("zoom.txt", 3, 1, 6);
    zoom_level = z_saved - 1;
    if (zoom_level > 8) zoom_level = 8;

    GtkCssProvider *css = gtk_css_provider_new();
    const char *css_data =
        "button.apply-btn {"
        "  background: #007AFF; color: #FFFFFF; border: none;"
        "  border-radius: 10px; padding: 10px; font-weight: bold; }"
        "button.apply-btn:hover { background: #0060CC; }";
    gtk_css_provider_load_from_data(css, css_data, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Sistema - Actriz OS");
    gtk_window_set_default_size(GTK_WINDOW(window), 620, 440);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), notebook);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_container_set_border_width(GTK_CONTAINER(box), 24);

    /* Tema */
    GtkWidget *l_t = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(l_t), "<b>Tema</b>");
    gtk_widget_set_halign(l_t, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), l_t, FALSE, FALSE, 0);

    GtkWidget *row_t = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_size_request(row_t, -1, 32);

    sunmoon_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(sunmoon_area, 50, 32);
    gtk_widget_set_valign(sunmoon_area, GTK_ALIGN_CENTER);
    g_signal_connect(sunmoon_area, "draw", G_CALLBACK(draw_sunmoon), NULL);
    gtk_box_pack_start(GTK_BOX(row_t), sunmoon_area, FALSE, FALSE, 0);

    GtkWidget *slider_t = gtk_drawing_area_new();
    gtk_widget_set_size_request(slider_t, 130, 32);
    gtk_widget_set_valign(slider_t, GTK_ALIGN_CENTER);
    gtk_widget_add_events(slider_t, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(slider_t, "draw", G_CALLBACK(on_slider_draw), &theme_level);
    g_signal_connect(slider_t, "button-press-event", G_CALLBACK(on_slider_click), &theme_level);
    g_signal_connect(slider_t, "motion-notify-event", G_CALLBACK(on_slider_drag), &theme_level);
    gtk_box_pack_start(GTK_BOX(row_t), slider_t, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), row_t, FALSE, FALSE, 0);

    /* Zoom */
    GtkWidget *l_z = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(l_z), "<b>Zoom</b>");
    gtk_widget_set_halign(l_z, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), l_z, FALSE, FALSE, 0);

    GtkWidget *row_z = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_size_request(row_z, -1, 32);

    aa_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(aa_area, 50, 32);
    gtk_widget_set_valign(aa_area, GTK_ALIGN_CENTER);
    g_signal_connect(aa_area, "draw", G_CALLBACK(draw_aa), NULL);
    gtk_box_pack_start(GTK_BOX(row_z), aa_area, FALSE, FALSE, 0);

    GtkWidget *slider_z = gtk_drawing_area_new();
    gtk_widget_set_size_request(slider_z, 130, 32);
    gtk_widget_set_valign(slider_z, GTK_ALIGN_CENTER);
    gtk_widget_add_events(slider_z, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(slider_z, "draw", G_CALLBACK(on_slider_draw), &zoom_level);
    g_signal_connect(slider_z, "button-press-event", G_CALLBACK(on_slider_click), &zoom_level);
    g_signal_connect(slider_z, "motion-notify-event", G_CALLBACK(on_slider_drag), &zoom_level);
    gtk_box_pack_start(GTK_BOX(row_z), slider_z, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), row_z, FALSE, FALSE, 0);

    /* Aplicar */
    GtkWidget *btn = gtk_button_new_with_label("Aplicar cambios");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "apply-btn");
    gtk_widget_set_size_request(btn, -1, 46);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_apply), NULL);
    gtk_box_pack_end(GTK_BOX(box), btn, FALSE, FALSE, 0);

    status_lbl = gtk_label_new("");
    gtk_box_pack_end(GTK_BOX(box), status_lbl, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, gtk_label_new("Apariencia"));

    GtkWidget *bw = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(bw), 20);
    GtkWidget *btn_wifi = gtk_button_new_with_label("Gestionar Redes WiFi (nmtui)");
    gtk_box_pack_start(GTK_BOX(bw), btn_wifi, FALSE, FALSE, 0);
    g_signal_connect(btn_wifi, "clicked", G_CALLBACK(abrir_wifi), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), bw, gtk_label_new("Red WiFi"));

    GtkWidget *bb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(bb), 20);
    GtkWidget *btn_bt = gtk_button_new_with_label("Gestionar Bluetooth");
    gtk_box_pack_start(GTK_BOX(bb), btn_bt, FALSE, FALSE, 0);
    g_signal_connect(btn_bt, "clicked", G_CALLBACK(abrir_bluetooth), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), bb, gtk_label_new("Bluetooth"));

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
