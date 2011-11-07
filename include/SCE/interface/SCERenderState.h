/*------------------------------------------------------------------------------
    SCEngine - A 3D real time rendering engine written in the C language
    Copyright (C) 2006-2011  Antony Martin <martin(dot)antony(at)yahoo(dot)fr>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 -----------------------------------------------------------------------------*/

/* created: 05/11/2011
   updated: 06/11/2011 */

#ifndef SCERENDERSTATE_H
#define SCERENDERSTATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* boolean states: things can be enabled/disabled */
typedef struct sce_srenderstate SCE_SRenderState;
struct sce_srenderstate {
    char **names;
    size_t n_states;
    SCEuint state;
};

void SCE_RenderState_Init (SCE_SRenderState*);
void SCE_RenderState_Clear (SCE_SRenderState*);
#if 0
SCE_SRenderState* SCE_RenderState_Create (void);
void SCE_RenderState_Delete (SCE_SRenderState*);
#endif

int SCE_RenderState_SetStates (SCE_SRenderState*, const char**, size_t);
const char* SCE_RenderState_GetStateName (const SCE_SRenderState*, size_t);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
