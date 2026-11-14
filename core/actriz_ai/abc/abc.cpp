#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

int levenshtein(const std::string& s1, const std::string& s2) {
    size_t m = s1.size(), n = s2.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    for (size_t i = 0; i <= m; i++) dp[i][0] = i;
    for (size_t j = 0; j <= n; j++) dp[0][j] = j;
    for (size_t i = 1; i <= m; i++) {
        for (size_t j = 1; j <= n; j++) {
            if (s1[i - 1] == s2[j - 1]) dp[i][j] = dp[i - 1][j - 1];
            else dp[i][j] = std::min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + 1});
        }
    }
    return dp[m][n];
}

bool se_parece(const std::string& palabra, const std::string& objetivo) {
    if (palabra == objetivo) return true;
    if (std::abs((int)palabra.length() - (int)objetivo.length()) > 2) return false;
    int limite = (objetivo.length() <= 4) ? 1 : 2;
    return levenshtein(palabra, objetivo) <= limite;
}

bool en_dict(const std::string& palabra, const std::vector<std::string>& dict) {
    for (const auto& d : dict) if (se_parece(palabra, d)) return true;
    return false;
}

bool es_exacto(const std::string& palabra, const std::vector<std::string>& dict) {
    for (const auto& d : dict) if (palabra == d) return true;
    return false;
}

// Diccionarios
std::vector<std::string> D_SUBIR = {"sube", "subir", "aumenta", "incrementa", "eleva", "alza", "subele", "mas", "monta", "escala"};
std::vector<std::string> D_BAJAR = {"baja", "bajar", "reduce", "disminuye", "decrece", "desciende", "merma", "menos", "aminora", "recorta", "bajale"};
std::vector<std::string> D_PONER = {"pon", "poner", "coloca", "fija", "establece", "ajusta", "configura", "asigna", "deja", "cambia"};
std::vector<std::string> D_MUTE = {"mute", "silencio", "mudo", "mutea", "silencia", "cala", "amordaza"};
std::vector<std::string> D_MAX = {"max", "maximo", "tope", "cien", "todo", "lleno", "completo", "arriba", "alto", "total"};
std::vector<std::string> D_MIN = {"min", "minimo", "nada", "cero", "bajo", "oscuro", "vacio"};
std::vector<std::string> D_APAGAR = {"apaga", "apagar", "duerme", "bloquea", "suspende"};

std::vector<std::string> D_BRILLO = {"brillo", "luz", "pantalla", "display", "monitor", "luminosidad", "claridad", "reflejo", "resplandor", "panel"};
std::vector<std::string> D_VOLUMEN = {"volumen", "audio", "sonido", "musica", "parlante", "bocina", "altavoz", "ruido", "decibelios", "voz"};
std::vector<std::string> D_FONDO = {"fondo", "wallpaper", "imagen", "foto", "tapiz", "escritorio", "lienzo", "wall", "background"};

std::vector<std::string> D_CEGAR = {"cegar", "ciega", "cegando", "duele", "lastima", "molesta", "quema"};
std::vector<std::string> D_LEER = {"leer", "leyendo", "estudiar", "estudiando", "trabajar", "trabajando"};

void mostrar_ayuda() {
    std::cout << "\033[1;36m=== ABC (Actriz Basic Command) v6.4 ===\033[0m\n";
    std::cout << "Soporta comandos multiples y matematicas:\n";
    std::cout << "  - \033[1;33mABC me esta cegando\033[0m\n";
    std::cout << "  - \033[1;33mABC pon el brillo a la mitad\033[0m\n";
    std::cout << "  - \033[1;33mABC apaga la pantalla\033[0m\n";
    std::cout << "  - \033[1;33mABC cambia el fondo a luna.jpg\033[0m\n";
}

void procesar_comando(std::string texto, std::string accion_heredada) {
    std::vector<std::string> palabras;
    std::istringstream stream(texto);
    std::string palabra;
    while (stream >> palabra) {
        // FIX: Borrar puntuación PERO mantener puntos, guiones y guiones bajos
        palabra.erase(std::remove_if(palabra.begin(), palabra.end(), [](char c) {
            return !std::isalnum(c) && c != '.' && c != '-' && c != '_';
        }), palabra.end());
        
        if (!palabra.empty()) palabras.push_back(palabra);
    }
    if (palabras.empty()) return;

    std::string entidad = "";
    
    for (const auto& p : palabras) {
        if (es_exacto(p, D_BRILLO)) { entidad = "brillo"; break; }
        if (es_exacto(p, D_VOLUMEN)) { entidad = "volumen"; break; }
        if (es_exacto(p, D_FONDO)) { entidad = "fondo"; break; }
    }

    if (entidad.empty()) {
        for (const auto& p : palabras) {
            if (en_dict(p, D_BRILLO)) { entidad = "brillo"; break; }
            if (en_dict(p, D_VOLUMEN)) { entidad = "volumen"; break; }
            if (en_dict(p, D_FONDO)) { entidad = "fondo"; break; }
        }
    }

    if (entidad.empty()) {
        for (const auto& p : palabras) {
            if (en_dict(p, D_CEGAR) || en_dict(p, D_LEER)) entidad = "brillo";
        }
    }

    if (entidad.empty()) {
        std::cout << "\033[1;31m[ABC]: No sé de qué me hablas (no encontré entidad).\033[0m\n";
        return;
    }

    if (entidad == "fondo") {
        std::string ruta = "/opt/actriz/entornografico/tema/";
        bool ver_lista = false;
        for (const auto& p : palabras) if (se_parece(p, "ver") || se_parece(p, "lista")) ver_lista = true;
        
        if (ver_lista) {
            std::cout << "\033[1;35m[ABC]:\033[0m Fondos disponibles:\n";
            for (const auto& entry : fs::directory_iterator(ruta)) {
                std::string ext = entry.path().extension().string();
                if (ext == ".jpg" || ext == ".png") std::cout << "  - " << entry.path().filename().string() << "\n";
            }
            return;
        }
        
        std::string archivo = "";
        for (const auto& p : palabras) {
            // Ahora "luna.jpg" sí conserva el punto
            if (p.find(".jpg") != std::string::npos || p.find(".png") != std::string::npos) archivo = p;
        }
        
        if (!archivo.empty() && fs::exists(ruta + archivo)) {
            system(("/opt/actriz/bin/set-wallpaper " + ruta + archivo).c_str());
            std::cout << "\033[1;35m[ABC]:\033[0m Fondo cambiado a \033[1;33m" << archivo << "\033[0m\n";
        } else {
            std::cout << "\033[1;31m[ABC]: Archivo no válido o no especificado.\033[0m\n";
        }
        return;
    }

    int valor = -1;
    std::string accion = accion_heredada;
    bool signo_menos = false;

    for (const auto& p : palabras) {
        if (en_dict(p, D_CEGAR)) accion = "bajar";
        if (en_dict(p, D_LEER)) accion = "subir";
    }

    for (size_t i = 0; i < palabras.size(); i++) {
        std::string p = palabras[i];
        if (se_parece(p, "mitad") || se_parece(p, "medio")) { valor = 50; if(accion.empty()) accion = "cambiar"; }
        if (se_parece(p, "cuarto")) { valor = 25; if(accion.empty()) accion = "cambiar"; }
        if (se_parece(p, "doble")) { valor = 100; if(accion.empty()) accion = "subir"; }

        if (en_dict(p, D_MAX)) { accion = "cambiar"; valor = 100; }
        if (en_dict(p, D_MIN)) { accion = "cambiar"; valor = 1; }
        if (en_dict(p, D_APAGAR)) { accion = "apagar"; }

        try {
            size_t pos;
            int num = std::stoi(p, &pos); 
            valor = num;
            
            bool hay_mas = (i > 0 && en_dict(palabras[i-1], D_SUBIR)) || (i < palabras.size()-1 && en_dict(palabras[i+1], D_SUBIR));
            bool hay_menos = (i > 0 && (en_dict(palabras[i-1], D_BAJAR) || palabras[i-1] == "menos")) || (i < palabras.size()-1 && (en_dict(palabras[i+1], D_BAJAR) || palabras[i+1] == "menos"));

            if (i > 0 && (palabras[i-1] == "al" || palabras[i-1] == "a" || palabras[i-1] == "en")) {
                if (hay_menos) accion = "bajar"; else accion = "cambiar";
            } else if (hay_mas) {
                accion = "subir";
            } else if (hay_menos) {
                accion = "bajar";
            }
        } catch (...) {}

        if (accion != "cambiar" && accion != "apagar") {
            if (en_dict(p, D_SUBIR)) accion = "subir";
            if (en_dict(p, D_BAJAR)) accion = "bajar";
            if (en_dict(p, D_PONER)) accion = "cambiar";
        }
        if (en_dict(p, D_MUTE) && entidad == "volumen") accion = "mute";
    }

    if (accion.empty()) {
        std::cout << "\033[1;31m[ABC]: No entendí la acción para " << entidad << ".\033[0m\n";
        return;
    }

    std::string script = (entidad == "volumen") ? "/opt/actriz/drivers/audio/control.sh" : "/opt/actriz/drivers/brillo/control.sh";
    std::string cmd = script + " " + accion;
    if (valor != -1 && accion != "mute" && accion != "apagar") cmd += " " + std::to_string(valor);
    system(cmd.c_str());
}

int main(int argc, char* argv[]) {
    if (argc < 2) { mostrar_ayuda(); return 0; }

    std::string texto = "";
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
        texto += arg + " ";
    }

    std::string accion_heredada = "";
    size_t pos = 0;
    std::string delim = " y ";
    
    while ((pos = texto.find(delim)) != std::string::npos) {
        std::string sub = texto.substr(0, pos);
        if (sub.find("baja") != std::string::npos) accion_heredada = "bajar";
        if (sub.find("sube") != std::string::npos) accion_heredada = "subir";
        if (sub.find("pon") != std::string::npos) accion_heredada = "cambiar";
        procesar_comando(sub, "");
        texto.erase(0, pos + delim.length());
    }
    procesar_comando(texto, accion_heredada);

    return 0;
}
