#ifndef ACTRIZ_OJOS_H
#define ACTRIZ_OJOS_H

#include <cairo.h>

#define OJOS_MARGEN 10.0

/*
 * Dos ojos circulares concéntricos con los extremos redondeados
 * de la cápsula. La separación geométrica es de 10 px.
 */
static inline void dibujar_ojos(
    cairo_t *cr,
    double ancho,
    double alto
) {
    if (!cr || ancho <= 0 || alto <= 2.0 * OJOS_MARGEN)
        return;

    const double radio = (alto - 2.0 * OJOS_MARGEN) / 2.0;
    const double centro_y = alto / 2.0;

    const double centro_izquierdo = alto / 2.0;
    const double centro_derecho = ancho - alto / 2.0;

    cairo_save(cr);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);

    cairo_new_sub_path(cr);
    cairo_arc(
        cr, centro_izquierdo, centro_y,
        radio, 0, 6.283185307179586
    );

    cairo_new_sub_path(cr);
    cairo_arc(
        cr, centro_derecho, centro_y,
        radio, 0, 6.283185307179586
    );

    cairo_fill(cr);
    cairo_restore(cr);
}

#endif
