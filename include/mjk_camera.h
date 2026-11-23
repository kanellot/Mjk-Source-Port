/**
 * @file mjk_camera.h
 * @brief Cinemática de cámara, motor de scroll de escenarios y registros PPU.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: RES-019, asm/src/prg.asm (loc_899D, loc_8A06, MainSub_3)
 *                  y asm/src/data.asm (AdjacentRoomsTable).
 */

#ifndef MJK_CAMERA_H
#define MJK_CAMERA_H

#include "mjk_types.h"
#include "mjk_platform.h"
#include "mjk_room_data.h"

#ifdef __cplusplus
extern "C" {
#endif

// Modos canónicos de cámara según topología de sala (RES-019)
typedef enum {
    CAMERA_MODE_SINGLE_SCREEN       = 0,  ///< Sala fija sin scroll (Palacio, Tortura, Secretas)
    CAMERA_MODE_HORIZONTAL_CORRIDOR = 1,  ///< Pasillo horizontal (X anclado a 128, clipping 8px)
    CAMERA_MODE_VERTICAL_SHAFT      = 2   ///< Pozo vertical / torre (Y anclado a 121, módulo 240px)
} CameraMode;

typedef CameraMode MjkCameraMode;

// Flags canónicos de RoomStatusFlags ($00BC / $03BE)
#define MJK_RF_1               0x01  ///< Flag interno / misceláneo
#define MJK_RF_BOMB_ROOM       0x02  ///< Sala Real / Bomb Room (A y B aletean)
#define MJK_RF_4               0x04  ///< Configuración de puerta de entrada
#define MJK_RF_AT_SCROLL_EDGE  0x08  ///< Cámara bloqueada en borde; Jack se mueve libre
#define MJK_RF_SINGLE_SCREEN   0x10  ///< Pantalla única fija (Scroll 0,0)
#define MJK_RF_SCROLL_STOP     0x20  ///< 1 = Horizontal, 0 = Vertical (cuando SingleScreen=0)
#define MJK_RF_HORIZONTAL      0x40  ///< Borde Este (Horizontal) o Sur (Vertical) pendiente
#define MJK_RF_SECTION_START   0x80  ///< Borde Oeste (Horizontal) o Norte (Vertical) pendiente

// Anclajes / Deadzones canónicos de Jack en pantalla (RES-019)
#define MJK_JACK_CENTER_X      128   ///< $80: Deadzone horizontal (centro de pantalla 256)
#define MJK_JACK_CENTER_Y      121   ///< $79: Deadzone vertical (centro de pantalla 240)

// Velocidad estándar de avance de scroll (idéntica a Jack: 1.5 px/f = 384 subpx)
#define MJK_SCROLL_SPEED_SUBPIXELS 384

typedef struct {
    CameraMode mode;                 ///< Modo activo según topología de la sala
    uint8_t    room_flags;           ///< RoomStatusFlags ($00BC/$03BE)
    uint8_t    current_room_id;      ///< CurrentRoomID ($03BF)
    uint8_t    seam_room_id;         ///< MaybeRoomIDCopyAgain ($03C7)

    // Coordenadas de Scroll en punto fijo Q8.8
    uint16_t   scroll_x_fixed;       ///< byte_3C1 (frac) : byte_3C2 (pixel)
    uint16_t   scroll_y_fixed;       ///< byte_3C3 (frac) : byte_3C4 (pixel)

    // Deltas de cámara para entidades (inversos al movimiento del scroll)
    int16_t    delta_cam_x_subpx;    ///< byte_98:byte_99 (-PlayerXVel)
    int16_t    delta_cam_y_subpx;    ///< byte_9A:byte_9B (-PlayerYVel)
    int16_t    delta_cam_x;          ///< Delta en píxeles enteros (delta_cam_x_subpx >> 8)
    int16_t    delta_cam_y;          ///< Delta en píxeles enteros (delta_cam_y_subpx >> 8)

    // Registros PPU
    uint8_t    ppu_ctrl;             ///< PPUCtrlMirror (bit 1: toggle nametable vertical $2800)
    uint8_t    ppu_mask;             ///< PPUMaskMirror (bits 1, 2: left 8px clipping)

    // Bounding / Estado de bordes
    bool       at_scroll_edge;       ///< True si Jack se desancla al llegar a los bordes
    bool       left_8px_clipped;     ///< True si el recorte lateral izquierdo está activo
} CameraState;

typedef CameraState MjkCameraState;

/**
 * @brief Inicializa la estructura de cámara en estado por defecto (pantalla fija 0,0).
 */
void camera_init(CameraState* cam);

/**
 * @brief Resetea la cámara para una sala concreta, determinando su modo topológico,
 *        flags de sala y registros PPU iniciales a partir de la tabla de adyacencia.
 * 
 * @param cam Puntero al estado de la cámara.
 * @param room_id Identificador de la sala activa (1..256).
 */
void camera_reset(CameraState* cam, uint8_t room_id);

/**
 * @brief Determina el modo de cámara a partir de los flags del byte 0 de la sala.
 */
CameraMode camera_determine_mode(uint8_t room_id);

/**
 * @brief Actualiza la cinemática de la cámara y el anclaje de Jack según los controles
 *        y la física de movimiento del frame.
 * 
 * @param cam Puntero al estado de la cámara.
 * @param adj_table Tabla de adyacencia de salas (g_adjacent_rooms).
 * @param jack Puntero a la estructura del jugador.
 * @param inputs Estado de mandos del frame (opcional, puede ser NULL).
 */
void camera_update_kinematics(CameraState* cam, const MjkAdjacentRoom* adj_table,
                              JackPlayer* jack, const MjkInputState* inputs);

/**
 * @brief Aplica el delta inverso de la cámara a una entidad que reside en coordenadas de pantalla.
 * 
 * @param cam Puntero al estado de la cámara.
 * @param entity_x Puntero a la coordenada X en pantalla (píxeles enteros).
 * @param entity_y Puntero a la coordenada Y en pantalla (píxeles enteros).
 */
void camera_apply_entity_delta(const CameraState* cam, int16_t* entity_x, int16_t* entity_y);

/**
 * @brief Aplica el delta inverso de la cámara a una entidad en coordenadas de pantalla Q8.8.
 */
void camera_apply_entity_delta_fixed(const CameraState* cam, fixed8_t* entity_x, fixed8_t* entity_y);

/**
 * @brief Convierte coordenadas absolutas de mundo a coordenadas relativas de pantalla.
 * 
 * @param cam Puntero al estado de la cámara.
 * @param world_x Coordenada horizontal en espacio de mundo.
 * @param world_y Coordenada vertical en espacio de mundo.
 * @param screen_x Puntero de salida para la coordenada horizontal en pantalla.
 * @param screen_y Puntero de salida para la coordenada vertical en pantalla.
 */
void camera_to_screen_coords(const CameraState* cam, int16_t world_x, int16_t world_y,
                             int16_t* screen_x, int16_t* screen_y);

/**
 * @brief Extrae los registros de scroll de la NES para el hardware/emulador PPU.
 * 
 * @param cam Puntero al estado de la cámara.
 * @param ppu_scroll_x Salida del byte de scroll X (escrito en $2005 1º write, 0..255).
 * @param ppu_scroll_y Salida del byte de scroll Y (escrito en $2005 2º write, 0..239).
 * @param ppu_ctrl_bits Salida de los bits 0-1 de PPUCTRL ($2000) para selección de nametable base.
 */
void camera_get_ppu_scroll(const CameraState* cam, uint8_t* ppu_scroll_x, uint8_t* ppu_scroll_y,
                           uint8_t* ppu_ctrl_bits);

/**
 * @brief Consulta si el clipping de los 8 píxeles izquierdos de la PPU está habilitado.
 */
bool camera_is_left_clipped(const CameraState* cam);

/**
 * @brief Consulta si la cámara se encuentra bloqueada en un borde de scroll (Jack desanclado).
 */
bool camera_is_at_scroll_edge(const CameraState* cam);

/**
 * @brief Obtiene el ID de la sala activa según la cámara ($03BF CurrentRoomID).
 */
uint8_t camera_get_current_room(const CameraState* cam);

/**
 * @brief Obtiene el ID de la sala contigua para costura ($03C7 MaybeRoomIDCopyAgain).
 */
uint8_t camera_get_seam_room(const CameraState* cam);

/**
 * @brief Obtiene la coordenada X entera de scroll PPU (0..255).
 */
uint8_t camera_get_scroll_x(const CameraState* cam);

/**
 * @brief Obtiene la coordenada Y entera de scroll PPU (0..239).
 */
uint8_t camera_get_scroll_y(const CameraState* cam);

/**
 * @brief Consulta si la cámara está en modo pasillo horizontal (RF_ScrollStop).
 */
bool camera_is_horizontal(const CameraState* cam);

/**
 * @brief Consulta si la cámara está en modo pozo vertical.
 */
bool camera_is_vertical(const CameraState* cam);

#ifdef __cplusplus
}
#endif

#endif // MJK_CAMERA_H
