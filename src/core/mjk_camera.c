/**
 * @file mjk_camera.c
 * @brief Implementación de la cinemática de cámara, scroll de escenarios y registros PPU.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: RES-019, asm/src/prg.asm (loc_899D, loc_8A06, MainSub_3)
 *                  y asm/src/data.asm (AdjacentRoomsTable).
 */

#include "mjk_camera.h"
#include <string.h>

void camera_init(CameraState* cam) {
    if (!cam) return;
    memset(cam, 0, sizeof(CameraState));
    cam->mode = CAMERA_MODE_SINGLE_SCREEN;
    cam->room_flags = MJK_RF_SINGLE_SCREEN;
    cam->current_room_id = 1;
    cam->seam_room_id = 1;
    cam->ppu_mask = 0x06; // Muestra 8px izquierdos por defecto (BG y SPR)
    cam->ppu_ctrl = 0x00; // Nametable 0 base ($2000)
    cam->at_scroll_edge = false;
    cam->left_8px_clipped = false;
}

CameraMode camera_determine_mode(uint8_t room_id) {
    if (room_id >= MJK_TOTAL_ROOMS_COUNT) {
        return CAMERA_MODE_SINGLE_SCREEN;
    }
    uint8_t raw_flags = g_adjacent_rooms[room_id].flags;
    // asm/src/prg.asm:2602-2615 (MaybeLoadRoomFlags):
    // Bit 0 original ($01) = RF_SingleScreen
    if (raw_flags & 0x01) {
        return CAMERA_MODE_SINGLE_SCREEN;
    }
    // Bit 2 original ($04) = RF_ScrollStop (1 = Horizontal, 0 = Vertical)
    if (raw_flags & 0x04) {
        return CAMERA_MODE_HORIZONTAL_CORRIDOR;
    }
    return CAMERA_MODE_VERTICAL_SHAFT;
}

void camera_reset(CameraState* cam, uint8_t room_id) {
    if (!cam) return;

    cam->current_room_id = room_id;
    cam->seam_room_id = room_id;
    cam->delta_cam_x = 0;
    cam->delta_cam_y = 0;
    cam->delta_cam_x_subpx = 0;
    cam->delta_cam_y_subpx = 0;
    cam->mode = camera_determine_mode(room_id);

    uint8_t raw_flags = (room_id < MJK_TOTAL_ROOMS_COUNT) ? g_adjacent_rooms[room_id].flags : 0x01;

    switch (cam->mode) {
        case CAMERA_MODE_SINGLE_SCREEN:
            cam->room_flags = MJK_RF_SINGLE_SCREEN;
            cam->scroll_x_fixed = 0;
            cam->scroll_y_fixed = 0;
            cam->ppu_mask = 0x06;  // Borde izquierdo visible
            cam->ppu_ctrl &= ~0x02; // Nametable $2000
            cam->at_scroll_edge = false;
            cam->left_8px_clipped = false;
            break;

        case CAMERA_MODE_HORIZONTAL_CORRIDOR:
            cam->room_flags = MJK_RF_SCROLL_STOP;
            cam->ppu_mask = 0x00;  // Oculta 8px izquierdos para seam streaming
            cam->ppu_ctrl &= ~0x02;
            cam->left_8px_clipped = true;
            cam->scroll_y_fixed = 0;

            // asm/src/prg.asm:2734-2755 (DrawFullScreen):
            // Si bit 1 está activo, es un extremo de sección
            if (raw_flags & 0x02) {
                if (raw_flags & 0x08) {
                    // Inicio de sección (Borde Oeste / Izquierdo)
                    cam->room_flags |= (MJK_RF_SECTION_START | MJK_RF_AT_SCROLL_EDGE);
                    cam->scroll_x_fixed = 0;
                    cam->at_scroll_edge = true;
                } else {
                    // Fin de sección (Borde Este / Derecho)
                    cam->room_flags |= (MJK_RF_HORIZONTAL | MJK_RF_AT_SCROLL_EDGE);
                    cam->scroll_x_fixed = 0;
                    cam->at_scroll_edge = true;
                }
            } else {
                cam->scroll_x_fixed = 0;
                cam->at_scroll_edge = false;
            }
            break;

        case CAMERA_MODE_VERTICAL_SHAFT:
            cam->room_flags = 0;
            cam->ppu_mask = 0x06;  // Pantalla completa visible
            cam->ppu_ctrl &= ~0x02; // Nametable $2000 inicial
            cam->left_8px_clipped = false;
            cam->scroll_x_fixed = 0;
            cam->scroll_y_fixed = 0;

            if (raw_flags & 0x02) {
                if (raw_flags & 0x08) {
                    // Inicio de sección vertical (Borde Norte / Techo)
                    cam->room_flags |= (MJK_RF_SECTION_START | MJK_RF_AT_SCROLL_EDGE);
                    cam->at_scroll_edge = true;
                } else {
                    // Fin de sección vertical (Borde Sur / Suelo)
                    cam->room_flags |= (MJK_RF_HORIZONTAL | MJK_RF_AT_SCROLL_EDGE);
                    cam->at_scroll_edge = true;
                }
            } else {
                cam->at_scroll_edge = false;
            }
            break;
    }
}

void camera_update_kinematics(CameraState* cam, const MjkAdjacentRoom* adj_table,
                              JackPlayer* jack, const MjkInputState* inputs) {
    (void)inputs;
    if (!cam || !jack) return;

    /* ========================================================================
     * PASO 1: CINEMÁTICA Y ANCLAJE DE JACK EN PANTALLA (DEADZONES)
     * FUENTE DE VERDAD: asm/src/prg.asm:1487-1518 (Y) y prg.asm:1548-1604 (X)
     * ======================================================================== */
    switch (cam->mode) {
        case CAMERA_MODE_SINGLE_SCREEN:
            // En sala fija, Jack se desplaza libremente en coordenadas de pantalla
            cam->at_scroll_edge = false;
            break;

        case CAMERA_MODE_HORIZONTAL_CORRIDOR: {
            // Eje Y: libre movimiento vertical
            // Eje X: anclaje a X=128 ($80) salvo al llegar a bordes (RF_AtScrollEdge)
            if (cam->room_flags & MJK_RF_AT_SCROLL_EDGE) {
                // Jack está desanclado y se desplaza libremente por la pantalla
                int16_t jx = FIXED_TO_INT(jack->pos_x);
                int16_t vx = jack->subpixel_vx;

                // Re-anclaje bidireccional (loc_8A5B / RES-019B Section 2.3):
                if (cam->room_flags & MJK_RF_SECTION_START) {
                    // Borde izquierdo: Jack camina libremente entre 0 y 128.
                    // Solo se re-ancla al caminar hacia el interior (derecha) y alcanzar X=128
                    if (vx > 0 && jx >= MJK_JACK_CENTER_X) {
                        cam->room_flags &= ~(MJK_RF_AT_SCROLL_EDGE | MJK_RF_SECTION_START);
                        cam->at_scroll_edge = false;
                        jack->pos_x = FIXED_FROM_INT(MJK_JACK_CENTER_X);
                    }
                } else if (cam->room_flags & MJK_RF_HORIZONTAL) {
                    // Borde derecho: Jack camina libremente entre 128 y 255.
                    // Solo se re-ancla al caminar hacia el interior (izquierda) y alcanzar X<128
                    if (vx < 0 && jx < MJK_JACK_CENTER_X) {
                        cam->room_flags &= ~MJK_RF_AT_SCROLL_EDGE;
                        cam->at_scroll_edge = false;
                        jack->pos_x = FIXED_FROM_INT(MJK_JACK_CENTER_X);
                    }
                } else {
                    if (vx > 0 && jx >= MJK_JACK_CENTER_X) {
                        cam->room_flags &= ~(MJK_RF_AT_SCROLL_EDGE | MJK_RF_SECTION_START);
                        cam->at_scroll_edge = false;
                        jack->pos_x = FIXED_FROM_INT(MJK_JACK_CENTER_X);
                    } else if (vx < 0 && jx < MJK_JACK_CENTER_X) {
                        cam->room_flags &= ~MJK_RF_AT_SCROLL_EDGE;
                        cam->at_scroll_edge = false;
                        jack->pos_x = FIXED_FROM_INT(MJK_JACK_CENTER_X);
                    }
                }
            } else {
                // Jack permanece anclado en el centro horizontal exacto X=128
                jack->pos_x = FIXED_FROM_INT(MJK_JACK_CENTER_X);
            }
            break;
        }

        case CAMERA_MODE_VERTICAL_SHAFT: {
            // Eje X: libre movimiento horizontal
            // Eje Y: anclaje a Y=121 ($79) salvo al llegar a bordes (RF_AtScrollEdge)
            if (cam->room_flags & MJK_RF_AT_SCROLL_EDGE) {
                int16_t jy = FIXED_TO_INT(jack->pos_y);
                int16_t vy = jack->subpixel_vy;

                // Re-anclaje bidireccional vertical (prg.asm:1500-1517):
                if (cam->room_flags & MJK_RF_SECTION_START) {
                    // Borde superior: Jack camina/salta libremente. Re-ancla al descender a Y=121
                    if (vy > 0 && jy >= MJK_JACK_CENTER_Y) {
                        cam->room_flags &= ~MJK_RF_AT_SCROLL_EDGE;
                        cam->at_scroll_edge = false;
                        jack->pos_y = FIXED_FROM_INT(MJK_JACK_CENTER_Y);
                    }
                } else if (cam->room_flags & MJK_RF_HORIZONTAL) {
                    // Borde inferior: Re-ancla al ascender a Y<121
                    if (vy < 0 && jy < MJK_JACK_CENTER_Y) {
                        cam->room_flags &= ~MJK_RF_AT_SCROLL_EDGE;
                        cam->at_scroll_edge = false;
                        jack->pos_y = FIXED_FROM_INT(MJK_JACK_CENTER_Y);
                    }
                } else {
                    if (vy > 0 && jy >= MJK_JACK_CENTER_Y) {
                        cam->room_flags &= ~MJK_RF_AT_SCROLL_EDGE;
                        cam->at_scroll_edge = false;
                        jack->pos_y = FIXED_FROM_INT(MJK_JACK_CENTER_Y);
                    } else if (vy < 0 && jy < MJK_JACK_CENTER_Y) {
                        cam->room_flags &= ~MJK_RF_AT_SCROLL_EDGE;
                        cam->at_scroll_edge = false;
                        jack->pos_y = FIXED_FROM_INT(MJK_JACK_CENTER_Y);
                    }
                }
            } else {
                // Jack permanece anclado en el centro vertical Y=121
                jack->pos_y = FIXED_FROM_INT(MJK_JACK_CENTER_Y);
            }
            break;
        }
    }

    /* ========================================================================
     * PASO 2: AVANCE DE SCROLL DE CÁMARA (MainSub_3)
     * FUENTE DE VERDAD: asm/src/prg.asm:5906-6410 (loc_A454 y loc_A63C)
     * ======================================================================== */
    cam->delta_cam_x = 0;
    cam->delta_cam_y = 0;
    cam->delta_cam_x_subpx = 0;
    cam->delta_cam_y_subpx = 0;

    if (cam->mode == CAMERA_MODE_SINGLE_SCREEN) {
        cam->scroll_x_fixed = 0;
        cam->scroll_y_fixed = 0;
        return;
    }

    if (cam->room_flags & MJK_RF_AT_SCROLL_EDGE) {
        // En los límites del escenario, la cámara permanece bloqueada
        return;
    }

    const MjkAdjacentRoom* adj = adj_table ? &adj_table[cam->current_room_id] : NULL;

    if (cam->mode == CAMERA_MODE_HORIZONTAL_CORRIDOR) {
        cam->ppu_mask &= ~0x06; // Oculta columna izquierda de 8px para seam
        cam->left_8px_clipped = true;

        int16_t vx = jack->subpixel_vx;
        if (vx == 0) return;

        // Delta de cámara para entidades activas (-PlayerXVel)
        cam->delta_cam_x_subpx = (int16_t)(-vx);
        cam->delta_cam_x = (int16_t)(-(vx >> 8));

        if (vx < 0) {
            // Moviendo a la izquierda (prg.asm:5987-6024):
            // Limpia flag de fin de sección derecha
            cam->room_flags &= ~MJK_RF_HORIZONTAL;

            int32_t new_x = (int32_t)cam->scroll_x_fixed + vx;
            if (new_x >= 0) {
                cam->scroll_x_fixed = (uint16_t)new_x;
            } else {
                // Subdesborde por debajo de 0 (Carry = 0 en NES):
                uint8_t left_room = adj ? adj->left : 0;
                if (left_room != 0) {
                    // Transición limpia de sala hacia la izquierda (loc_A4DD)
                    cam->scroll_x_fixed = (uint16_t)(new_x + 0x10000);
                    cam->current_room_id = left_room;
                    const MjkAdjacentRoom* prev_adj = &adj_table[left_room];
                    cam->seam_room_id = prev_adj->left ? prev_adj->left : left_room;
                    // Paridad loc_A4DD: Omitir loc_A4D2 (no inyectar MJK_RF_SECTION_START en scroll_x = 255)
                    cam->room_flags &= ~MJK_RF_SECTION_START;
                } else {
                    // Borde inicial absoluto del pasillo (loc_A4D2): anclar cámara a 0 y desanclar a Jack
                    cam->room_flags |= (MJK_RF_SECTION_START | MJK_RF_AT_SCROLL_EDGE);
                    cam->at_scroll_edge = true;
                    cam->scroll_x_fixed = 0;
                }
            }
        } else {
            // Moviendo a la derecha (prg.asm:6070-6111):
            // Limpia flag de inicio de sección (loc_A57A)
            cam->room_flags &= ~MJK_RF_SECTION_START;

            uint32_t total_x = (uint32_t)cam->scroll_x_fixed + (uint16_t)vx;
            if (total_x < 0x10000) {
                cam->scroll_x_fixed = (uint16_t)total_x;
            } else {
                // Desborde de 256 píxeles (Carry = 1 en NES):
                uint8_t right_room = adj ? adj->right : 0;
                if (right_room != 0) {
                    // Transición limpia de sala hacia la derecha (loc_A55E)
                    cam->scroll_x_fixed = (uint16_t)(total_x - 0x10000);
                    cam->current_room_id = right_room;
                    const MjkAdjacentRoom* next_adj = &adj_table[right_room];
                    cam->seam_room_id = next_adj->right ? next_adj->right : right_room;
                    if (next_adj->right == 0 || (next_adj->flags & 0x02)) {
                        cam->room_flags |= MJK_RF_HORIZONTAL;
                    }
                } else {
                    // Borde terminal absoluto del pasillo: bloquear cámara y desanclar a Jack (loc_A554)
                    cam->room_flags |= (MJK_RF_HORIZONTAL | MJK_RF_AT_SCROLL_EDGE);
                    cam->at_scroll_edge = true;
                    // Paridad loc_A554: Preservar ScrollX en fin de sección derecha sin forzar reset a 0
                }
            }
        }
    } else {
        // --- SCROLL VERTICAL ---
        cam->ppu_mask |= 0x06; // Pantalla completa visible
        cam->left_8px_clipped = false;

        int16_t vy = jack->subpixel_vy;
        if (vy == 0) return;

        // Delta de cámara para entidades activas (-PlayerYVel)
        cam->delta_cam_y_subpx = (int16_t)(-vy);
        cam->delta_cam_y = (int16_t)(-(vy >> 8));

        if (vy < 0) {
            // Subiendo (ascenso)
            if (cam->room_flags & MJK_RF_SECTION_START) {
                cam->scroll_y_fixed = 0;
                cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
                cam->at_scroll_edge = true;
                return;
            }

            cam->room_flags &= ~MJK_RF_HORIZONTAL;
            int32_t y_fixed = (int32_t)cam->scroll_y_fixed + vy;

            if (y_fixed < 0) {
                // Underflow por debajo de 0: wrap módulo 240 líneas (asm: loc_A66F)
                y_fixed += (240 << 8);

                if (adj && adj->up != 0) {
                    cam->current_room_id = adj->up;
                    cam->seam_room_id = adj->up;
                    cam->ppu_ctrl ^= 0x02; // Alterna bit 1 de PPUCTRL ($2000 <-> $2800)
                } else {
                    cam->room_flags |= MJK_RF_SECTION_START;
                    y_fixed = 0;
                }
            }
            cam->scroll_y_fixed = (uint16_t)y_fixed;
        } else {
            // Bajando (descenso, vy > 0)
            if (cam->room_flags & MJK_RF_HORIZONTAL) {
                cam->scroll_y_fixed = 0;
                cam->room_flags |= MJK_RF_AT_SCROLL_EDGE;
                cam->at_scroll_edge = true;
                return;
            }

            cam->room_flags &= ~MJK_RF_SECTION_START;
            int32_t y_fixed = (int32_t)cam->scroll_y_fixed + vy;

            if (y_fixed >= (240 << 8)) {
                // Overflow por encima de 240: wrap módulo 240 líneas (asm: loc_A716)
                y_fixed -= (240 << 8);
                cam->ppu_ctrl ^= 0x02; // Alterna bit 1 de PPUCTRL

                if (adj && adj->down != 0) {
                    cam->current_room_id = adj->down;
                    cam->seam_room_id = adj->down;
                } else {
                    cam->room_flags |= MJK_RF_HORIZONTAL;
                    y_fixed = 0;
                }
            }
            cam->scroll_y_fixed = (uint16_t)y_fixed;
        }
    }
}

void camera_apply_entity_delta(const CameraState* cam, int16_t* entity_x, int16_t* entity_y) {
    if (!cam) return;
    if (entity_x) {
        *entity_x = (int16_t)(*entity_x + cam->delta_cam_x);
    }
    if (entity_y) {
        *entity_y = (int16_t)(*entity_y + cam->delta_cam_y);
    }
}

void camera_apply_entity_delta_fixed(const CameraState* cam, fixed8_t* entity_x, fixed8_t* entity_y) {
    if (!cam) return;
    if (entity_x) {
        *entity_x = (fixed8_t)(*entity_x + cam->delta_cam_x_subpx);
    }
    if (entity_y) {
        *entity_y = (fixed8_t)(*entity_y + cam->delta_cam_y_subpx);
    }
}

void camera_to_screen_coords(const CameraState* cam, int16_t world_x, int16_t world_y,
                             int16_t* screen_x, int16_t* screen_y) {
    if (!cam) {
        if (screen_x) *screen_x = world_x;
        if (screen_y) *screen_y = world_y;
        return;
    }
    if (screen_x) {
        *screen_x = (int16_t)(world_x - (int16_t)(cam->scroll_x_fixed >> 8));
    }
    if (screen_y) {
        *screen_y = (int16_t)(world_y - (int16_t)(cam->scroll_y_fixed >> 8));
    }
}

void camera_get_ppu_scroll(const CameraState* cam, uint8_t* ppu_scroll_x, uint8_t* ppu_scroll_y,
                           uint8_t* ppu_ctrl_bits) {
    if (!cam) return;
    if (ppu_scroll_x) {
        *ppu_scroll_x = (uint8_t)((cam->scroll_x_fixed >> 8) & 0xFF);
    }
    if (ppu_scroll_y) {
        *ppu_scroll_y = (uint8_t)((cam->scroll_y_fixed >> 8) & 0xFF);
    }
    if (ppu_ctrl_bits) {
        *ppu_ctrl_bits = (uint8_t)(cam->ppu_ctrl & 0x03);
    }
}

bool camera_is_left_clipped(const CameraState* cam) {
    return cam ? cam->left_8px_clipped : false;
}

bool camera_is_at_scroll_edge(const CameraState* cam) {
    return cam ? (cam->at_scroll_edge || (cam->room_flags & MJK_RF_AT_SCROLL_EDGE) != 0) : false;
}

uint8_t camera_get_current_room(const CameraState* cam) {
    return cam ? cam->current_room_id : 0;
}

uint8_t camera_get_seam_room(const CameraState* cam) {
    return cam ? cam->seam_room_id : 0;
}

uint8_t camera_get_scroll_x(const CameraState* cam) {
    return cam ? (uint8_t)((cam->scroll_x_fixed >> 8) & 0xFF) : 0;
}

uint8_t camera_get_scroll_y(const CameraState* cam) {
    return cam ? (uint8_t)((cam->scroll_y_fixed >> 8) & 0xFF) : 0;
}

bool camera_is_horizontal(const CameraState* cam) {
    return cam ? (cam->mode == CAMERA_MODE_HORIZONTAL_CORRIDOR) : false;
}

bool camera_is_vertical(const CameraState* cam) {
    return cam ? (cam->mode == CAMERA_MODE_VERTICAL_SHAFT) : false;
}
