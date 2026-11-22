#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <cctype>
#include <unistd.h>

// ===== Color opcional (ANSI) =====
static bool USE_COLOR = false;

static void init_color(int argc, char* argv[]) {
    USE_COLOR = isatty(fileno(stdout)) != 0;
    for (int i = 1; i < argc; i++) if (std::string(argv[i]) == "--color") USE_COLOR = true;
    if (const char* e = getenv("ABC_COLOR")) if (std::string(e) == "0") USE_COLOR = false;
    if (const char* n = getenv("NO_COLOR")) if (n[0] != '\0') USE_COLOR = false;
}

static std::string col(const std::string& s, const char* code) {
    if (!USE_COLOR) return s;
    return std::string(code) + s + "\033[0m";
}

void print_interp(const std::string& accion, const std::string& entidad,
                  int valor, const std::string& prefijo = "") {
    std::cout << "\033[1;35m[ABC]:\033[0m ";
    if (!prefijo.empty()) std::cout << prefijo << " ";
    std::cout << col("**" + accion + "**", "\033[1;34m") << " "
              << col("\"" + entidad + "\"", "\033[1;32m");
    if (valor != -1 && accion != "mute" && accion != "desmute" && accion != "apagar")
        std::cout << " " << col("/" + std::to_string(valor) + "/", "\033[1;37m");
    std::cout << "\n";
}

namespace fs = std::filesystem;

// FON-DL
std::unordered_map<std::string, std::string> _cache_fon;
std::string normalizar_fonetico(const std::string& s) {
    auto it = _cache_fon.find(s);
    if (it != _cache_fon.end()) return it->second;
    std::string r;
    for (size_t i = 0; i < s.size(); i++) {
        char ch = (char)std::tolower((unsigned char)s[i]);
        if (ch == 'h') continue;
        if (ch == 'v') { r += 'b'; continue; }
        if (ch == 'z') { r += 's'; continue; }
        if (ch == 'q') { if (i+1 < s.size() && s[i+1] == 'u') i++; r += 'k'; continue; }
        if (ch == 'c') {
            if (i+1 < s.size() && (s[i+1] == 'e' || s[i+1] == 'i')) { r += 's'; continue; }
            r += 'k'; continue;
        }
        if (ch == 'l' && i+1 < s.size() && s[i+1] == 'l') { r += 'y'; i++; continue; }
        r += ch;
    }
    _cache_fon[s] = r;
    return r;
}
int levenshtein(const std::string& s1, const std::string& s2) {
    size_t m = s1.size(), nn = s2.size();
    std::vector<std::vector<int>> dp(m+1, std::vector<int>(nn+1));
    for (size_t i = 0; i <= m; i++) dp[i][0] = (int)i;
    for (size_t j = 0; j <= nn; j++) dp[0][j] = (int)j;
    for (size_t i = 1; i <= m; i++) for (size_t j = 1; j <= nn; j++) {
        if (s1[i-1] == s2[j-1]) dp[i][j] = dp[i-1][j-1];
        else dp[i][j] = std::min({dp[i-1][j]+1, dp[i][j-1]+1, dp[i-1][j-1]+1});
        if (i > 1 && j > 1 && s1[i-1] == s2[j-2] && s1[i-2] == s2[j-1])
            dp[i][j] = std::min(dp[i][j], dp[i-2][j-2] + 1);
    }
    return dp[m][nn];
}
bool bolsa_parecida(const std::string& a, const std::string& b, int lim) {
    int fa[256] = {0}, fb[256] = {0};
    for (char ch : a) fa[(unsigned char)std::tolower((unsigned char)ch)]++;
    for (char ch : b) fb[(unsigned char)std::tolower((unsigned char)ch)]++;
    int falta = 0;
    for (int i = 0; i < 256; i++) if (fa[i] > fb[i]) falta += fa[i] - fb[i];
    return falta <= lim;
}


// SP-NEU
bool se_parece(const std::string& palabra, const std::string& objetivo) {
    if (palabra == objetivo) return true;
    std::string pa = normalizar_fonetico(palabra);
    std::string ob = normalizar_fonetico(objetivo);
    if (pa == ob) return true;
    if (std::abs((int)pa.length() - (int)ob.length()) > 2) return false;
    int limite = (ob.length() <= 4) ? 1 : 2;
    if (!bolsa_parecida(pa, ob, limite)) return false;
    return levenshtein(pa, ob) <= limite;
}


bool en_dict(const std::string& palabra, const std::vector<std::string>& dict) {
    for (const auto& d : dict) if (se_parece(palabra, d)) return true;
    return false;
}

bool es_exacto(const std::string& palabra, const std::vector<std::string>& dict) {
    for (const auto& d : dict) if (palabra == d) return true;
    return false;
}

// Normaliza "por ciento" y variantes a "porciento" (se ignora luego)
std::string normalizar_porcentaje(std::string s) {
    struct Rep { const char* from; const char* to; };
    static const Rep reps[] = {
        {"por ciento", "porciento"}, {"por cien", "porciento"},
        {"por siento", "porciento"}, {"por sentaje", "porciento"},
        {"porcentaje", "porciento"}, {"porsentaje", "porciento"},
        {"porcentual", "porciento"}, {"procentaje", "porciento"},
        {"porcento", "porciento"}, {"porsiento", "porciento"},
        {"porceinto", "porciento"}, {"porsentage", "porciento"},
        {"porcenta", "porciento"}
    };
    for (const auto& r : reps) {
        std::string from = r.from, to = r.to;
        size_t pos;
        while ((pos = s.find(from)) != std::string::npos) {
            s.replace(pos, from.size(), to);
        }
    }
    return s;
}

// Diccionarios
std::vector<std::string> D_SUBIR = { "sube","subir","subele","subirle","aumenta","aumentale","aumentar","incrementa","incrementale","incrementar","eleva","elevale","elevar","alza","alzale","alzar","mas","monta","montale","escala","suma","sumale","sumar","agrega","agregale","agregar","anade","anadele","añade","añadele","concatena","concetalo","concatenalo","impulsa","mejora","potencia","acelera","levanta","levantale","levantar","crece","crecele","amplia","ampliale","amplificar","expande","magnifica","arriba","alto","activa","enciende","prende","despierta","aviva","intensifica"};
std::vector<std::string> D_BAJAR = { "baja","bajar","bajale","bajarle","reduce","reducele","reducir","disminuye","disminuyele","disminuir","decrece","decrecer","desciende","descender","merma","mermale","menos","aminora","aminorale","aminorar","recorta","recortale","recortar","quita","quitale","quitar","resta","restale","restar","saca","sacale","sacar","elimina","eliminale","eliminar","remueve","remuevele","remover","suprime","suprimile","suprimir","decrementa","decrementale","achica","achicale","minora","minorale","contrae","contraele","rebaja","rebajale","abajo","bajo"};
std::vector<std::string> D_PONER = { "pon","poner","ponle","coloca","colocale","colocar","fija","fijale","fijar","establece","establecele","establecer","ajusta","ajustale","ajustar","configura","configurale","configurar","asigna","asignale","asignar","deja","dejale","dejar","cambia","cambiale","cambiar","set","setea","setealo","seteala","setear","configuralo","configurala","ajustalo","ajustala","asienta","asientale","asentar","define","definile","marca","marcele","selecciona","elige","eligele"};
std::vector<std::string> D_MUTE = { "mute","mutea","mutealo","muteala","muteame","mutear","silencio","silencia","silencie","silenciar","silenciame","mudo","muda","mudes","cala","acalla","acallalo","amordaza","enmudece","enmudecele","enmudecer","calla","callalo","callar","sordina","tapona","tapone","inhibe"};
std::vector<std::string> D_DESMUTE = { "desmute","desmutea","desmutealo","desmuteala","desmuteame","desmutear","unmute","desilencia","desilencie","desilenciar","quitasilencio","quitasilencios","reactiva","reactivale","reactivar","restaura","restaurame","restaurar","resonoriza","resonorizar"};
std::vector<std::string> D_MAX = { "max","maximo","maxima","tope","cien","100","todo","todos","toda","todas","lleno","llena","completo","completa","full","almaximo","altope","afull","peak","pico","cap","extremo","extrema","pleno","plena","repleto","cima"};
std::vector<std::string> D_MIN = { "min","minimo","minima","nada","cero","0","vacio","vacia","nulo","nula","apagado","apagada","alminimo","alcero","alfondo","floor","piso","basal","nihil","niente","nanai"};
std::vector<std::string> D_APAGAR = {"apaga", "apagar", "duerme", "bloquea", "suspende"};

std::vector<std::string> D_BRILLO = { "brillo","brillos","luz","luces","pantalla","pantallas","display","displays","monitor","monitores","luminosidad","luminancia","luminico","luminica","claridad","claro","claros","reflejo","reflejos","resplandor","resplandeciente","panel","backlight","retroiluminacion","iluminacion","visibilidad","screen","screens","contraste","intensidad","lampara","flash","destello","fulgor","luminiscencia","nits","candela","lux"};
std::vector<std::string> D_VOLUMEN = { "volumen","audio","sonido","sonidos","musica","musical","parlante","parlantes","bocina","bocinas","altavoz","altavoces","altoparlante","altoparlantes","auricular","auriculares","audifonos","audifono","ruido","ruidos","decibelios","decibeles","voz","voces","multimedia","reproductor","reproductores","cancion","canciones","estereo","equipo","hifi","speaker","speakers","amplificador","ampli","melodia","track","pista","tono","banda","radio","estacion","reproduccion","playback","beats"};
std::vector<std::string> D_FONDO = {"fondo", "wallpaper", "imagen", "foto", "tapiz", "escritorio", "lienzo", "wall", "background"};

std::vector<std::string> D_CEGAR = {"cegar", "ciega", "cegando", "duele", "lastima", "molesta", "quema"};
std::vector<std::string> D_LEER = {"leer", "leyendo", "estudiar", "estudiando", "trabajar", "trabajando"};

std::vector<std::string> D_MEDIO = { "medio", "media", "medias", "medios", "mitad", "intermedio", "intermedia", "half", "middle", "cincuenta", "50", "center", "centro", "moderado", "equilibrado", "balanced", "promedio" };
std::vector<std::string> D_CUARTO = { "cuarto", "cuarta", "quarter", "veinticinco", "25" };
std::vector<std::string> D_DOBLE = { "doble", "doblar", "double", "duplica", "duplicar", "dosveces" };

std::string hist_path() {
    const char* hh=getenv("HOME");
    std::string d=std::string(hh?hh:".")+"/.local/state/actriz";
    system(("mkdir -p "+d).c_str());
    return d+"/abc-history.txt";
}
bool cargar_historial(std::string& e, std::string& a, int& v) {
    std::ifstream f(hist_path()); std::string l,last;
    while (std::getline(f,l)) if (!l.empty()) last=l;
    if (last.empty()) return false;
    std::istringstream is(last);
    return !!(is>>e>>a>>v);
}
void guardar_historial(const std::string& e, const std::string& a, int v) {
    std::ifstream f(hist_path()); std::vector<std::string> ls; std::string l;
    while (std::getline(f,l)) if (!l.empty()) ls.push_back(l);
    f.close();
    ls.push_back(e+" "+a+" "+std::to_string(v));
    while (ls.size()>5) ls.erase(ls.begin());
    std::ofstream o(hist_path());
    for (auto& x : ls) o << x << "\n";
}

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
        
        if (!palabra.empty()) {
            if (palabra == "porciento") continue;
            palabras.push_back(palabra);
        }
    }
    if (palabras.empty()) return;

    // CTX-V66B
    palabras.erase(std::remove_if(palabras.begin(), palabras.end(),
        [](const std::string& p){
            return p=="porciento"||p=="porcien"||p=="porcentaje"||p=="porcentajes"||
                   p=="porsentaje"||p=="porsiento"||p=="porsentage"||
                   p=="pct"||p=="percent"||p=="percentage"||p=="pctje";
        }), palabras.end());
    if (palabras.empty()) return;
    {
        bool hp=0,hm=0,hn=0,ht=0,ha=0,hb=0,har=0,hab=0,hv=0,hd=0,hc=0;
        for (auto& p : palabras) {
            if (p=="poco") hp=1;
            if (p=="mas"||p=="más") hm=1;
            if (p=="no") hn=1;
            if (p=="tanto") ht=1;
            if (p=="alto"||p=="alta") ha=1;
            if (p=="bajo"||p=="baja") hb=1;
            if (p=="arriba") har=1;
            if (p=="abajo") hab=1;
            if (p=="vez") hv=1;
            if (p=="deshaz"||p=="revierte"||p=="corrige"||p=="deshacer"||p=="revertir") hd=1;
            if (p=="continua"||p=="sigue"||p=="repite") hc=1;
        }
        int ctx=0;
        if ((hn&&ht)||hd) ctx=2;
        else if ((hp&&hm)||hc||(hm&&hv)) ctx=1;
        else if (hm&&ha) ctx=3;
        else if (hm&&har) ctx=3;
        else if (hm&&hb) ctx=4;
        else if (hm&&hab) ctx=4;
        if (ctx>0) {
            bool verbo=0;
            for (auto& p : palabras) {
                if (p=="mas"||p=="más") continue;
                if (en_dict(p,D_SUBIR)||en_dict(p,D_BAJAR)||en_dict(p,D_PONER)||en_dict(p,D_MUTE)||en_dict(p,D_APAGAR)) { verbo=1; break; }
            }
            if (!verbo) {
                std::string he,haa; int hv2=0;
                if (!cargar_historial(he,haa,hv2)) {
                    std::cout << "\033[1;33m[ABC]:\033[0m (contexto) Aun no hay historial.\n";
                    return;
                }
                std::string tgt=he;
                for (auto& p : palabras) {
                    if (en_dict(p,D_BRILLO)) { tgt="brillo"; break; }
                    if (en_dict(p,D_VOLUMEN)) { tgt="volumen"; break; }
                }
                std::string ac=haa;
                if (tgt!=he) ac = (ctx==4||ctx==2) ? "bajar" : "subir";
                else {
                    if (ctx==3) ac="subir";
                    else if (ctx==4) ac="bajar";
                    else if (ctx==2) ac = (haa=="subir"||haa=="cambiar") ? "bajar" : "subir";
                }
                int paso=10;
                std::string scr=(tgt=="volumen")?"/opt/actriz/drivers/audio/control.sh":"/opt/actriz/drivers/brillo/control.sh";
                system((scr+" "+ac+" "+std::to_string(paso)).c_str());
                guardar_historial(tgt,ac,paso);
                print_interp(ac, tgt, paso, "(contexto)");
                return;
            }
        }
    }

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

    // NUEVO: si solo dijo mute/silencio/mudo, asumir volumen
    // DESMUTE-LOGIC
    if (entidad.empty()) {
        for (const auto& p : palabras) {
            if (en_dict(p, D_MUTE) || en_dict(p, D_DESMUTE)) { entidad = "volumen"; break; }
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
        if (en_dict(p, D_MEDIO)) { valor = 50; if(accion.empty()) accion = "cambiar"; }
        if (en_dict(p, D_CUARTO)) { valor = 25; if(accion.empty()) accion = "cambiar"; }
        if (en_dict(p, D_DOBLE)) { valor = 100; if(accion.empty()) accion = "subir"; }

        if (es_exacto(p, D_MAX)) { accion = "cambiar"; valor = 100; }
        if (es_exacto(p, D_MIN)) { accion = "cambiar"; valor = 1; }
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
        // DESMUTE-ACT
        if (en_dict(p, D_DESMUTE) && entidad == "volumen") accion = "desmute";
        if (en_dict(p, D_MUTE) && entidad == "volumen" && accion != "desmute") accion = "mute";
    }

    // TODO-ARR-ABA
    {
        bool tiene_todo = false, tiene_abajo = false, tiene_arriba = false;
        for (const auto& p : palabras) {
            if (p == "todo" || p == "todos" || p == "toda" || p == "todas") tiene_todo = true;
            if (p == "abajo" || p == "bajo") tiene_abajo = true;
            if (p == "arriba" || p == "alto") tiene_arriba = true;
        }
        if (tiene_todo && tiene_abajo && !tiene_arriba) { accion = "cambiar"; valor = 1; }
        else if (tiene_todo && tiene_arriba && !tiene_abajo) { accion = "cambiar"; valor = 100; }
    }

    if (accion.empty()) {
        if (valor != -1) {
            accion = "cambiar";
        } else {
            std::cout << "\033[1;31m[ABC]: No entendí la acción para " << entidad << ".\033[0m\n";
            return;
        }
    }

    std::string script = (entidad == "volumen") ? "/opt/actriz/drivers/audio/control.sh" : "/opt/actriz/drivers/brillo/control.sh";
    std::string cmd = script + " " + accion;
    if (valor != -1 && accion != "mute" && accion != "apagar") cmd += " " + std::to_string(valor);
    print_interp(accion, entidad, valor);
    system(cmd.c_str());
    // SAVE-HIST
    { int hh=(valor>0)?valor:((accion=="mute")?0:5); guardar_historial(entidad,accion,hh); }
}

int main(int argc, char* argv[]) {
    init_color(argc, argv);
    if (argc < 2) { mostrar_ayuda(); return 0; }

    std::string texto = "";
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
        texto += arg + " ";
    }
    texto = normalizar_porcentaje(texto);

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
