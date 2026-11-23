/**
 * @file mjk_title.c
 * @brief Implementación de TitleScreenHandler REV_US (asm/src/prg.asm:7473-7660).
 */

#include "mjk_title.h"
#include "mjk_room_decoder.h"
#include <string.h>

void MjkTitle_Init(MjkTitleSystem* ts) {
    if (!ts) {
        return;
    }
    memset(ts, 0, sizeof(*ts));
    ts->state = MJK_TS_US_COPYRIGHT_INIT;
}

static uint8_t draw_title_sprites(const MjkTitleSystem* ts,
                                  const MjkAssetPackage* pkg,
                                  MjkPpu* ppu,
                                  const uint8_t score_bcd[4],
                                  uint8_t high_gdv_bcd) {
    MjkPpu_ClearOam(ppu);
    uint8_t slot = 0u;
    slot = MjkPpu_DrawHudScore(ppu, score_bcd, slot);
    slot = MjkPpu_DrawBcdByte(ppu, high_gdv_bcd, 0xCCu, 0x1Cu, false, slot);
    slot = MjkPpu_DrawBcdByte(ppu, 0x19u, 0xBCu, 0x93u, false, slot);
    slot = MjkPpu_DrawBcdByte(ppu, 0x87u, 0xCCu, 0x93u, false, slot);
    slot = MjkPpu_WriteMetasprite(ppu, pkg, ts->jack_sprite,
                                  MjkUQ8_Int(ts->jack_x),
                                  MjkUQ8_Int(ts->jack_y),
                                  0x05u, slot);
    return slot;
}

bool MjkTitle_Step(MjkTitleSystem* ts,
                   const MjkInputState* input,
                   const MjkAssetPackage* pkg,
                   MjkPpu* ppu,
                   const uint8_t score_bcd[4],
                   uint8_t high_gdv_bcd) {
    if (!ts || !pkg || !ppu) {
        return false;
    }
    const uint8_t held = input ? input->held : 0u;
    const uint8_t pressed = input ? input->just_pressed : 0u;
    static const uint8_t k_zero_score[4] = { 0u, 0u, 0u, 0u };
    const uint8_t* sc = score_bcd ? score_bcd : k_zero_score;

    switch (ts->state) {
        case MJK_TS_US_COPYRIGHT_INIT: {
            MjkPpu_RestoreDefaultPalettes(ppu, pkg);
            MjkPpu_ClearNametable(ppu, 0x24u);
            MjkPpu_ClearOam(ppu);
            ppu->scroll_x = 0u;
            ppu->scroll_y = 0u;

            MjkPpu_WriteString(ppu, pkg, MJK_STR_MIGHTY_BOMB_JACK_TM);
            MjkPpu_WriteString(ppu, pkg, MJK_STR_TM_AND_C_TECMO_LTD);
            MjkPpu_WriteString(ppu, pkg, MJK_STR_LICENSED_BY);
            MjkPpu_WriteString(ppu, pkg, MJK_STR_NINTENDO_OF_AMERICA);

            uint8_t slot = MjkPpu_DrawBcdByte(ppu, 0x19u, 0x74u, 0x63u, false, 0u);
            MjkPpu_DrawBcdByte(ppu, 0x87u, 0x84u, 0x63u, false, slot);

            ts->timer = 256u;
            ts->state = MJK_TS_US_COPYRIGHT_WAIT;
            break;
        }

        case MJK_TS_US_COPYRIGHT_WAIT:
            if (ts->timer > 0u) {
                ts->timer--;
            }
            if ((held & MJK_BTN_START) != 0u || ts->timer == 0u) {
                ts->state = MJK_TS_TITLE_INIT;
            }
            break;

        case MJK_TS_TITLE_INIT: {
            MjkPpu_RestoreDefaultPalettes(ppu, pkg);
            MjkPpu_ClearNametable(ppu, 0x24u);
            ppu->scroll_x = 0u;
            ppu->scroll_y = 0u;

            MjkRoomState room0;
            if (MjkRoom_DecodeGeometry(pkg, 0x00u, &room0)) {
                MjkRoom_BlitToPpu(&room0, pkg, ppu, 0u);
            }
            MjkPpu_WriteString(ppu, pkg, MJK_STR_PUSH_START_BUTTON);
            MjkPpu_WriteString(ppu, pkg, MJK_STR_C_TECMO);
            MjkPpu_WriteString(ppu, pkg, MJK_STR_HIGH_GDV);
            MjkPpu_WriteString(ppu, pkg, MJK_STR_TM);

            ts->jack_x = MjkUQ8_FromInt(0x28u); /* 40 */
            ts->jack_y = MjkUQ8_FromInt(0x78u); /* 120 */
            ts->jack_vx = 0;
            ts->jack_vy = 0;
            ts->jack_sprite = 0u;
            ts->timer = 512u;
            draw_title_sprites(ts, pkg, ppu, sc, high_gdv_bcd);
            ts->state = MJK_TS_TITLE_WAIT;
            break;
        }

        case MJK_TS_TITLE_WAIT:
            if ((pressed & MJK_BTN_START) != 0u) {
                ts->jack_vx = (mjk_q8_8_t)0x0080;
                ts->jack_sprite = 3u;
                ts->grav_index = 0x4000u;
                ts->state = MJK_TS_JACK_FALLING;
                break;
            }
            if (ts->timer > 0u) {
                ts->timer--;
            }
            if (ts->timer == 0u) {
                ts->state = MJK_TS_US_COPYRIGHT_INIT;
            }
            break;

        case MJK_TS_JACK_FALLING: {
            uint16_t rate = 0x00A0u;
            MjkAssets_ReadCpuU16(pkg, (uint16_t)(MJK_ADDR_VEL_X_TABLE_C163 + 4u), &rate);
            uint32_t next_acc = (uint32_t)ts->grav_index + (uint32_t)rate;
            if ((next_acc >> 8u) > 0x7Fu) {
                next_acc = (0x7Fu << 8u) | (next_acc & 0xFFu);
            }
            ts->grav_index = (uint16_t)next_acc;

            uint8_t step = (uint8_t)(ts->grav_index >> 8u);
            uint8_t tbl_idx = (step < 64u) ? step : (uint8_t)((step ^ 0xFFu) & 0x3Fu);
            uint16_t mag = 0u;
            MjkAssets_ReadCpuU16(pkg, (uint16_t)(MJK_ADDR_JUMP_TABLE_C0E3 + (uint16_t)tbl_idx * 2u), &mag);
            ts->jack_vy = (step < 64u) ? (mjk_q8_8_t)(-(int16_t)mag) : (mjk_q8_8_t)mag;

            ts->jack_x = MjkUQ8_AddSigned(ts->jack_x, ts->jack_vx);
            ts->jack_y = MjkUQ8_AddSigned(ts->jack_y, ts->jack_vy);

            if (MjkUQ8_Int(ts->jack_y) >= 0xD8u) {
                MjkPpu_ClearNametable(ppu, 0x24u);
                MjkPpu_ClearOam(ppu);
                ts->state = MJK_TS_US_COPYRIGHT_INIT;
                return true;
            }
            draw_title_sprites(ts, pkg, ppu, sc, high_gdv_bcd);
            break;
        }

        default:
            ts->state = MJK_TS_US_COPYRIGHT_INIT;
            break;
    }

    return false;
}
