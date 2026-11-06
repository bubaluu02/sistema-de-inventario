#include "database.hpp"
#include "main_window.hpp"
#include <gtkmm.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <system_error>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#endif

static void loadCSS() {
    auto cssProvider = Gtk::CssProvider::create();
    bool loaded = false;

    // Try loading from file
    std::vector<std::string> paths = {"src/style.css", "style.css", "../src/style.css"};
    for (const auto& path : paths) {
        std::ifstream f(path);
        if (f.good()) {
            try {
                cssProvider->load_from_path(path);
                loaded = true;
                std::cout << "[INFO] Estilos CSS cargados desde: " << path << std::endl;
                break;
            } catch (const Glib::Error& e) {
                std::cerr << "[WARN] Error cargando CSS desde " << path << ": " << e.what() << std::endl;
            }
        }
    }

    if (!loaded) {
        // Fallback embedded basic CSS
        const std::string defaultCss = R"CSS(
            .excel-ribbon-bar { background: #107c41; color: white; padding: 10px; }
            .excel-title { font-size: 18px; font-weight: bold; color: white; }
            .kpi-card { background: white; border: 1px solid #ccc; border-radius: 6px; padding: 12px; }
            .kpi-value-green { font-size: 24px; font-weight: bold; color: #107c41; }
            .btn-action-primary { background: #107c41; color: white; font-weight: bold; padding: 8px 16px; border-radius: 4px; }
            treeview { background: white; }
        )CSS";
        cssProvider->load_from_data(defaultCss);
    }

    auto display = Gdk::Display::get_default();
    if (display) {
        Gtk::StyleContext::add_provider_for_display(
            display,
            cssProvider,
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
}

#ifdef _WIN32
// En Windows el programa se abre con doble clic y el directorio de trabajo
// puede ser cualquiera. Se fija a la carpeta del .exe para que barberia.db,
// recibos/, style.css y los CSV queden siempre junto al programa.
static void setWorkingDirToExe() {
    wchar_t buf[MAX_PATH * 4];
    DWORD n = GetModuleFileNameW(nullptr, buf, (DWORD)(sizeof(buf) / sizeof(buf[0])));
    if (n == 0 || n >= sizeof(buf) / sizeof(buf[0])) return;
    std::error_code ec;
    std::filesystem::current_path(std::filesystem::path(buf).parent_path(), ec);
}
#endif

int main(int argc, char* argv[]) {
#ifdef _WIN32
    setWorkingDirToExe();
#endif
    std::cout << "========================================================\n";
    std::cout << "  💈 SISTEMA DE CONTROL DE VENTAS E INVENTARIO - BARBERÍA\n";
    std::cout << "========================================================\n";

    // 1. Initialize SQLite Database
    std::string dbPath = "barberia.db";
    if (!Database::instance().open(dbPath)) {
        std::cerr << "[ERROR] No se pudo inicializar la base de datos SQLite en: " << dbPath << std::endl;
        return 1;
    }
    std::cout << "[OK] Base de datos SQLite conectada con éxito (" << dbPath << ").\n";

    // 2. Initialize GTK4 Application
    auto app = Gtk::Application::create("com.barberia.pos");

    app->signal_startup().connect([]() {
        loadCSS();
    });

    return app->make_window_and_run<MainWindow>(argc, argv);
}
