#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

static GtkWidget *window;
static GtkWidget *text_view;
static GtkWidget *overlay;
static GtkWidget *top_overlay;
static GtkTextBuffer *buffer;
static GtkTextMark *input_mark;
static GtkWidget *sys_label = NULL;
static GtkWidget *sys_scroll = NULL;
static GtkWidget *sidebar_overlay = NULL;
static char **tree_lines = NULL;
static int tree_line_count = 0;
static int tree_reveal_index = 0;
static guint tree_reveal_timer_id = 0;
static int first_tree_load = 1;
static gdouble ambient_time = 0.0;
static int level_tema = 1;
static int level_brillo = 4;
static int level_volumen = 4;
static int level_zoom = 4;

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

static void draw_glow(cairo_t *cr, double cx, double cy, double radius, double r, double g, double b, double peak_alpha) {
    cairo_pattern_t *pat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, radius);
    cairo_pattern_add_color_stop_rgba(pat, 0.00, r, g, b, peak_alpha);
    cairo_pattern_add_color_stop_rgba(pat, 0.20, r, g, b, peak_alpha * 0.80);
    cairo_pattern_add_color_stop_rgba(pat, 0.45, r, g, b, peak_alpha * 0.50);
    cairo_pattern_add_color_stop_rgba(pat, 0.70, r, g, b, peak_alpha * 0.20);
    cairo_pattern_add_color_stop_rgba(pat, 0.90, r, g, b, peak_alpha * 0.05);
    cairo_pattern_add_color_stop_rgba(pat, 1.00, r, g, b, 0.0);
    cairo_set_source(cr, pat);
    cairo_arc(cr, cx, cy, radius, 0, 2*M_PI);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
}

static double light_progress(double base, double delay) {
    double v = (base - delay) / (1.0 - delay);
    if (v <= 0.0) return 0.0;
    if (v >= 1.0) v = 1.0;
    const double c1 = 3.5;
    const double c3 = c1 + 1.0;
    double vv = v - 1.0;
    return 1.0 + c3 * vv * vv * vv + c1 * vv * vv;
}

static gboolean draw_left_fade(GtkWidget *w, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);
    double bg = 18.0 / 255.0;
    cairo_pattern_t *pat = cairo_pattern_create_linear(0, 0, width, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0.00, bg, bg, bg, 1.00);
    cairo_pattern_add_color_stop_rgba(pat, 0.35, bg, bg, bg, 0.80);
    cairo_pattern_add_color_stop_rgba(pat, 0.70, bg, bg, bg, 0.35);
    cairo_pattern_add_color_stop_rgba(pat, 1.00, bg, bg, bg, 0.00);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
    return FALSE;
}

static gboolean draw_right_fade(GtkWidget *w, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);
    double bg = 18.0 / 255.0;
    cairo_pattern_t *pat = cairo_pattern_create_linear(0, 0, width, 0);
    cairo_pattern_add_color_stop_rgba(pat, 0.00, bg, bg, bg, 0.00);
    cairo_pattern_add_color_stop_rgba(pat, 0.30, bg, bg, bg, 0.35);
    cairo_pattern_add_color_stop_rgba(pat, 0.65, bg, bg, bg, 0.80);
    cairo_pattern_add_color_stop_rgba(pat, 1.00, bg, bg, bg, 1.00);
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
    return FALSE;
}

static gboolean on_draw_ambient(GtkWidget *w, cairo_t *cr, gpointer data) {
    int width = gtk_widget_get_allocated_width(w);
    int height = gtk_widget_get_allocated_height(w);
    cairo_set_source_rgb(cr, 18.0/255.0, 18.0/255.0, 18.0/255.0);
    cairo_paint(cr);

    double t = ambient_time;
    double a_pink = light_progress(1.0, 0.0);
    double a_purple = light_progress(1.0, 0.25);
    double a_blue = light_progress(1.0, 0.5);

    double pink_y = height * 0.75 + sin(t * 1.5) * 150 + 100;
    double pink_r = 1265.0 * (0.1 + 0.9 * a_pink);
    // draw_glow(cr, width + 200, pink_y, pink_r, 240.0/255.0, 130.0/255.0, 200.0/255.0, 0.105 * a_pink);

    double purple_x = width * 0.85 - sin(t * 1.2) * 150;
    double purple_r = 805.0 * (0.1 + 0.9 * a_purple);
    // draw_glow(cr, purple_x, height + 150, purple_r, 200.0/255.0, 100.0/255.0, 230.0/255.0, 0.13 * a_purple);

    double blue_x = width * 0.55 + sin(t * 1.2) * 150;
    double blue_r = 805.0 * (0.1 + 0.9 * a_blue);
    // draw_glow(cr, blue_x, height + 150, blue_r, 100.0/255.0, 170.0/255.0, 230.0/255.0, 0.13 * a_blue);

    return FALSE;
}

static gboolean timer_cb(gpointer data) {
    if (!data || !GTK_IS_WIDGET(data)) return FALSE;
    ambient_time += 0.015;
    gtk_widget_queue_draw(GTK_WIDGET(data));
    return TRUE;
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
        const char *color;
        if (is_dir) color = "#6CB4EE";
        else if (st.st_mode & S_IXUSR) color = "#27AE60";
        else color = "#E8E8E8";
        const char *branch = is_last ? "\u2514" : "\u251c";
        char *escaped = g_markup_escape_text(namelist[i]->d_name, -1);
        g_string_append_printf(out,
            "<span foreground='#2A2A2A'>%s%s </span><span foreground='%s'>%s</span>\n",
            prefix, branch, color, escaped);
        g_free(escaped);
        if (is_dir) {
            char new_prefix[512];
            snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "  " : "\u2502 ");
            build_tree_rec(path, new_prefix, depth + 1, out);
        }
        free(namelist[i]);
    }
    free(namelist);
}
static gboolean reveal_next_line(gpointer data) {
    if (tree_reveal_index >= tree_line_count) {
        tree_reveal_timer_id = 0;
        return FALSE;
    }
    tree_reveal_index++;
    GString *display = g_string_new("");
    for (int i = 0; i < tree_reveal_index; i++) {
        g_string_append(display, tree_lines[i]);
        g_string_append(display, "\n");
    }
    if (tree_reveal_index < tree_line_count) {
        g_string_append(display, "<span foreground='#F0E5A8'>▊ cargando...</span>");
    }
    gtk_label_set_markup(GTK_LABEL(sys_label), display->str);
    g_string_free(display, TRUE);
    return TRUE;
}

static void start_tree_reveal(const char *full_markup) {
    if (tree_lines) {
        for (int i = 0; i < tree_line_count; i++) free(tree_lines[i]);
        free(tree_lines);
        tree_lines = NULL;
    }
    char *copy = strdup(full_markup);
    tree_line_count = 0;
    char *saveptr = NULL;
    for (char *line = strtok_r(copy, "\n", &saveptr); line; line = strtok_r(NULL, "\n", &saveptr)) {
        tree_line_count++;
    }
    free(copy);
    if (tree_line_count == 0) return;
    tree_lines = malloc(sizeof(char*) * tree_line_count);
    copy = strdup(full_markup);
    saveptr = NULL;
    int idx = 0;
    for (char *line = strtok_r(copy, "\n", &saveptr); line; line = strtok_r(NULL, "\n", &saveptr)) {
        tree_lines[idx++] = strdup(line);
    }
    free(copy);
    tree_reveal_index = 0;
    if (tree_reveal_timer_id) g_source_remove(tree_reveal_timer_id);
    tree_reveal_timer_id = g_timeout_add(12, reveal_next_line, NULL);
}

static void read_system_info() {
    if (!sys_label) return;
    const char *dir = g_get_home_dir();
    GString *s = g_string_new("");
    g_string_append_printf(s, "\xe2\x80\x89<span foreground='#D8DCE3' weight='bold' size='9677'>%s</span>\n", dir);
    build_tree_rec(dir, "", 0, s);
    if (0) {
        start_tree_reveal(s->str);
        first_tree_load = 0;
    } else {
        gtk_label_set_markup(GTK_LABEL(sys_label), s->str);
    }
    g_string_free(s, TRUE);
}

static gboolean auto_scroll_cb(gpointer data) {
    static int was_revealing = 0;
    if (!sys_scroll) return TRUE;
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(sys_scroll));
    if (!vadj) return TRUE;
    double upper = gtk_adjustment_get_upper(vadj);
    double page = gtk_adjustment_get_page_size(vadj);
    double max = upper - page;
    if (max <= 0) return TRUE;

    if (tree_reveal_timer_id != 0) {
        gtk_adjustment_set_value(vadj, max);
        was_revealing = 1;
        return TRUE;
    }

    if (was_revealing) {
        was_revealing = 0;
        gtk_adjustment_set_value(vadj, 0);
        return TRUE;
    }

    double value = gtk_adjustment_get_value(vadj) + 3.0;
    if (value > max) value = 0;
    gtk_adjustment_set_value(vadj, value);
    return TRUE;
}


static gboolean sys_timer_cb(gpointer data) { if (tree_reveal_timer_id != 0) return TRUE; read_system_info(); return TRUE; }

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
    GtkTextTag *tag_abc  = gtk_text_buffer_create_tag(buffer, "abc", "foreground", "#F06BFF", "weight", PANGO_WEIGHT_BOLD, NULL);
    GtkTextTag *tag_def  = gtk_text_buffer_create_tag(buffer, NULL, "foreground", "#E6EAF0", NULL);

    char u_h[512];
    snprintf(u_h, sizeof(u_h), "%s", user);
    gtk_text_buffer_insert_with_tags(buffer, &end, u_h, -1, tag_user, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, "@", -1, tag_at, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, hostname, -1, tag_user, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, "/", -1, tag_def, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, "abc", -1, tag_abc, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, ":", -1, tag_def, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, path, -1, tag_def, NULL);
    gtk_text_buffer_insert_with_tags(buffer, &end, "$ ", -1, tag_def, NULL);

    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_move_mark(buffer, input_mark, &end);
    gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(text_view), input_mark);
}

static void aplicar_ajuste(GtkWidget *w, gpointer data) {
    const char *tipo = (const char*)data;
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
    gtk_widget_destroy(GTK_WIDGET(w));
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
    gtk_label_set_markup(GTK_LABEL(lbl), title);
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
            if (strcmp(cmd, "shell") == 0 || strcmp(cmd, "cd") == 0 || strncmp(cmd, "cd ", 3) == 0 || strcmp(cmd, "ls") == 0) {
                gtk_text_buffer_insert(buffer, &end, "\n[ABC]: Usa Win+Enter para alacritty.\n", -1);
                g_free(cmd);
                append_prompt();
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

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    g_object_set(gtk_settings_get_default(), "gtk-error-bell", FALSE, NULL);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "ABC");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window { background-color: #121212; border: none; outline: none; box-shadow: none; }\n"
        "box, overlay, hbox, sidebar, viewport, .background { border: none; box-shadow: none; background-color: transparent; background-image: none; padding: 0; margin: 0; }\n"
        "textview, textview text { background-color: transparent; color: #D8DCE3; font-family: 'Iosevka', monospace; font-size: 18px; padding: 0; }\n"
        "#sys-label { font-family: 'Noto Sans Mono', monospace; font-size: 12px; background: transparent; border: none; }\n"
        "scrollbar { background: transparent; border: none; }\n"
        "scrollbar trough { background: transparent; border: none; box-shadow: none; min-width: 8px; min-height: 8px; }\n"
        "scrollbar slider { background-color: rgba(108,180,238,0.4); border-radius: 4px; border: none; }\n"
        "scrollbar.vertical slider { min-width: 6px; min-height: 24px; }\n"
        "scrollbar.horizontal slider { min-width: 24px; min-height: 6px; }\n"
        "scrollbar slider:hover { background-color: rgba(108,180,238,0.75); }", -1, NULL);
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
    gtk_widget_set_size_request(sidebar, 296, -1);
    gtk_box_pack_start(GTK_BOX(hbox), sidebar, FALSE, FALSE, 0);

    sys_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sys_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_propagate_natural_width(GTK_SCROLLED_WINDOW(sys_scroll), FALSE);
    gtk_widget_set_vexpand(sys_scroll, TRUE);
    sidebar_overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(sidebar_overlay), sys_scroll);
    GtkWidget *fade_left = gtk_drawing_area_new();
    gtk_widget_set_size_request(fade_left, 24, -1);
    gtk_widget_set_halign(fade_left, GTK_ALIGN_START);
    gtk_widget_set_valign(fade_left, GTK_ALIGN_FILL);
    g_signal_connect(fade_left, "draw", G_CALLBACK(draw_left_fade), NULL);
    gtk_overlay_add_overlay(GTK_OVERLAY(sidebar_overlay), fade_left);
    GtkWidget *fade_right = gtk_drawing_area_new();
    gtk_widget_set_size_request(fade_right, 16, -1);
    gtk_widget_set_halign(fade_right, GTK_ALIGN_END);
    gtk_widget_set_valign(fade_right, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand(fade_right, FALSE);
    gtk_widget_set_vexpand(fade_right, TRUE);
    g_signal_connect(fade_right, "draw", G_CALLBACK(draw_right_fade), NULL);
    gtk_overlay_add_overlay(GTK_OVERLAY(sidebar_overlay), fade_right);
    gtk_box_pack_start(GTK_BOX(sidebar), sidebar_overlay, TRUE, TRUE, 0);

    sys_label = gtk_label_new(NULL);
    gtk_widget_set_name(sys_label, "sys-label");
    gtk_label_set_xalign(GTK_LABEL(sys_label), 0.0);
    gtk_label_set_yalign(GTK_LABEL(sys_label), 0.0);
    gtk_widget_set_halign(sys_label, GTK_ALIGN_START);
    gtk_widget_set_valign(sys_label, GTK_ALIGN_START);
    gtk_label_set_line_wrap(GTK_LABEL(sys_label), FALSE);
    gtk_widget_set_margin_start(sys_label, 6);
    gtk_widget_set_margin_top(sys_label, 8);
    gtk_container_add(GTK_CONTAINER(sys_scroll), sys_label);

    overlay = gtk_overlay_new();
    gtk_widget_set_vexpand(overlay, TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), overlay, TRUE, TRUE, 0);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), TRUE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_margin_start(text_view, 10);
    gtk_widget_set_margin_top(text_view, 12);
    gtk_container_add(GTK_CONTAINER(overlay), text_view);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    input_mark = gtk_text_buffer_create_mark(buffer, "input", &end, TRUE);
    g_signal_connect(text_view, "key-press-event", G_CALLBACK(on_key_press), NULL);

    g_signal_connect(top_overlay, "draw", G_CALLBACK(on_draw_ambient), NULL);
    g_timeout_add(30, timer_cb, top_overlay);

    read_system_info();
    g_timeout_add(2000, sys_timer_cb, NULL);
    // g_timeout_add(50, auto_scroll_cb, NULL);

    append_prompt();

    gtk_widget_show_all(window);
    gtk_widget_grab_focus(text_view);
    gtk_main();
    return 0;
}
