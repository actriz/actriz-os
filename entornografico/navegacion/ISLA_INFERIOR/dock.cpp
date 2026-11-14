#include <gtkmm.h>
#include <cairomm/context.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cmath>

class Dock : public Gtk::Window {
public:
    Dock() {
        set_decorated(false);
        set_keep_above(true);
        set_skip_taskbar_hint(true);
        set_skip_pager_hint(true);
        set_accept_focus(false);
        
        // ¡CAMBIO CLAVE 1! Quitamos el WINDOW_TYPE_HINT_DOCK
        set_type_hint(Gdk::WINDOW_TYPE_HINT_UTILITY);
        
        set_wmclass("keu-dock", "KeuDock");
        
        width = 400;
        height = 60;
        set_default_size(width, height);

        set_app_paintable(true);
        auto screen = Gdk::Screen::get_default();
        auto visual = screen->get_rgba_visual();
        if (visual) {
            gtk_widget_set_visual(GTK_WIDGET(gobj()), visual->gobj());
        }

        auto settings = Gtk::Settings::get_default();
        if (settings) {
            settings->set_property("gtk-icon-theme-name", Glib::ustring("WhiteSur-dark"));
        }

        auto theme = Gtk::IconTheme::get_default();
        std::vector<std::string> icon_names = {
            "utilities-terminal", 
            "web-browser", 
            "system-file-manager", 
            "preferences-system"
        };
        
        for (const auto& name : icon_names) {
            try {
                auto pixbuf = theme->load_icon(name, 32, Gtk::ICON_LOOKUP_FORCE_SIZE);
                icons.push_back(pixbuf);
            } catch (const Glib::Error& ex) {
                icons.push_back(Glib::RefPtr<Gdk::Pixbuf>());
            }
        }

        commands = {"alacritty", "flatpak run app.zen_browser.zen", "thunar", "mate-control-center"};

        add_events(Gdk::BUTTON_PRESS_MASK | Gdk::POINTER_MOTION_MASK | Gdk::LEAVE_NOTIFY_MASK);
        signal_button_press_event().connect(sigc::mem_fun(*this, &Dock::on_button_press));
        signal_motion_notify_event().connect(sigc::mem_fun(*this, &Dock::on_mouse_move));
        signal_leave_notify_event().connect(sigc::mem_fun(*this, &Dock::on_mouse_leave));
        
        signal_map().connect([this]() {
            // ¡CAMBIO CLAVE 2! Esperamos 100ms a que i3 asiente la ventana y luego la movemos
            Glib::signal_timeout().connect([this]() {
                auto screen = Gdk::Screen::get_default();
                int screen_w = screen->get_width();
                int screen_h = screen->get_height();
                
                int x_pos = (screen_w - width) / 2;
                int y_pos = screen_h - height - 35;

                auto gdk_win = get_window();
                if (gdk_win) {
                    Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_win->get_display()->gobj());
                    ::Window xwin = gdk_x11_window_get_xid(gdk_win->gobj());
                    XMoveWindow(xdisplay, xwin, x_pos, y_pos);
                    XFlush(xdisplay); // ¡CAMBIO CLAVE 3! Forzamos a X11 a aplicar el cambio
                }
                return false;
            }, 100);

            // Aplicar máscara de recorte
            Cairo::RefPtr<Cairo::ImageSurface> mask = Cairo::ImageSurface::create(Cairo::FORMAT_A1, width, height);
            Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(mask);
            double r = height / 2.0;
            cr->begin_new_sub_path();
            cr->arc(width - r, r, r, -M_PI/2, 0);
            cr->arc(width - r, height - r, r, 0, M_PI/2);
            cr->arc(r, height - r, r, M_PI/2, M_PI);
            cr->arc(r, r, r, M_PI, 3*M_PI/2);
            cr->close_path();
            cr->fill();
            mask->flush();

            cairo_region_t* c_region = gdk_cairo_region_create_from_surface(mask->cobj());
            gtk_widget_shape_combine_region(GTK_WIDGET(gobj()), c_region);
            cairo_region_destroy(c_region);
        });
    }

protected:
    int width, height;
    int hover_index = -1;
    std::vector<Glib::RefPtr<Gdk::Pixbuf>> icons;
    std::vector<std::string> commands;

    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override {
        cr->set_operator(Cairo::OPERATOR_SOURCE);
        cr->set_source_rgba(0, 0, 0, 0);
        cr->paint();
        cr->set_operator(Cairo::OPERATOR_OVER);
        cr->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);

        double r = height / 2.0;
        cr->begin_new_sub_path();
        cr->arc(width - r, r, r, -M_PI/2, 0);
        cr->arc(width - r, height - r, r, 0, M_PI/2);
        cr->arc(r, height - r, r, M_PI/2, M_PI);
        cr->arc(r, r, r, M_PI, 3*M_PI/2);
        cr->close_path();

        cr->set_source_rgba(0.1, 0.1, 0.1, 0.7);
        cr->fill_preserve();
        cr->set_source_rgba(1.0, 1.0, 1.0, 0.2);
        cr->set_line_width(1.5);
        cr->stroke();

        if (icons.empty()) return false;
        
        double spacing = 70.0;
        double start_x = (width - (icons.size() * spacing)) / 2.0 + 15;
        double y_pos = (height - 32) / 2.0;

        for (size_t i = 0; i < icons.size(); ++i) {
            if (icons[i]) {
                double x_pos = start_x + i * spacing;
                double scale = (hover_index == (int)i) ? 1.2 : 1.0;
                double size = 32 * scale;
                double offset = (32 - size) / 2.0;

                Gdk::Cairo::set_source_pixbuf(cr, icons[i], x_pos + offset, y_pos + offset);
                cr->paint_with_alpha((hover_index == (int)i) ? 1.0 : 0.8);
            }
        }
        return false;
    }

    bool on_mouse_move(GdkEventMotion* event) {
        int spacing = 70;
        int start_x = (width - (icons.size() * spacing)) / 2 + 15;
        int new_hover = -1;
        
        for (size_t i = 0; i < icons.size(); ++i) {
            int ix = start_x + i * spacing;
            if (event->x >= ix && event->x <= ix + 32 && event->y >= 14 && event->y <= 46) {
                new_hover = i;
                break;
            }
        }
        
        if (new_hover != hover_index) {
            hover_index = new_hover;
            queue_draw();
        }
        return true;
    }

    bool on_mouse_leave(GdkEventCrossing* event) {
        hover_index = -1;
        queue_draw();
        return true;
    }

    bool on_button_press(GdkEventButton* event) {
        if (event->button == 1 && hover_index != -1) {
            std::string cmd = commands[hover_index] + " &";
            system(cmd.c_str());
            hide();
            exit(0); 
        }
        return true;
    }
};

int main(int argc, char* argv[]) {
    Gtk::Main kit(argc, argv);
    Dock dock;
    Gtk::Main::run(dock);
    return 0;
}
