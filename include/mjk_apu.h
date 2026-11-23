/**
 * @file mjk_apu.h
 * @brief Sintetizador Chiptune APU 2A03 (Pulse 1, Pulse 2, Triangle, Noise) y SFX.
 * 
 * PROYECTO: Mighty Bomb Jack - Source Port Educacional en C
 * FUENTE DE VERDAD: RES-030, RES-014, music-engine.asm y music-data.asm
 */

#ifndef MJK_APU_H
#define MJK_APU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MJK_APU_SAMPLE_RATE     44100
#define MJK_APU_SAMPLES_FRAME   735   // 44100 / 60 = 735 muestras PCM por frame
#define MJK_APU_CALL_STACK_MAX  4

// Pistas de música chiptune canónicas (SoundData_* en music-data.asm)
typedef enum {
    MUSIC_NONE               = 0,
    MUSIC_ROUND_1            = 1,   ///< Tema principal de Round 1 (SoundData_10_Music_Main)
    MUSIC_MAIN_WITH_INTRO    = 1,   ///< Comando canónico $16 (SoundData_16_Music_MainIntro, prg.asm:620)
    MUSIC_TREASURE           = 2,   ///< Tema de salas del tesoro (SoundData_11_Music_Treasure)
    MUSIC_TORTURE            = 3,   ///< Tema de la sala de tortura (SoundData_1F_Music_Torture)
    MUSIC_GAME_OVER          = 4,   ///< Pantalla de Game Over (SoundData_17_Music_GameOver)
    MUSIC_ROUND_CLEAR        = 5,   ///< Fanfarria de Round Clear (SoundData_18_Music_RoundClear)
    MUSIC_POWER_COIN         = 6,   ///< Música de transmutación en monedas (SoundData_06_PowerCoinMusic)
    MUSIC_ENDING             = 7    ///< Música de final (SoundData_22_Music_Ending)
} MjkMusicId;

// Efectos de sonido y comandos canónicos ($00..$23 de defs.asm / music-data.asm)
typedef enum {
    SFX_NONE                  = 0x00,
    SFX_SILENCE               = 0x00,
    SFX_JUMP                  = 0x01, ///< Sound_Jump (Pulse 2 hardware sweep)
    SFX_DEATH                 = 0x02, ///< Sound_Death (Sweep descendente + pitch bend)
    SFX_PLAYER_DIE            = 0x02, ///< Alias de Sound_Death
    SFX_BONUS_COIN_SPAWN      = 0x03, ///< Sound_BonusCoinSpawn (P1 + P2)
    SFX_COIN_COLLECTED        = 0x04, ///< Sound_CoinCollected (Pulse 2)
    SFX_COLLECT_COIN          = 0x04, ///< Alias de Sound_CoinCollected
    SFX_POWER_COIN_SPAWN      = 0x05, ///< Sound_PowerCoinSpawn (Pulse 1)
    SFX_POWER_COIN_MUSIC      = 0x06, ///< Sound_PowerCoinMusic (BGM)
    SFX_POWER_COIN            = 0x06, ///< Alias de Sound_PowerCoinMusic
    SFX_COLLECT_BOMB          = 0x07, ///< Sound_BombCollectedUnlit (Pulse 2)
    SFX_BOMB_UNLIT            = 0x07, ///< Alias de Sound_BombCollectedUnlit
    SFX_FIRE_BOMB             = 0x08, ///< Sound_BombCollectedLit (Pulse 2)
    SFX_BOMB_LIT              = 0x08, ///< Alias de Sound_BombCollectedLit
    SFX_YOU_ARE_GREEDY        = 0x09, ///< Sound_YouAreGreedy (P1 + P2)
    SFX_1UP                   = 0x0A, ///< Sound_1UP_Captive (P1 + P2 + Triangle)
    SFX_1UP_CAPTIVE           = 0x0A, ///< Alias de Sound_1UP_Captive
    SFX_CRYSTAL_BALL          = 0x0B, ///< Sound_CrystalBall (P1 + P2 + Triangle)
    SFX_BONUS_STATUE          = 0x0D, ///< Sound_BonusStatueCollected (Pulse 2)
    SFX_SECRET_BLOCK          = 0x0E, ///< Sound_SecretBlockRevealed (Pulse 2)
    SFX_DOOR_OPEN             = 0x0F, ///< Sound_Door (P1 + P2 + Triangle pitch bend)
    SFX_DOOR                  = 0x0F, ///< Alias de Sound_Door
    SFX_BOMB_CHEST_BONUS      = 0x19, ///< Sound_BombChestBonus (P1 + P2 + Triangle)
    SFX_PAUSE                 = 0x1B, ///< Sound_Pause
    SFX_UNPAUSE               = 0x1C, ///< Sound_Unpause
    SFX_MIGHTY_POWER          = 0x1D, ///< Sound_UsedMightyCoin (P1 + P2 + Triangle)
    SFX_USED_MIGHTY_COIN      = 0x1D, ///< Alias de Sound_UsedMightyCoin
    SFX_BOMB_BONUS            = 0x20, ///< Sound_BombBonus (P1 + P2 + Triangle + Noise)
    SFX_TIME_PICKUP           = 0x21, ///< Sound_TimePickup (Mighty Drink)
    SFX_MIGHTY_DRINK          = 0x21, ///< Alias de Sound_TimePickup
    SFX_COLLECTED_FULL_FAMILY = 0x23, ///< Music_CollectedFullFamily (Beelzebub)
    SFX_OPEN_CHEST            = 0xFF  ///< Deprecated dummy / no-op (no produce sonido en NES)
} MjkSfxId;

/**
 * @brief Estructura de estado para los 8 canales virtuales del motor Tecmo 6502.
 */
typedef struct {
    uint8_t        status;              ///< 1 = activo, 0 = silenciado
    uint8_t        duration_counter;    ///< frames restantes de la nota actual
    uint8_t        duration_reload;     ///< duración base de la nota
    const uint8_t* data_ptr;            ///< puntero actual al flujo de bytecode
    const uint8_t* track_base;          ///< dirección base del track para saltos/calls con offset
    const uint8_t* vol_env_ptr;         ///< puntero a la tabla de envolvente activa
    uint8_t        vol_env_index;       ///< índice actual en la tabla de envolvente
    const uint8_t* loop_ptr;            ///< puntero de repetición de bucle ($F1)
    uint8_t        loop_counter;        ///< repeticiones restantes de bucle ($F1/$F2)
    uint8_t        stack_depth;         ///< profundidad de pila de subrutinas ($F3/$F4)
    const uint8_t* call_stack[MJK_APU_CALL_STACK_MAX]; ///< pila de retorno (SoundChannelB)
    uint8_t        vol_attenuation;     ///< atenuación de volumen ($Ex)
    uint8_t        vol_delay_counter;   ///< temporizador de frames de la fase de envolvente
    uint8_t        duty_and_vol;        ///< bits 7-6: duty, bits 3-0: volumen actual
    uint8_t        sweep_reg;           ///< registro Sweep de hardware ($F5)
    uint8_t        sweep_divider;       ///< divisor del hardware sweep
    uint16_t       period;              ///< período de 11 bits del oscilador APU (o índice de ruido)
    int8_t         pitch_bend_step;     ///< paso de pitch bend ($Dx)
    uint8_t        pitch_bend_acc;      ///< acumulador de pitch bend
    float          phase;               ///< acumulador de fase normalizado [0.0, 1.0)
} MjkSoundChannel;

typedef struct MjkApuState {
    // 8 Canales virtuales canónicos (music-engine.asm:793-811):
    // 0..3: BGM (Pulse 1, Pulse 2, Triangle, Noise)
    // 4..7: SFX (Pulse 1, Pulse 2, Triangle, Noise)
    MjkSoundChannel channels[8];

    // Variables de estado APU expuestas para compatibilidad
    // Canal Pulse 1 (Hardware Voice 0)
    bool     p1_enabled;
    uint16_t p1_timer;
    uint16_t p1_period;
    uint8_t  p1_duty;        // 0=12.5%, 1=25%, 2=50%, 3=75%
    uint8_t  p1_volume;      // 0..15
    uint8_t  p1_step;

    // Canal Pulse 2 (Hardware Voice 1)
    bool     p2_enabled;
    uint16_t p2_timer;
    uint16_t p2_period;
    uint8_t  p2_duty;
    uint8_t  p2_volume;
    uint8_t  p2_step;

    // Canal Triangle (Hardware Voice 2)
    bool     tri_enabled;
    uint16_t tri_timer;
    uint16_t tri_period;
    uint8_t  tri_step;

    // Canal Noise (Hardware Voice 3)
    bool     noise_enabled;
    uint16_t noise_timer;
    uint16_t noise_period;
    uint16_t noise_lfsr;     // Registro de desplazamiento lineal de 15 bits
    uint8_t  noise_volume;

    // Cola y estado de efectos de sonido (prioridad sobre música)
    MjkSfxId active_sfx;
    uint16_t sfx_frames_left;

    // Secuenciador bytecode de música chiptune
    bool       music_enabled;
    MjkMusicId current_music;
    uint32_t   music_step_ticks;
    uint16_t   music_step_index;
    uint16_t   music_loop_length;
} MjkApuState;

typedef struct MjkApuState MjkAPUState;

/**
 * @brief Inicializa el sintetizador APU 2A03 y los 8 canales virtuales.
 */
void MjkAPU_Init(MjkApuState* apu);

/**
 * @brief Inicia la reproducción de una pista musical chiptune de fondo en canales 0..3.
 */
void MjkAPU_PlayMusic(MjkApuState* apu, MjkMusicId music);

/**
 * @brief Detiene la música de fondo y silencia los canales melódicos.
 */
void MjkAPU_StopMusic(MjkApuState* apu);

/**
 * @brief Despacha un efecto de sonido canónico ($00..$23) según SoundPlay_Dispatch ($E2C8 / HandleSoundQueue).
 */
void MjkAPU_PlaySFX(MjkApuState* apu, uint8_t sfx_id);

/**
 * @brief Dispara un efecto de sonido tipado (envoltura de MjkAPU_PlaySFX).
 */
void MjkAPU_PlaySfx(MjkApuState* apu, MjkSfxId sfx);

/**
 * @brief Actualiza la máquina de estados de bytecode y multiplexa los 8 canales a 60 Hz.
 */
void MjkAPU_UpdateFrame(MjkApuState* apu);

/**
 * @brief Sintetiza 735 muestras de audio PCM de 16 bits para el frame actual.
 */
void MjkAPU_GenerateFrameSamples(MjkApuState* apu, int16_t* out_samples, uint32_t sample_count);

// =========================================================================
// API pública snake_case (ADR-010 / Compatibilidad C99/C11)
// =========================================================================
static inline void mjk_apu_init(MjkApuState* apu) {
    MjkAPU_Init(apu);
}

static inline void mjk_apu_play_music(MjkApuState* apu, MjkMusicId music) {
    MjkAPU_PlayMusic(apu, music);
}

static inline void mjk_apu_stop_music(MjkApuState* apu) {
    MjkAPU_StopMusic(apu);
}

static inline void mjk_apu_play_sound(MjkApuState* apu, MjkSfxId sfx) {
    MjkAPU_PlaySfx(apu, sfx);
}

static inline void mjk_apu_play_sfx(MjkApuState* apu, MjkSfxId sfx) {
    MjkAPU_PlaySfx(apu, sfx);
}

static inline void mjk_apu_update_frame(MjkApuState* apu) {
    MjkAPU_UpdateFrame(apu);
}

static inline void mjk_apu_generate_samples(MjkApuState* apu, int16_t* out_samples, uint32_t sample_count) {
    MjkAPU_GenerateFrameSamples(apu, out_samples, sample_count);
}

#endif // MJK_APU_H
