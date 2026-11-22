# ============================================================
#  Actriz OS / Keu OS — Makefile raíz
# ============================================================
CC      = gcc
CXX     = g++
CFLAGS  = -O2 -Wall -Wextra
CXXFLAGS= -O2 -Wall -std=c++17

# ─── Detección de dependencias ────────────────────────────────
GTK3_CFLAGS   := $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK3_LIBS     := $(shell pkg-config --libs   gtk+-3.0 2>/dev/null)
VTE_CFLAGS    := $(shell pkg-config --cflags vte-2.91 2>/dev/null)
VTE_LIBS      := $(shell pkg-config --libs   vte-2.91 2>/dev/null)
CAIRO_CFLAGS  := $(shell pkg-config --cflags cairo   2>/dev/null)
CAIRO_LIBS    := $(shell pkg-config --libs   cairo   2>/dev/null)

# ─── Binarios ─────────────────────────────────────────────────
BIN_MAIN   = bin/terminal bin/abc-app bin/abc-term
BIN_WIDGET = entornografico/navegacion/ISLA_SUPERIOR/isla \
             entornografico/navegacion/ISLA_SUPERIOR/bote_reloj \
             entornografico/navegacion/ISLA_SUPERIOR/bote_wifi_bat \
             entornografico/navegacion/ISLA_INFERIOR/isla_inferior \
             entornografico/navegacion/ISLA_INFERIOR/dock \
             entornografico/navegacion/BORDE/borde \
             entornografico/aplicaciones/SISTEMA/sistema
BIN_ABC    = ABC/abc

ALL = $(BIN_MAIN) $(BIN_WIDGET) $(BIN_ABC)

.PHONY: all clean help check-deps

all: $(ALL)
	@echo "✅ Compilación completa."

# ─── Motor ABC (C++ puro) ─────────────────────────────────────
ABC/abc: ABC/abc.cpp
	@echo "[CXX] $<"
	$(CXX) $(CXXFLAGS) -o $@ $<

# ─── Wrapper terminal (C puro) ────────────────────────────────
bin/terminal: bin/terminal.c
	@echo "[CC ] $<"
	$(CC) $(CFLAGS) -o $@ $<

# ─── ABC App (GTK3) ───────────────────────────────────────────
bin/abc-app: bin/abc-app.c
	@echo "[CC ] $< (GTK3)"
	$(CC) $(CFLAGS) $(GTK3_CFLAGS) -o $@ $< $(GTK3_LIBS) -lm

# ─── ABC Term (GTK3 + VTE) ────────────────────────────────────
bin/abc-term: bin/abc-term.c
	@echo "[CC ] $< (GTK3 + VTE)"
	$(CC) $(CFLAGS) $(GTK3_CFLAGS) $(VTE_CFLAGS) -o $@ $< $(GTK3_LIBS) $(VTE_LIBS) -lm

# ─── Widgets del escritorio (GTK3 + Cairo) ────────────────────
entornografico/navegacion/ISLA_SUPERIOR/isla: \
		entornografico/navegacion/ISLA_SUPERIOR/isla.c
	@echo "[CC ] $<"
	$(CC) $(CFLAGS) $(GTK3_CFLAGS) $(CAIRO_CFLAGS) -o $@ $< $(GTK3_LIBS) $(CAIRO_LIBS) -lm

entornografico/navegacion/ISLA_SUPERIOR/bote_reloj: \
		entornografico/navegacion/ISLA_SUPERIOR/bote_reloj.c
	@echo "[CC ] $<"
	$(CC) $(CFLAGS) $(GTK3_CFLAGS) $(CAIRO_CFLAGS) -o $@ $< $(GTK3_LIBS) $(CAIRO_LIBS) -lm

entornografico/navegacion/ISLA_SUPERIOR/bote_wifi_bat: \
		entornografico/navegacion/ISLA_SUPERIOR/bote_wifi_bat.c
	@echo "[CC ] $<"
	$(CC) $(CFLAGS) $(GTK3_CFLAGS) $(CAIRO_CFLAGS) -o $@ $< $(GTK3_LIBS) $(CAIRO_LIBS) -lm

entornografico/navegacion/ISLA_INFERIOR/isla_inferior: \
		entornografico/navegacion/ISLA_INFERIOR/isla_inferior.c
	@echo "[CC ] $<"
	$(CC) $(CFLAGS) $(GTK3_CFLAGS) $(CAIRO_CFLAGS) -o $@ $< $(GTK3_LIBS) $(CAIRO_LIBS) -lm

# ─── Dock (C++, GTK3) ─────────────────────────────────────────
entornografico/navegacion/ISLA_INFERIOR/dock: \
		entornografico/navegacion/ISLA_INFERIOR/dock.cpp
	@echo "[CXX] $<"
	$(CXX) $(CXXFLAGS) $(GTK3_CFLAGS) -o $@ $< $(GTK3_LIBS) -lm

# ─── Borde (GTK3 + Cairo) ─────────────────────────────────────
entornografico/navegacion/BORDE/borde: \
		entornografico/navegacion/BORDE/borde.c
	@echo "[CC ] $<"
	$(CC) $(CFLAGS) $(GTK3_CFLAGS) $(CAIRO_CFLAGS) -o $@ $< $(GTK3_LIBS) $(CAIRO_LIBS) -lm

# ─── App Sistema (GTK3) ───────────────────────────────────────
entornografico/aplicaciones/SISTEMA/sistema: \
		entornografico/aplicaciones/SISTEMA/sistema.c
	@echo "[CC ] $<"
	$(CC) $(CFLAGS) $(GTK3_CFLAGS) -o $@ $< $(GTK3_LIBS) -lm

# ─── Utilidades ───────────────────────────────────────────────
clean:
	@echo "🧹 Borrando binarios…"
	@rm -f $(ALL)
	@echo "✅ Listo."

check-deps:
	@echo "── Dependencias detectadas: ──"
	@echo "GTK3:  $(if $(GTK3_LIBS),✅,❌ falta libgtk-3-dev)"
	@echo "VTE:   $(if $(VTE_LIBS),✅,❌ falta libvte-2.91-dev)"
	@echo "Cairo: $(if $(CAIRO_LIBS),✅,❌ falta libcairo2-dev)"

help:
	@echo "Actriz OS Makefile"
	@echo ""
	@echo "  make            compila todo"
	@echo "  make clean      borra binarios"
	@echo "  make check-deps verifica dependencias"
	@echo "  make bin/abc-app      compila solo la app"
	@echo "  make ABC/abc          compila solo el motor NLP"
