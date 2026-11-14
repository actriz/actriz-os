#include <gtk/gtk.h>
#include <stdlib.h>

GtkWidget *window;
GtkWidget *label;

void aplicar_css(const char* modo) {
    GtkCssProvider *provider = gtk_css_provider_new();
    char css[2048];

    // CSS mejorado: ajustamos tamaño del riel y del slider proporcionalmente
    if (g_strcmp0(modo, "dark") == 0) {
        snprintf(css, sizeof(css),
            "window { background-color: #121212; transition: 200ms; } "
            "label { color: #ffffff; font-size: 16px; font-weight: bold; font-family: sans-serif; } "
            "switch { background-color: #3a3a3c; border-radius: 16px; min-width: 56px; min-height: 32px; border: none; box-shadow: none; } "
            "switch:checked { background-color: #34c759; } "
            "switch slider { background-color: #ffffff; border-radius: 14px; min-width: 28px; min-height: 28px; margin: 2px; box-shadow: 0 2px 4px rgba(0,0,0,0.4); }"
        );
    } else {
        snprintf(css, sizeof(css),
            "window { background-color: #f2f2f7; transition: 200ms; } "
            "label { color: #000000; font-size: 16px; font-weight: bold; font-family: sans-serif; } "
            "switch { background-color: #e5e5ea; border-radius: 16px; min-width: 56px; min-height: 32px; border: none; box-shadow: none; } "
            "switch:checked { background-color: #34c759; } "
            "switch slider { background-color: #ffffff; border-radius: 14px; min-width: 28px; min-height: 28px; margin: 2px; box-shadow: 0 2px 4px rgba(0,0,0,0.15); }"
        );
    }

    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);
}

void on_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data) {
    GtkSwitch *sw = GTK_SWITCH(object);
    if (gtk_switch_get_active(sw)) {
        // Actualizamos la interfaz INMEDIATAMENTE
        aplicar_css("dark");
        // Ejecutamos el script en segundo plano (sin bloquear la app)
        g_spawn_command_line_async("/opt/actriz/bin/set-theme.sh dark", NULL);
    } else {
        aplicar_css("light");
        g_spawn_command_line_async("/opt/actriz/bin/set-theme.sh light", NULL);
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Actriz - Tema");
    gtk_window_set_default_size(GTK_WINDOW(window), 320, 120);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_container_add(GTK_CONTAINER(window), box);

    label = gtk_label_new("Claro / Oscuro");
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    GtkWidget *switch_widget = gtk_switch_new();
    // Ya NO forzamos el tamaño aquí, el CSS se encarga de darle la forma correcta
    gtk_box_pack_start(GTK_BOX(box), switch_widget, FALSE, FALSE, 0);

    aplicar_css("light");

    g_signal_connect(switch_widget, "notify::active", G_CALLBACK(on_switch_toggled), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
