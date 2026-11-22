#include <gtk/gtk.h>
#include <vte/vte.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <limits.h>

static gboolean safe_label_set_markup(GtkWidget *w, const char *markup) {
    if (!w || !GTK_IS_LABEL(w)) {
        fprintf(stderr, "[abc-term] WARN: label invalido (%p)\n", (void*)w);
        return FALSE;
    }
    gtk_label_set_markup(GTK_LABEL(w), markup);
    return TRUE;
}

static gboolean safe_queue_draw(GtkWidget *w) {
    if (!w || !GTK_IS_WIDGET(w)) {
        fprintf(stderr, "[abc-term] WARN: widget invalido (%p)\n", (void*)w);
        return FALSE;
    }
    gtk_widget_queue_draw(w);
    return TRUE;
}
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/statvfs.h>
#include <dirent.h>

static GtkWidget *window;
static GtkWidget *top_overlay;
static GtkWidget *dir_label;
static GtkWidget *tree_label;
static GtkWidget *sidebar_btn_abc;
static GtkWidget *sidebar_btn_term;
static GtkWidget *stack;
static GtkWidget *text_view;
static GtkWidget *overlay;
static GtkWidget *vte_terminal;
static GtkTextBuffer *buffer;
static GtkTextMark *input_mark;

static int level_tema = 1;
static int level_brillo = 4;
static int level_volumen = 4;
static int level_zoom = 4;
static gdouble ambient_time = 0.0;
static double blob_intensity = 0.0;
static double blob_target = 1.0;

static gboolean do_switch_to_abc(gpointer data) {
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "abc");
    gtk_widget_grab_focus(text_view);
    return FALSE;
}

static void switch_tab(GtkWidget *widget, gpointer data) {
    const gchar *cur = gtk_stack_get_visible_child_name(GTK_STACK(stack));
    if (cur && g_strcmp0(cur, "abc") == 0) {
        blob_target = 0.0;
        gtk_stack_set_visible_child_name(GTK_STACK(stack), "term");
        gtk_widget_grab_focus(vte_terminal);
        gtk_style_context_add_class(gtk_widget_get_style_context(sidebar_btn_term), "active");
        gtk_style_context_remove_class(gtk_widget_get_style_context(sidebar_btn_abc), "active");
        safe_queue_draw(top_overlay);
    } else {
        blob_intensity = 0.0;
        blob_target = 1.0;
        safe_queue_draw(top_overlay);
        g_timeout_add(100, do_switch_to_abc, NULL);
        gtk_style_context_add_class(gtk_widget_get_style_context(sidebar_btn_abc), "active");
        gtk_style_context_remove_class(gtk_widget_get_style_context(sidebar_btn_term), "active");
    }
}

static char* strip_ansi(const char* input) {
    char* output = g_malloc(strlen(input) + 1);
    int i = 0, j = 0;
    while (input[i]) {
        if (input[i] == '\033' && input[i+1] == '[') {
            i += 2;
            while (input[i] && (input[i] < 'A' || input[i] > 'Z')) i++;
            if (input[i]) i++;
        } else {
            output[j++] = input[i++];
        }
    }
    output[j] = '\0';
    return output;
}

static void append_prompt() {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);

    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    char *dot = strchr(hostname, '.');
    if (dot) *dot = '\0';

    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL) strcpy(cwd, "?");
    const char *home = getenv("HOME");
    char path[512];
    if (home && strncmp(cwd, home, strlen(home)) == 0) {
        snprintf(path, sizeof(path), "~%s", cwd + strlen(home));
    } else {
        strcpy(path, cwd);
    }
    const char *user = getenv("USER");
    if (!user) user = "magnum";

    GtkTextTag *tag_user = gtk_text_buffer_create_tag(buffer, NULL, "foreground", "#4A90E2", "weight", PANGO_WEIGHT_BOLD, NULL);
    GtkTextTag *tag_at   = gtk_text_buffer_create_tag(buffer, "at", "foreground", "#F1C40F", "weight", PANGO_WEIGHT_BOLD, NULL);
    GtkTextTag *tag_def  = gtk_text_buffer_create_tag(buffer, NULL, "foreground", "#E6EAF0", NULL);

    char u_h[512];
    snprintf(u_h, sizeof(u_h), "%s@%s", user, hostname);
    gtk_text_buffer_insert_with_tags(buffer, &end, u_h, -1, tag_user, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, "/abc", -1, gtk_text_buffer_create_tag(buffer, NULL, "foreground", "#F06BFF", "weight", PANGO_WEIGHT_BOLD, NULL), NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, ":", -1, tag_def, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, path, -1, tag_def, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, "$ ", -1, tag_def, NULL);

    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_move_mark(buffer, input_mark, &end);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(text_view), input_mark);
}

static void draw_glow(cairo_t *cr, double cx, double cy, double radius, double r, double g, double b, double peak_alpha) {
    cairo_pattern_t *pat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, radius);
    cairo_pattern_add_color_stop_rgba(pat, 0.00, r, g, b, peak_alpha * 1.000);
    cairo_pattern_add_color_stop_rgba(pat, 0.05, r, g, b, peak_alpha * 0.992);
    cairo_pattern_add_color_stop_rgba(pat, 0.10, r, g, b, peak_alpha * 0.969);
    cairo_pattern_add_color_stop_rgba(pat, 0.15, r, g, b, peak_alpha * 0.931);
    cairo_pattern_add_color_stop_rgba(pat, 0.20, r, g, b, peak_alpha * 0.881);
    cairo_pattern_add_color_stop_rgba(pat, 0.25, r, g, b, peak_alpha * 0.820);
    cairo_pattern_add_color_stop_rgba(pat, 0.30, r, g, b, peak_alpha * 0.751);
    cairo_pattern_add_color_stop_rgba(pat, 0.35, r, g, b, peak_alpha * 0.677);
    cairo_pattern_add_color_stop_rgba(pat, 0.40, r, g, b, peak_alpha * 0.599);
    cairo_pattern_add_color_stop_rgba(pat, 0.45, r, g, b, peak_alpha * 0.521);
    cairo_pattern_add_color_stop_rgba(pat, 0.50, r, g, b, peak_alpha * 0.445);
    cairo_pattern_add_color_stop_rgba(pat, 0.55, r, g, b, peak_alpha * 0.372);
    cairo_pattern_add_color_stop_rgba(pat, 0.60, r, g, b, peak_alpha * 0.305);
    cairo_pattern_add_color_stop_rgba(pat, 0.65, r, g, b, peak_alpha * 0.244);
    cairo_pattern_add_color_stop_rgba(pat, 0.70, r, g, b, peak_alpha * 0.190);
    cairo_pattern_add_color_stop_rgba(pat, 0.75, r, g, b, peak_alpha * 0.142);
    cairo_pattern_add_color_stop_rgba(pat, 0.80, r, g, b, peak_alpha * 0.102);
    cairo_pattern_add_color_stop_rgba(pat, 0.85, r, g, b, peak_alpha * 0.068);
    cairo_pattern_add_color_stop_rgba(pat, 0.90, r, g, b, peak_alpha * 0.040);
    cairo_pattern_add_color_stop_rgba(pat, 0.95, r, g, b, peak_alpha * 0.018);
    cairo_pattern_add_color_stop_rgba(pat, 1.00, r, g, b, 0.000);
    cairo_set_source(cr, pat);
    cairo_arc(cr, cx, cy, radius, 0, 2*M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
}

static void update_dir_label() {
    if (!dir_label) return;
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL) strcpy(cwd, "?");
    const char *home = getenv("HOME");
    char path[512];
    if (home && strncmp(cwd, home, strlen(home)) == 0) {
        snprintf(path, sizeof(path), "~%s", cwd + strlen(home));
    } else {
        strcpy(path, cwd);
    }
    char markup[640];
    snprintf(markup, sizeof(markup), "<span foreground='#6CB4EE'>abc</span> <span foreground='#666'>/</span> <span foreground='#D8DCE3'>%s</span>", path);
    safe_label_set_markup(dir_label, markup);
}

static void build_tree_rec(const char *base, const char *prefix, int depth, GString *out) {
    if (depth > 1) return;
    struct dirent **namelist;
    int n = scandir(base, &namelist, NULL, alphasort);
    if (n < 0) return;
    int count = 0;
    for (int i = 0; i < n; i++) if (namelist[i]->d_name[0] != '.') count++;
    int idx = 0;
    for (int i = 0; i < n; i++) {
        if (namelist[i]->d_name[0] == '.') { free(namelist[i]); continue; }
        idx++;
        int is_last = (idx == count);
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", base, namelist[i]->d_name);
        struct stat st;
        int is_dir = (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
        const char *color = is_dir ? "#6CB4EE" : "#999";
        const char *branch = is_last ? "└── " : "├── ";
        char *escaped_name = g_markup_escape_text(namelist[i]->d_name, -1);
        g_string_append_printf(out, "%s<span foreground='%s'>%s%s</span>\n", prefix, color, branch, escaped_name);
        g_free(escaped_name);
        if (is_dir) {
            char new_prefix[512];
            snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");
            build_tree_rec(path, new_prefix, depth + 1, out);
        }
        free(namelist[i]);
    }
    free(namelist);
}

static void update_tree_label() {
    if (!tree_label) return;
    GString *s = g_string_new("");
    g_string_append(s, "<span foreground='#D8DCE3' weight='bold'>/opt/actriz</span>\n");
    build_tree_rec("/opt/actriz", "", 0, s);
    safe_label_set_markup(tree_label, s->str);
    g_string_free(s, TRUE);
}

static gboolean dir_timer_cb(gpointer data) {
    update_dir_label();
    return TRUE;
}

static double light_progress(double base, double delay) {
    double v = (base - delay) / (1.0 - delay);
    if (v <= 0.0) return 0.0;
    if (v >= 1.0) v = 1.0;
    if (blob_target > 0.5) {
        // ease_out_back: overshoot 30% y rebote
        const double c1 = 3.5;
        const double c3 = c1 + 1.0;
        double vv = v - 1.0;
        double r = 1.0 + c3 * vv * vv * vv + c1 * vv * vv;
        return r < 0.0 ? 0.0 : r;
    }
    return v;
}

static gboolean timer_cb(gpointer data) {
    if (!data || !GTK_IS_WIDGET(data)) return FALSE;
    ambient_time += 0.015;
    if (blob_intensity < blob_target) {
        double rem = blob_target - blob_intensity;
        double rate = (rem > 0.55) ? 0.055 : 0.028;
        blob_intensity += 0.025;
        if (blob_intensity > blob_target) blob_intensity = blob_target;
    } else if (blob_intensity > blob_target) {
        double rem = blob_intensity - blob_target;
        double rate = (rem > 0.4) ? 0.025 : 0.055;
        blob_intensity -= 0.020;
        if (blob_intensity < blob_target) blob_intensity = blob_target;
    }
    safe_queue_draw(GTK_WIDGET(data));
    return TRUE;
}

static gboolean on_draw_ambient_real(GtkWidget *w, cairo_t *cr, gpointer data) {
    if (blob_intensity < 0.01 && blob_target < 0.01) return FALSE;
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);
    cairo_set_source_rgb(cr, 18.0/255.0, 18.0/255.0, 18.0/255.0);
    cairo_paint(cr);

    double t = ambient_time;

    double pink_y = height * 0.75 + sin(t * 1.8) * 210 + 100;
    double i_pink = light_progress(blob_intensity, 0.0);
    double pink_r = (800.0 + sin(t * 2.5) * 180) * (0.1 + 0.9 * i_pink);
    cairo_set_operator(cr, CAIRO_OPERATOR_ADD);
    draw_glow(cr, width + 200, pink_y, pink_r, 240.0/255.0, 130.0/255.0, 200.0/255.0, 0.25 * i_pink * (i_pink < 1.0 ? i_pink : 1.0));

    double purple_x = width * 0.85 - sin(t * 1.6) * 210;
    double i_purple = light_progress(blob_intensity, 0.25);
    double purple_r = (700.0 - sin(t * 2.2) * 180) * (0.1 + 0.9 * i_purple);
    draw_glow(cr, purple_x, height + 150, purple_r, 200.0/255.0, 100.0/255.0, 230.0/255.0, 0.32 * i_purple * (i_purple < 1.0 ? i_purple : 1.0));

    double blue_x = width * 0.15 + sin(t * 1.6) * 210;
    double i_blue = light_progress(blob_intensity, 0.7);
    double blue_r = (700.0 - sin(t * 2.2 + 1.5) * 180) * (0.1 + 0.9 * i_blue);
    draw_glow(cr, blue_x, height + 150, blue_r, 100.0/255.0, 170.0/255.0, 230.0/255.0, 0.32 * i_blue * (i_blue < 1.0 ? i_blue : 1.0));
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    return FALSE;
}

static gboolean on_slider_draw(GtkWidget *w, cairo_t *cr, gpointer data) {
    int val = *(int*)data;
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);
    double radius = height / 2.0;

    cairo_set_source_rgb(cr, 45.0/255.0, 50.0/255.0, 56.0/255.0);
    cairo_new_sub_path(cr);
    cairo_arc(cr, width - radius, radius, radius, -M_PI/2, M_PI/2);
    cairo_arc(cr, radius, radius, radius, M_PI/2, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, 1, 1, 1, 0.15);
    cairo_set_line_width(cr, 1.5);
    cairo_new_sub_path(cr);
    cairo_arc(cr, width - radius, radius, radius - 1, -M_PI/2, M_PI/2);
    cairo_arc(cr, radius, radius, radius - 1, M_PI/2, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_stroke(cr);

    cairo_save(cr);
    cairo_new_sub_path(cr);
    cairo_arc(cr, width - radius, radius, radius, -M_PI/2, M_PI/2);
    cairo_arc(cr, radius, radius, radius, M_PI/2, 3*M_PI/2);
    cairo_close_path(cr);
    cairo_clip(cr);

    double seg_w = width / 9.0;
    double fill_w = round((val + 1) * seg_w);
    
    cairo_pattern_t *fill_pat = cairo_pattern_create_linear(0, 0, fill_w, 0);
    cairo_pattern_add_color_stop_rgb(fill_pat, 0.0, 168.0/255.0, 199.0/255.0, 250.0/255.0);
    cairo_pattern_add_color_stop_rgb(fill_pat, 1.0, 140.0/255.0, 170.0/255.0, 214.0/255.0);
    cairo_set_source(cr, fill_pat);
    cairo_rectangle(cr, 0, 0, fill_w, height);
    cairo_fill(cr);
    cairo_pattern_destroy(fill_pat);

    for (int i = 1; i < 9; i++) {
        double x = round((width * i) / 9.0);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.15);
        if (i % 2 != 0) cairo_set_line_width(cr, 2.0);
        else cairo_set_line_width(cr, 0.5);
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
    if (*val_ptr != val) { *val_ptr = val; gtk_widget_queue_draw(w); }
    return TRUE;
}

static gboolean on_slider_drag(GtkWidget *w, GdkEventMotion *e, gpointer data) {
    if (!(e->state & GDK_BUTTON1_MASK)) return FALSE;
    return on_slider_click(w, (GdkEventButton*)e, data);
}

static void cerrar_widget(GtkWidget *w, gpointer data) {
    gtk_widget_destroy(GTK_WIDGET(data));
    gtk_widget_grab_focus(text_view);
}

static void aplicar_ajuste(GtkWidget *w, gpointer data) {
    const char *tipo = (const char*)data;
    GtkWidget *widget_box = gtk_widget_get_parent(GTK_WIDGET(w));
    widget_box = gtk_widget_get_parent(widget_box);
    char cmd[256];
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);

    if (strcmp(tipo, "tema") == 0) {
        snprintf(cmd, sizeof(cmd), "/opt/actriz/bin/set-theme.sh %d", (level_tema * 100) / 8);
        gtk_text_buffer_insert(buffer, &end, "[ABC]: Tema aplicado.\n", -1);
    } else if (strcmp(tipo, "brillo") == 0) {
        snprintf(cmd, sizeof(cmd), "/opt/actriz/drivers/brillo/control.sh cambiar %d", (level_brillo * 100) / 8);
        gtk_text_buffer_insert(buffer, &end, "[ABC]: Brillo aplicado.\n", -1);
    } else if (strcmp(tipo, "volumen") == 0) {
        snprintf(cmd, sizeof(cmd), "/opt/actriz/drivers/audio/control.sh cambiar %d", (level_volumen * 100) / 8);
        gtk_text_buffer_insert(buffer, &end, "[ABC]: Volumen aplicado.\n", -1);
    } else if (strcmp(tipo, "zoom") == 0) {
        snprintf(cmd, sizeof(cmd), "/opt/actriz/bin/set-zoom.sh %d", (level_zoom * 100) / 8);
        gtk_text_buffer_insert(buffer, &end, "[ABC]: Zoom aplicado.\n", -1);
    }
    
    system(cmd);
    gtk_widget_destroy(widget_box);
    gtk_widget_grab_focus(text_view);
    append_prompt();
}

static void mostrar_widget(const char *tipo) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
    gtk_widget_set_size_request(box, 350, 120);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "box { background-color: #212224; border-radius: 12px; border: 1px solid #444; padding: 15px; }"
        "button { background: #007AFF; color: #FFF; border: none; border-radius: 6px; padding: 8px; font-weight: bold; }"
        "button:hover { background: #0060CC; }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(box), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

    char title[64];
    snprintf(title, sizeof(title), "<span color='#FFF'><b>Ajustar %s</b></span>", tipo);
    GtkWidget *lbl = gtk_label_new(NULL);
    safe_label_set_markup(lbl, title);
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);

    int *target_level = &level_tema;
    if (strcmp(tipo, "brillo") == 0) target_level = &level_brillo;
    if (strcmp(tipo, "volumen") == 0) target_level = &level_volumen;
    if (strcmp(tipo, "zoom") == 0) target_level = &level_zoom;

    GtkWidget *slider = gtk_drawing_area_new();
    gtk_widget_set_size_request(slider, 130, 32);
    gtk_widget_set_halign(slider, GTK_ALIGN_CENTER);
    gtk_widget_add_events(slider, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    g_signal_connect(slider, "draw", G_CALLBACK(on_slider_draw), target_level);
    g_signal_connect(slider, "button-press-event", G_CALLBACK(on_slider_click), target_level);
    g_signal_connect(slider, "motion-notify-event", G_CALLBACK(on_slider_drag), target_level);
    gtk_box_pack_start(GTK_BOX(box), slider, FALSE, FALSE, 0);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *btn_cancel = gtk_button_new_with_label("Cancelar");
    GtkWidget *btn_apply = gtk_button_new_with_label("Aplicar");
    
    g_signal_connect(btn_cancel, "clicked", G_CALLBACK(cerrar_widget), box);
    g_signal_connect(btn_apply, "clicked", G_CALLBACK(aplicar_ajuste), (gpointer)tipo);
    
    gtk_box_pack_end(GTK_BOX(hbox), btn_apply, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), btn_cancel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);

    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), box);
    gtk_widget_show_all(box);
}

static gboolean on_vte_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if ((event->state & GDK_CONTROL_MASK) && (event->state & GDK_SHIFT_MASK)) {
        if (event->keyval == GDK_KEY_C || event->keyval == GDK_KEY_c) {
            vte_terminal_copy_clipboard_format(VTE_TERMINAL(widget), VTE_FORMAT_TEXT);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_V || event->keyval == GDK_KEY_v) {
            vte_terminal_paste_clipboard(VTE_TERMINAL(widget));
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if ((event->state & GDK_CONTROL_MASK) && (event->state & GDK_SHIFT_MASK)) {
        if (event->keyval == GDK_KEY_C || event->keyval == GDK_KEY_c) {
            gtk_text_buffer_copy_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
            return TRUE;
        }
        if (event->keyval == GDK_KEY_V || event->keyval == GDK_KEY_v) {
            gtk_text_buffer_paste_clipboard(buffer, gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), NULL, TRUE);
            return TRUE;
        }
    }

    GList *children = gtk_container_get_children(GTK_CONTAINER(overlay));
    if (children && g_list_length(children) > 1) {
        g_list_free(children);
        return FALSE; 
    }

    GtkTextIter cursor, start;
    GtkTextMark *cursor_mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_buffer_get_iter_at_mark(buffer, &cursor, cursor_mark);
    gtk_text_buffer_get_iter_at_mark(buffer, &start, input_mark);

    if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        
        char *cmd = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
        if (strlen(cmd) > 0) {
            if (strcmp(cmd, "shell") == 0) {
                gtk_text_buffer_insert(buffer, &end, "\n", -1);
                switch_tab(NULL, NULL);
                g_free(cmd);
                return TRUE;
            }
            if (strcmp(cmd, "salir") == 0 || strcmp(cmd, "exit") == 0) {
                g_free(cmd);
                gtk_main_quit();
                return TRUE;
            }
            if (strcmp(cmd, "tema") == 0 || strcmp(cmd, "brillo") == 0 || strcmp(cmd, "volumen") == 0 || strcmp(cmd, "zoom") == 0) {
                gtk_text_buffer_insert(buffer, &end, "\n", -1);
                mostrar_widget(cmd);
                g_free(cmd);
                return TRUE;
            }

            char full_cmd[1024];
            snprintf(full_cmd, sizeof(full_cmd), "/opt/actriz/ABC/abc \"%s\"", cmd);
            gchar *stdout_str = NULL;
            GError *error = NULL;
            if (g_spawn_command_line_sync(full_cmd, &stdout_str, NULL, NULL, &error)) {
                if (stdout_str && strlen(stdout_str) > 0) {
                    char *clean_str = strip_ansi(stdout_str);
                    gtk_text_buffer_insert(buffer, &end, "\n", -1);
                    gtk_text_buffer_insert(buffer, &end, clean_str, -1);
                    if (clean_str[strlen(clean_str)-1] != '\n') gtk_text_buffer_insert(buffer, &end, "\n", -1);
                    g_free(clean_str);
                }
            } else {
                gtk_text_buffer_insert(buffer, &end, "\n[Error ejecutando ABC]\n", -1);
                g_error_free(error);
            }
            if (stdout_str) g_free(stdout_str);
        } else {
            gtk_text_buffer_insert(buffer, &end, "\n", -1);
        }
        g_free(cmd);
        append_prompt();
        return TRUE;
    }

    if (event->keyval == GDK_KEY_BackSpace) {
        if (gtk_text_iter_compare(&cursor, &start) <= 0) return TRUE;
        return FALSE;
    }

    if (event->length > 0) {
        if (gtk_text_iter_compare(&cursor, &start) < 0) {
            gtk_text_buffer_place_cursor(buffer, &start);
        }
        return FALSE;
    }

    return FALSE;
}

static gboolean on_draw_ambient(GtkWidget *w, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);
    int half_w = (width + 1) / 2;
    int half_h = (height + 1) / 2;

    cairo_surface_t *small = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, half_w, half_h);
    cairo_t *cr_small = cairo_create(small);
    cairo_scale(cr_small, 0.5, 0.5);

    on_draw_ambient_real(w, cr_small, data);

    cairo_destroy(cr_small);

    cairo_save(cr);
    cairo_scale(cr, 2.0, 2.0);
    cairo_set_source_surface(cr, small, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
    cairo_paint(cr);
    cairo_restore(cr);
    cairo_surface_destroy(small);

    return FALSE;
}

static GtkWidget *sys_label = NULL;
static GtkWidget *sys_scroll = NULL;
static unsigned long long prev_cpu_total = 0, prev_cpu_idle = 0;
static unsigned long long prev_rx = 0, prev_tx = 0;
static double sys_cpu = 0.0;
static void read_system_info_old() {
    char markup[4096];
    unsigned long long mem_total = 0, mem_avail = 0;
    FILE *f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "MemTotal: %llu kB", &mem_total) == 1) continue;
            if (sscanf(line, "MemAvailable: %llu kB", &mem_avail) == 1) continue;
        }
        fclose(f);
    }
    double ram_u = (mem_total - mem_avail) / 1048576.0;
    double ram_t = mem_total / 1048576.0;
    double ram_p = mem_total > 0 ? (100.0 * (mem_total - mem_avail) / mem_total) : 0;
    unsigned long long u=0,n=0,s=0,i=0,w=0,q=0,sq=0;
    FILE *cf = fopen("/proc/stat", "r");
    if (cf) {
        char line[512];
        if (fgets(line, sizeof(line), cf)) {
            sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu", &u, &n, &s, &i, &w, &q, &sq);
        }
        fclose(cf);
    }
    unsigned long long tot = u+n+s+i+w+q+sq;
    unsigned long long dt = tot - prev_cpu_total;
    unsigned long long di = i - prev_cpu_idle;
    if (dt > 0) sys_cpu = 100.0 * (dt - di) / dt;
    prev_cpu_total = tot;
    prev_cpu_idle = i;
    int bat = -1;
    FILE *bf = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (!bf) bf = fopen("/sys/class/power_supply/BAT1/capacity", "r");
    if (bf) { if (fscanf(bf, "%d", &bat) != 1) bat = -1; fclose(bf); }
    int temp = -1;
    FILE *tf = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (tf) { int t; if (fscanf(tf, "%d", &t) == 1) temp = t / 1000; fclose(tf); }
    unsigned long long rx = 0, tx = 0;
    DIR *nd = opendir("/sys/class/net");
    if (nd) {
        struct dirent *e;
        while ((e = readdir(nd))) {
            if (e->d_name[0] == '.' || strcmp(e->d_name, "lo") == 0) continue;
            char pth[512];
            snprintf(pth, sizeof(pth), "/sys/class/net/%s/statistics/rx_bytes", e->d_name);
            FILE *rf = fopen(pth, "r");
            if (rf) { unsigned long long v; if (fscanf(rf, "%llu", &v) == 1) rx += v; fclose(rf); }
            snprintf(pth, sizeof(pth), "/sys/class/net/%s/statistics/tx_bytes", e->d_name);
            FILE *sf = fopen(pth, "r");
            if (sf) { unsigned long long v; if (fscanf(sf, "%llu", &v) == 1) tx += v; fclose(sf); }
        }
        closedir(nd);
    }
    double rxkb = (rx - prev_rx) / 1024.0;
    double txkb = (tx - prev_tx) / 1024.0;
    prev_rx = rx;
    prev_tx = tx;
    struct statvfs st;
    double du = 0, dtot = 0, dp = 0;
    if (statvfs("/", &st) == 0) {
        dtot = (double)st.f_blocks * st.f_frsize / 1073741824.0;
        du = (double)(st.f_blocks - st.f_bfree) * st.f_frsize / 1073741824.0;
        dp = dtot > 0 ? (100.0 * du / dtot) : 0;
    }
    char gpu[32] = "---";
    FILE *gf = fopen("/sys/class/drm/card0/device/vendor", "r");
    if (gf) {
        char vid[16] = {0};
        if (fgets(vid, sizeof(vid), gf)) {
            if (strncmp(vid, "0x1002", 6) == 0) strcpy(gpu, "AMD");
            else if (strncmp(vid, "0x8086", 6) == 0) strcpy(gpu, "Intel");
            else if (strncmp(vid, "0x10de", 6) == 0) strcpy(gpu, "NVIDIA");
        }
        fclose(gf);
    }
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", lt);
    const char *rc = ram_p > 80 ? "#C0392B" : (ram_p > 50 ? "#F39C12" : "#27AE60");
    const char *dc = dp > 85 ? "#C0392B" : (dp > 60 ? "#F39C12" : "#27AE60");
    const char *bc = bat < 0 ? "#888" : (bat < 20 ? "#C0392B" : (bat < 50 ? "#F39C12" : "#27AE60"));
    const char *tc = temp < 0 ? "#888" : (temp > 70 ? "#C0392B" : "#27AE60");
    char bat_str[8], temp_str[8];
    if (bat < 0) strcpy(bat_str, "---"); else snprintf(bat_str, sizeof(bat_str), "%d%%", bat);
    if (temp < 0) strcpy(temp_str, "---"); else snprintf(temp_str, sizeof(temp_str), "%d°C", temp);
    snprintf(markup, sizeof(markup),
        "<span foreground='#F06BFF' weight='bold'>╭─ SISTEMA ─╮</span>\n"
        "<span foreground='#6CB4EE'>│</span> <span foreground='#888'>RAM </span><span foreground='#FFF'>%.1f/%.1fG</span> <span foreground='%s'>%3.0f%%</span>\n"
        "<span foreground='#6CB4EE'>│</span> <span foreground='#888'>CPU </span><span foreground='#FFF'>%3.0f%%</span>\n"
        "<span foreground='#6CB4EE'>│</span> <span foreground='#888'>ROM </span><span foreground='#FFF'>%.1f/%.1fG</span> <span foreground='%s'>%3.0f%%</span>\n"
        "<span foreground='#6CB4EE'>│</span> <span foreground='#888'>GPU </span><span foreground='#FFF'>%s</span>\n"
        "<span foreground='#6CB4EE'>│</span> <span foreground='#888'>BAT </span><span foreground='%s'>%s</span>\n"
        "<span foreground='#6CB4EE'>│</span> <span foreground='#888'>TEMP</span> <span foreground='%s'>%s</span>\n"
        "<span foreground='#6CB4EE'>│</span> <span foreground='#888'>NET </span><span foreground='#27AE60'>↓%.1f</span> <span foreground='#C0392B'>↑%.1f</span> <span foreground='#888'>KB/s</span>\n"
        "<span foreground='#F06BFF' weight='bold'>╰────────────╯</span>\n"
        "<span foreground='#6CB4EE' weight='bold'>      %s      </span>",
        ram_u, ram_t, rc, ram_p,
        sys_cpu,
        du, dtot, dc, dp,
        gpu,
        bc, bat_str,
        tc, temp_str,
        rxkb, txkb,
        time_str
    );
    gtk_label_set_markup(GTK_LABEL(sys_label), markup);
}
static gboolean scroll_to_bottom(gpointer data) {
    if (!sys_scroll) return FALSE;
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sys_scroll));
    if (vadj) gtk_adjustment_set_value(vadj, gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_page_size(vadj));
    return FALSE;
}

static gboolean draw_fade(GtkWidget *w, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);
    cairo_pattern_t *pat = cairo_pattern_create_linear(0, 0, 0, height);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 25.0/255.0, 25.0/255.0, 25.0/255.0, 0.85);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 18.0/255.0, 18.0/255.0, 18.0/255.0, 0.0);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
    return FALSE;
}

static void read_system_info() {
    if (!sys_label) return;
    char *dir = NULL;
    char proc_path[64];
    char cwd_buf[512];

    VtePty *pty = vte_terminal_get_pty(VTE_TERMINAL(vte_terminal));
    if (pty) {
        int fd = vte_pty_get_fd(pty);
        if (fd >= 0) {
            pid_t pgid = tcgetpgrp(fd);
            if (pgid > 0) {
                snprintf(proc_path, sizeof(proc_path), "/proc/%d/cwd", (int)pgid);
                ssize_t len = readlink(proc_path, cwd_buf, sizeof(cwd_buf) - 1);
                if (len > 0) {
                    cwd_buf[len] = '\0';
                    dir = g_strdup(cwd_buf);
                }
            }
        }
    }
    if (!dir) dir = g_strdup(g_get_home_dir());
    char *quoted = g_shell_quote(dir);
    char *cmd = g_strdup_printf("/opt/actriz/bin/tree.sh %s", quoted);
    gchar *out = NULL;
    if (g_spawn_command_line_sync(cmd, &out, NULL, NULL, NULL)) {
        if (out && strlen(out) > 0) gtk_label_set_markup(GTK_LABEL(sys_label), out);
    }
    if (out) g_free(out);
    g_free(cmd);
    g_free(quoted);
    g_free(dir);
}
static gboolean sys_timer_cb(gpointer data) { read_system_info(); return TRUE; }
int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    srand(time(NULL));
    srand(time(NULL));
    srand(time(NULL));
    g_object_set(gtk_settings_get_default(), "gtk-error-bell", FALSE, NULL);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "abc-term");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window { background-color: #121212; border: none; outline: none; box-shadow: none; }\n"
        "notebook, stack, box, overlay, hbox, sidebar, viewport, .background { border: none; box-shadow: none; background-color: transparent; background-image: none; padding: 0; margin: 0; }\n"
        "notebook > stack { border: none; box-shadow: none; }\n"
        "textview, textview text { background-color: transparent; color: #D8DCE3; font-family: 'Iosevka', monospace; font-size: 18px; padding: 0; }\n"
        "vte-terminal { font-family: 'Courier Prime', monospace; font-size: 21.69px; padding: 20px; padding-top: 10px; padding-left: 12px; }\n"
        "#sidebar, #sidebar > *, #tabs-row, #tabs-row > * { background-color: transparent; background-image: none; border: none; box-shadow: none; }\n"
        "notebook { border: none; box-shadow: none; }\n"
        "notebook > header { border: none; box-shadow: none; background: transparent; }\n"
        "#tabs-row { padding: 10px 5px; border: none; }\n"
        "#tabs-row button { background-image: linear-gradient(to bottom, rgba(80,80,100,0.4), rgba(40,40,55,0.4)); border: 1px solid rgba(255,255,255,0.06); outline: none; border-image: none; -gtk-outline-radius: 0; color: #888; font-family: 'Iosevka', monospace; font-size: 14px; font-weight: bold; min-width: 100px; min-height: 36px; border-radius: 8px; margin: 0 2px; padding: 0; text-shadow: none; -gtk-icon-shadow: none; box-shadow: inset 0 1px 0 rgba(255,255,255,0.10), 0 1px 3px rgba(0,0,0,0.3); }\n"
        "#tabs-row button:hover { color: #CCC; background-image: linear-gradient(to bottom, rgba(100,100,130,0.5), rgba(60,60,80,0.5)); border-color: rgba(255,255,255,0.12); }\n"
        "#tabs-row button.active { color: #FFFFFF; background-image: linear-gradient(to bottom, rgba(207,168,255,0.25), rgba(108,180,238,0.15)); border: 1px solid rgba(207,168,255,0.35); text-shadow: 0 0 6px rgba(207,168,255,0.6), 0 0 14px rgba(207,168,255,0.25); box-shadow: inset 0 1px 0 rgba(255,255,255,0.15), 0 0 12px rgba(207,168,255,0.25), 0 1px 4px rgba(0,0,0,0.4); }\n"
        "#tabs-row button:focus, #tabs-row button:focus-within, #tabs-row button:active { outline: none; }\n"
        "#tree-label { font-family: 'Iosevka', monospace; font-size: 11px; background: transparent; border: none; }\n"
        "#dir-label { color: #D8DCE3; font-family: 'Iosevka', monospace; font-size: 12px; padding: 15px 10px; border: none; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);

    top_overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(window), top_overlay);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_top(hbox, 0);
    gtk_widget_set_margin_start(hbox, 0);
    gtk_widget_set_margin_end(hbox, 0);
    gtk_widget_set_margin_bottom(hbox, 0);
    gtk_container_add(GTK_CONTAINER(top_overlay), hbox);

    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(sidebar, "sidebar");
    gtk_widget_set_size_request(sidebar, 230, -1);
    gtk_box_pack_start(GTK_BOX(hbox), sidebar, FALSE, FALSE, 0);

    // Fila horizontal con los dos botones de pestaña
    GtkWidget *tabs_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(tabs_row, "tabs-row");
    gtk_box_pack_start(GTK_BOX(sidebar), tabs_row, FALSE, FALSE, 0);

    // Primero TERM (izquierda)
    sidebar_btn_term = gtk_button_new_with_label(">_");
    g_signal_connect(sidebar_btn_term, "clicked", G_CALLBACK(switch_tab), NULL);
    gtk_widget_set_hexpand(sidebar_btn_term, TRUE);
    gtk_box_pack_start(GTK_BOX(tabs_row), sidebar_btn_term, TRUE, TRUE, 0);

    // Luego ABC (derecha) - activo por defecto
    sidebar_btn_abc = gtk_button_new_with_label("ABC");
    gtk_style_context_add_class(gtk_widget_get_style_context(sidebar_btn_abc), "active");
    g_signal_connect(sidebar_btn_abc, "clicked", G_CALLBACK(switch_tab), NULL);
    gtk_widget_set_hexpand(sidebar_btn_abc, TRUE);
    gtk_box_pack_start(GTK_BOX(tabs_row), sidebar_btn_abc, TRUE, TRUE, 0);

    sys_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sys_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(sys_scroll, TRUE);
    sys_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(sys_label), 0.0);
    gtk_widget_set_halign(sys_label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(sys_label, 2);
    gtk_widget_set_margin_end(sys_label, 2);
    gtk_widget_set_margin_top(sys_label, 3);
    gtk_widget_set_margin_bottom(sys_label, 3);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_family_new("Noto Sans Mono"));
    pango_attr_list_insert(attrs, pango_attr_size_new(9728));
    gtk_label_set_attributes(GTK_LABEL(sys_label), attrs);
    pango_attr_list_unref(attrs);
    gtk_container_add(GTK_CONTAINER(sys_scroll), sys_label);
    GtkWidget *sys_overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(sys_overlay), sys_scroll);
    GtkWidget *fade = gtk_drawing_area_new();
    gtk_widget_set_size_request(fade, -1, 12);
    gtk_widget_set_halign(fade, GTK_ALIGN_FILL);
    gtk_widget_set_valign(fade, GTK_ALIGN_START);
    g_signal_connect(fade, "draw", G_CALLBACK(draw_fade), NULL);
    gtk_overlay_add_overlay(GTK_OVERLAY(sys_overlay), fade);
    gtk_widget_set_vexpand(sys_overlay, TRUE);
    gtk_box_pack_end(GTK_BOX(sidebar), sys_overlay, TRUE, TRUE, 0);
    read_system_info();
    g_timeout_add(2000, sys_timer_cb, NULL);


    stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 280);
    gtk_box_pack_start(GTK_BOX(hbox), stack, TRUE, TRUE, 0);

    // Pestaña 0: ABC
    overlay = gtk_overlay_new();
    text_view = gtk_text_view_new();
    gtk_widget_set_margin_start(text_view, 12);
    gtk_widget_set_margin_end(text_view, 12);
    gtk_widget_set_margin_top(text_view, 18);
    gtk_widget_set_margin_bottom(text_view, 12);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
    gtk_container_add(GTK_CONTAINER(overlay), text_view);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    input_mark = gtk_text_buffer_create_mark(buffer, "input", &end, TRUE);
    g_signal_connect(text_view, "key-press-event", G_CALLBACK(on_key_press), NULL);
    
    g_signal_connect(top_overlay, "draw", G_CALLBACK(on_draw_ambient), NULL);
    g_timeout_add(30, timer_cb, top_overlay);

    gtk_stack_add_named(GTK_STACK(stack), overlay, "abc");

    // Pestaña 1: Shell (VTE)
    vte_terminal = vte_terminal_new();
    vte_terminal_set_audible_bell(VTE_TERMINAL(vte_terminal), FALSE);
    gtk_widget_set_margin_start(vte_terminal, 0);
    gtk_widget_set_margin_end(vte_terminal, 12);
    gtk_widget_set_margin_top(vte_terminal, 12);
    gtk_widget_set_margin_bottom(vte_terminal, 12);
    
    PangoFontDescription *fd = pango_font_description_new();
    pango_font_description_set_family(fd, "Nimbus Mono PS");
    pango_font_description_set_absolute_size(fd, 22211);
    vte_terminal_set_font(VTE_TERMINAL(vte_terminal), fd);
    vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(vte_terminal), VTE_CURSOR_BLINK_ON);
    pango_font_description_free(fd);
    
    GdkRGBA bg_color, fg_color;
    gdk_rgba_parse(&bg_color, "#121212");
    gdk_rgba_parse(&fg_color, "#A8D8A8");
    GdkRGBA palette[16];
    gdk_rgba_parse(&palette[0], "#000000");
    gdk_rgba_parse(&palette[1], "#CD3131");
    gdk_rgba_parse(&palette[2], "#0DBC79");
    gdk_rgba_parse(&palette[3], "#C9B369");
    gdk_rgba_parse(&palette[4], "#7FA8D0");
    gdk_rgba_parse(&palette[5], "#BC3FBC");
    gdk_rgba_parse(&palette[6], "#11A8CD");
    gdk_rgba_parse(&palette[7], "#E5E5E5");
    gdk_rgba_parse(&palette[8], "#666666");
    gdk_rgba_parse(&palette[9], "#F14C4C");
    gdk_rgba_parse(&palette[10], "#23D18B");
    gdk_rgba_parse(&palette[11], "#F7DC6F");
    gdk_rgba_parse(&palette[12], "#6CB4EE");
    gdk_rgba_parse(&palette[13], "#D670D6");
    gdk_rgba_parse(&palette[14], "#29B8DB");
    gdk_rgba_parse(&palette[15], "#FFFFFF");
    vte_terminal_set_colors(VTE_TERMINAL(vte_terminal), &fg_color, &bg_color, palette, 16);
    
    char *shell = getenv("SHELL");
    if (!shell) shell = "/bin/bash";
    char *argv_shell[] = {shell, NULL};
    vte_terminal_spawn_async(VTE_TERMINAL(vte_terminal), VTE_PTY_DEFAULT, NULL, argv_shell, NULL, 
                             G_SPAWN_DEFAULT, NULL, NULL, NULL, -1, NULL, NULL, NULL);

    g_signal_connect(vte_terminal, "key-press-event", G_CALLBACK(on_vte_key_press), NULL);
    gtk_stack_add_named(GTK_STACK(stack), vte_terminal, "term");

    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
    gtk_accel_group_connect(accel_group, GDK_KEY_t, GDK_CONTROL_MASK, 0, 
                            g_cclosure_new(G_CALLBACK(switch_tab), NULL, NULL));

    gtk_stack_set_visible_child_name(GTK_STACK(stack), "abc");
    append_prompt();

    gtk_widget_show_all(window);
    gtk_widget_grab_focus(text_view);
    gtk_main();
    return 0;
}
