/**
 * @file mjk_enemy_internal.h
 * @brief Helpers internos compartidos entre mjk_enemy.c y mjk_enemy_collectibles.c.
 */

#ifndef MJK_ENEMY_INTERNAL_H
#define MJK_ENEMY_INTERNAL_H

#include "mjk_enemy.h"

struct MjkGame;

bool MjkEnemyInternal_IsMonsterType(uint8_t t);
mjk_q8_8_t MjkEnemyInternal_SpeedQ8(const MjkAssetPackage* pkg, uint8_t speed_tier);
uint8_t MjkEnemyInternal_ArchetypeSpriteId(uint8_t t, uint8_t anim, bool facing_left);
uint8_t MjkEnemyInternal_ArchetypePalette(uint8_t t);
uint8_t MjkEnemyInternal_CalcCollisionFlags(const struct MjkGame* game, uint8_t ex, uint8_t ey);
void MjkEnemyInternal_InitMummyFall(MjkEnemy* e, uint8_t transform_reload);

bool MjkEnemyInternal_StepCollectible(MjkEnemy* e, const struct MjkGame* game);
void MjkEnemyInternal_HandleTransmutedPickup(MjkEnemy* e, MjkEnemyManager* mgr,
                                             struct MjkGame* game, uint8_t ex, uint8_t ey);
void MjkEnemyInternal_HandleCollectiblePickup(MjkEnemy* e, MjkEnemyManager* mgr,
                                              struct MjkGame* game, uint8_t ex, uint8_t ey);

#endif /* MJK_ENEMY_INTERNAL_H */
