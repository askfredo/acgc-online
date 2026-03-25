static void eEL_InitUnchar(u8* unchar_p, int count, u8 val) {
    int i;

    for (i = 0; i < count; i++) {
        *unchar_p++ = val;
    }
}

static void eEL_VectorRoteteY(xyz_t* vec, f32 angle) {
    xyz_t src = *vec;
    f32 cos = cosf_table(angle);
    f32 sin = sinf_table(angle);

    vec->x = src.x * cos + src.z * sin;
    vec->z = -src.x * sin + src.z * cos;
}

static s16 eEL_RandomFirstSpeed(xyz_t* speed, f32 y, f32 max_z, f32 max_x) {
    xyz_t vec = { 0.0f, 1.0f, 0.0f };
    f32 angle = 0.0f;

    vec.y = y;

    if (max_z != 0.0f) {
        f32 rnd_z = RANDOM_F(max_z) - max_z * 0.5f;

        angle = rnd_z;
        sMath_RotateZ(&vec, DEG2RAD(rnd_z));
    }

    if (max_x != 0.0f) {
        f32 rnd_x = RANDOM_F(max_x) - max_x * 0.5f;
        sMath_RotateX(&vec, DEG2RAD(rnd_x));
    }

    if (speed != NULL) {
        *speed = vec;
    }

    return RAD2SHORT_ANGLE2(DEG2RAD(angle));
}

static void eEL_SetContiniousEnv(eEC_Effect_c* effect, s16 unused, s16 timer) {
    if (effect->state == eEC_STATE_NORMAL) {
        if (effect->timer <= 1) {
            effect->state = eEC_STATE_CONTINUOUS;
            effect->timer = timer;
        }
    } else if (effect->state == eEC_STATE_CONTINUOUS) {
        if (effect->timer <= 1) {
            effect->state = eEC_STATE_CONTINUOUS;
            effect->timer = timer;
        }
    }
}

static f32 eEL_CalcAdjust(s16 now, s16 start, s16 end, f32 start_val, f32 end_val) {

    if (start == end) {
        return start_val;
    }

    if (now <= start) {
        return start_val;
    }

    if (now >= end) {
        return end_val;
    }

    return start_val + (f32)(now - start) * ((end_val - start_val) / (f32)(end - start));
}

static void eEL_AutoMatrixXlu(GAME* game, xyz_t* pos, xyz_t* scale) {
    GAME_PLAY* play = (GAME_PLAY*)game;
    Mtx* mtx = (Mtx*)GRAPH_ALLOC_TYPE(game->graph, Mtx, 1);
    f32 adj_scale;

    OPEN_DISP(game->graph);

    _texture_z_light_fog_prim_xlu(game->graph);
    adj_scale = 1.0f + ((f32)(int)GETREG(MYKREG, 27)) * 0.01f;
    suMtxMakeTS(mtx, scale->x * adj_scale, scale->y * adj_scale, scale->z * adj_scale, pos->x, pos->y, pos->z);

    gSPMatrix(NEXT_POLY_XLU_DISP, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPMatrix(NEXT_POLY_XLU_DISP, play->billboard_mtx_p, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);

    CLOSE_DISP(game->graph);
}

static void eEL_AutoMatrixXlu_Offset(GAME* game, xyz_t* pos, xyz_t* scale, xyz_t* offset) {
    GAME_PLAY* play = (GAME_PLAY*)game;
    f32 adj_scale;

    OPEN_DISP(game->graph);

    _texture_z_light_fog_prim_xlu(game->graph);
    Matrix_translate(pos->x, pos->y, pos->z, MTX_LOAD);
    Matrix_mult(&play->billboard_matrix, MTX_MULT);
    Matrix_translate(offset->x + (f32)(int)GETREG(MYKREG, 24), offset->y + (f32)(int)GETREG(MYKREG, 25),
                     offset->z + (f32)(int)GETREG(MYKREG, 26), MTX_MULT);
    adj_scale = 1.0f + ((f32)(int)GETREG(MYKREG, 27)) * 0.01f;
    Matrix_scale(scale->x * adj_scale, scale->y * adj_scale, scale->z * adj_scale, MTX_MULT);

    gSPMatrix(NEXT_POLY_XLU_DISP, _Matrix_to_Mtx_new(game->graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    CLOSE_DISP(game->graph);
}

static void eEC_MorphCombine(u8* const result, eEC_morph_data_c* const morph_data, s16 now_timer) {
    int i;

    for (i = 0; i < 9; i++) {
        if (morph_data[i].morph_flag) {
            result[i] = eEL_CalcAdjust(now_timer, morph_data[i].start_frame << 1, morph_data[i].end_frame << 1,
                                       morph_data[i].start_val, morph_data[i].end_val);
        } else {
            result[i] = morph_data[i].start_val;
        }
    }
}

static int eEL_SpecialBlockCenterPos(xyz_t* pos, u32 block_kind) {
    int bx;
    int bz;

    *pos = ZeroVec;

    if (mFI_BlockKind2BkNum(&bx, &bz, block_kind) && mFI_BkNum2WposXZ(&pos->x, &pos->z, bx, bz)) {
        pos->x += mFI_BK_WORLDSIZE_X_F / 2;
        pos->z += mFI_BK_WORLDSIZE_Z_F / 2;
        return TRUE;
    } else {
        return FALSE;
    }
}

static eEC_light_data_c eEL_light_data;
static const eEC_light_data_c eEC_light_default = { 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, 0, 0, 0 };

static int eEL_EffectLightStatus(rgba_t* color, int* shadow_flag) {
    color->r = eEC_ctrl_work.light_info.lights.diffuse.color[0];
    color->g = eEC_ctrl_work.light_info.lights.diffuse.color[1];
    color->b = eEC_ctrl_work.light_info.lights.diffuse.color[2];
    color->a = 0;

    *shadow_flag = eEL_light_data.shadow_flag;
    return eEL_light_data.light_state;
}

static void eEL_InitEffectLight(void) {
    eEL_light_data = eEC_light_default;
    eEL_light_data.n_frames = 10;
}

static void eEL_CalcEffectLight_Set(GAME* game) {
    int r;
    int g;
    int b;

    if (eEL_light_data.ctr > eEL_light_data.n_frames) {
        eEL_InitEffectLight();
        eEC_ctrl_work.light_info.lights.diffuse.color[0] = 0;
        eEC_ctrl_work.light_info.lights.diffuse.color[1] = 0;
        eEC_ctrl_work.light_info.lights.diffuse.color[2] = 0;
        eEL_light_data.light_state = eEC_LIGHT_COLOR_DISABLED;
    } else {
        if (eEL_light_data.ctr < eEL_light_data.max_frame) {
            r = (int)eEC_CLIP->calc_adjust_proc(eEL_light_data.ctr, 0, eEL_light_data.max_frame,
                                                eEL_light_data.start_color.r, eEL_light_data.target_color.r);
            g = (int)eEC_CLIP->calc_adjust_proc(eEL_light_data.ctr, 0, eEL_light_data.max_frame,
                                                eEL_light_data.start_color.g, eEL_light_data.target_color.g);
            b = (int)eEC_CLIP->calc_adjust_proc(eEL_light_data.ctr, 0, eEL_light_data.max_frame,
                                                eEL_light_data.start_color.b, eEL_light_data.target_color.b);

            eEL_light_data.light_state = eEC_LIGHT_COLOR_INCREASING;
        } else {
            r = (int)eEC_CLIP->calc_adjust_proc(eEL_light_data.ctr, eEL_light_data.max_frame, eEL_light_data.n_frames,
                                                eEL_light_data.target_color.r, 0.0f);
            g = (int)eEC_CLIP->calc_adjust_proc(eEL_light_data.ctr, eEL_light_data.max_frame, eEL_light_data.n_frames,
                                                eEL_light_data.target_color.g, 0.0f);
            b = (int)eEC_CLIP->calc_adjust_proc(eEL_light_data.ctr, eEL_light_data.max_frame, eEL_light_data.n_frames,
                                                eEL_light_data.target_color.b, 0.0f);

            if (eEL_light_data.ctr == eEL_light_data.max_frame) {
                eEL_light_data.light_state = eEC_LIGHT_COLOR_MAXIMUM;
            } else {
                eEL_light_data.light_state = eEC_LIGHT_COLOR_DECREASING;
            }
        }

        eEC_ctrl_work.light_info.lights.diffuse.color[0] = r;
        eEC_ctrl_work.light_info.lights.diffuse.color[1] = g;
        eEC_ctrl_work.light_info.lights.diffuse.color[2] = b;
        eEL_light_data.ctr++;
    }
}

static void eEL_RegistEffectLight(rgba_t color, s16 max_frames, s16 n_frames, int shadow_flag) {
    eEL_light_data.start_color.r = eEC_ctrl_work.light_info.lights.diffuse.color[0];
    eEL_light_data.start_color.g = eEC_ctrl_work.light_info.lights.diffuse.color[1];
    eEL_light_data.start_color.b = eEC_ctrl_work.light_info.lights.diffuse.color[2];

    eEL_light_data.ctr = 0;
    eEL_light_data.target_color = color;
    eEL_light_data.max_frame = max_frames;
    eEL_light_data.n_frames = n_frames;
    eEL_light_data.shadow_flag = shadow_flag;
}

static void eEL_DecideLightPower(rgba_t* result_color, rgba_t base_color, xyz_t pos, GAME* game, f32 max_power,
                                 f32 min_power, f32 max_dist) {
    GAME_PLAY* play = (GAME_PLAY*)game;
    PLAYER_ACTOR* player = GET_PLAYER_ACTOR(play);

    if (player != NULL) {
        xyz_t player_pos = player->actor_class.world.position;
        f32 dist = search_position_distance(&pos, &player_pos);
        f32 power;

        if (dist > max_dist) {
            power = min_power;
        } else {
            power = (max_power - min_power) * (1.0f - dist / max_dist);
            power += min_power;
        }

        result_color->r = (int)(base_color.r * power);
        result_color->g = (int)(base_color.g * power);
        result_color->b = (int)(base_color.b * power);
        result_color->a = 255;
    } else {
        *result_color = base_color;
    }
}

static int mEL_CheckLookAtBlock(xyz_t pos) {
    xyz_t* lookat = Camera2_getCenterPos_p();
    int effect_bx;
    int effect_bz;
    int lookat_bx;
    int lookat_bz;

    if (mFI_Wpos2BlockNum(&effect_bx, &effect_bz, pos) && mFI_Wpos2BlockNum(&lookat_bx, &lookat_bz, *lookat)) {
        if (effect_bx == lookat_bx && effect_bz == lookat_bz) {
            return TRUE;
        }
    }

    return FALSE;
}

#if VERSION >= VER_DELUXE

/**
 * @brief Check if an effect should be killed based on its state and timer.
 *
 * @param effect Pointer to the effect.
 * @return TRUE if the effect should die, FALSE otherwise.
 */
static int eEC_ShouldEffectDie(eEC_Effect_c* effect) {
    if (effect->state == eEC_STATE_FINISHED && effect->timer <= 0) {
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Linearly interpolate between two float values for effects.
 *
 * @param from Start value.
 * @param to End value.
 * @param t Interpolation factor (0.0 = from, 1.0 = to).
 * @return Interpolated float value.
 */
static f32 eEL_linear_interpolate_f(f32 from, f32 to, f32 t) {
    return from + (to - from) * t;
}

/**
 * @brief Linearly interpolate between two integer values for effects.
 *
 * @param from Start value.
 * @param to End value.
 * @param t Interpolation factor (0.0 = from, 1.0 = to).
 * @return Interpolated integer value.
 */
static int eEL_linear_interpolate_int(int from, int to, f32 t) {
    return from + (int)((f32)(to - from) * t);
}

/**
 * @brief Set continuous environment effect lifetime.
 *
 * Extends the lifetime of a continuous effect by resetting
 * the timer to the given value.
 *
 * @param effect Pointer to the effect.
 * @param lifetime New lifetime timer value.
 */
static void eEL_SetContinuousEnvLifetime(eEC_Effect_c* effect, s16 lifetime) {
    if (effect->state == eEC_STATE_CONTINUOUS) {
        effect->timer = lifetime;
    }
}

/**
 * @brief Calculate float adjustment between start and end values.
 *
 * Similar to eEL_CalcAdjust but returns a float and uses float
 * parameters for start/end frames.
 *
 * @param now Current frame.
 * @param start Start frame.
 * @param end End frame.
 * @param start_val Start value.
 * @param end_val End value.
 * @return Adjusted float value.
 */
static f32 eEL_CalcAdjust_Float(f32 now, f32 start, f32 end, f32 start_val, f32 end_val) {
    if (start == end) {
        return start_val;
    }

    if (now <= start) {
        return start_val;
    }

    if (now >= end) {
        return end_val;
    }

    return start_val + (now - start) * ((end_val - start_val) / (end - start));
}

/**
 * @brief Calculate integer adjustment between start and end values.
 *
 * @param now Current frame.
 * @param start Start frame.
 * @param end End frame.
 * @param start_val Start value.
 * @param end_val End value.
 * @return Adjusted integer value.
 */
static int eEL_CalcAdjust_Int(s16 now, s16 start, s16 end, int start_val, int end_val) {
    if (start == end) {
        return start_val;
    }

    if (now <= start) {
        return start_val;
    }

    if (now >= end) {
        return end_val;
    }

    return start_val + (int)((f32)(now - start) * ((f32)(end_val - start_val) / (f32)(end - start)));
}

/**
 * @brief Combine morph targets for integer color/attribute values.
 *
 * Evaluates each morph channel and writes the interpolated u8 result
 * into the result array. Supports an arbitrary number of channels.
 *
 * @param result Output array of u8 values.
 * @param morph_data Array of morph data descriptors.
 * @param now_timer Current animation timer.
 * @param count Number of morph channels to process.
 */
static void eEL_MorphCombine(u8* const result, eEC_morph_data_c* const morph_data, s16 now_timer, int count) {
    int i;

    for (i = 0; i < count; i++) {
        if (morph_data[i].morph_flag) {
            result[i] = (u8)(int)eEL_CalcAdjust_Float(
                (f32)now_timer,
                (f32)(morph_data[i].start_frame << 1),
                (f32)(morph_data[i].end_frame << 1),
                morph_data[i].start_val,
                morph_data[i].end_val);
        } else {
            result[i] = (u8)(int)morph_data[i].start_val;
        }
    }
}

/**
 * @brief Combine morph targets for float values.
 *
 * Same as eEL_MorphCombine but outputs float values instead of u8.
 *
 * @param result Output array of float values.
 * @param morph_data Array of morph data descriptors.
 * @param now_timer Current animation timer.
 * @param count Number of morph channels to process.
 */
static void eEL_MorphCombineFloat(f32* const result, eEC_morph_data_c* const morph_data, s16 now_timer, int count) {
    int i;

    for (i = 0; i < count; i++) {
        if (morph_data[i].morph_flag) {
            result[i] = eEL_CalcAdjust_Float(
                (f32)now_timer,
                (f32)(morph_data[i].start_frame << 1),
                (f32)(morph_data[i].end_frame << 1),
                morph_data[i].start_val,
                morph_data[i].end_val);
        } else {
            result[i] = morph_data[i].start_val;
        }
    }
}

/**
 * @brief Check if an effect position is within the camera's look-at block.
 *
 * Extended version of mEL_CheckLookAtBlock that also checks adjacent blocks.
 *
 * @param pos Effect world position.
 * @param range Number of adjacent blocks to include (0 = exact match only).
 * @return TRUE if within range of the look-at block.
 */
static int eEL_CheckLookAtBlock(xyz_t pos, int range) {
    xyz_t* lookat = Camera2_getCenterPos_p();
    int effect_bx;
    int effect_bz;
    int lookat_bx;
    int lookat_bz;

    if (mFI_Wpos2BlockNum(&effect_bx, &effect_bz, pos) && mFI_Wpos2BlockNum(&lookat_bx, &lookat_bz, *lookat)) {
        if (ABS(effect_bx - lookat_bx) <= range && ABS(effect_bz - lookat_bz) <= range) {
            return TRUE;
        }
    }

    return FALSE;
}

#endif /* VERSION >= VER_DELUXE */
