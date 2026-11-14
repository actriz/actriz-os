#include <gtk/gtk.h>
#include <stdlib.h>

void on_switch_toggled(GObject *object, GParamSpec *pspec, gpointer user_data) {
    GtkSwitch *sw = GTK_SWITCH(object);
    if (gtk_switch_get_active(sw)) {
        g_spawn_command_line_async("/opt/actriz/bin/set-theme.sh dark", NULL);
    } else {
        g_spawn_command_line_async("/opt/actriz/bin/set-theme.sh light", NULL);
    }
}

void abrir_wifi(GtkWidget *widget, gpointer data) {
    g_spawn_command_line_async("alacritty -e nmtui", NULL);
}

void abrir_bluetooth(GtkWidget *widget, gpointer data) {
    g_spawn_command_line_async("alacritty -e bluetoothctl", NULL);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Sistema - Actriz OS");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Cuaderno de pestañas
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), notebook);

    // === PESTAÑA 1: APARIENCIA ===
    GtkWidget *box_apariencia = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box_apariencia), 20);
    
    GtkWidget *box_switch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    GtkWidget *label_tema = gtk_label_new("Modo Oscuro");
    GtkWidget *switch_tema = gtk_switch_new();
    
    // Hacer el switch más grande para que se vea estilo Apple
    gtk_widget_set_size_request(switch_tema, 60, 30);
    
    gtk_box_pack_start(GTK_BOX(box_switch), label_tema, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(box_switch), switch_tema, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box_apariencia), box_switch, FALSE, FALSE, 0);

    g_signal_connect(switch_tema, "notify::active", G_CALLBACK(on_switch_toggled), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box_apariencia, gtk_label_new("Apariencia"));

    // === PESTAÑA 2: RED (WIFI) ===
    GtkWidget *box_wifi = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box_wifi), 20);
    GtkWidget *btn_wifi = gtk_button_new_with_label("Gestionar Redes WiFi (nmtui)");
    gtk_box_pack_start(GTK_BOX(box_wifi), btn_wifi, FALSE, FALSE, 0);
    g_signal_connect(btn_wifi, "clicked", G_CALLBACK(abrir_wifi), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box_wifi, gtk_label_new("Red WiFi"));

    // === PESTAÑA 3: BLUETOOTH ===
    GtkWidget *box_bt = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_container_set_border_width(GTK_CONTAINER(box_bt), 20);
    GtkWidget *btn_bt = gtk_button_new_with_label("Gestionar Dispositivos Bluetooth");
    gtk_box_pack_start(GTK_BOX(box_bt), btn_bt, FALSE, FALSE, 0);
    g_signal_connect(btn_bt, "clicked", G_CALLBACK(abrir_bluetooth), NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box_bt, gtk_label_new("Bluetooth"));

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
