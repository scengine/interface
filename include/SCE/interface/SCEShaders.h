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
 
/* created: 06/03/2007
   updated: 12/01/2011 */

#ifndef SCESHADERS_H
#define SCESHADERS_H

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCESceneResource.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCE_UNKNOWN_SHADER 0
#define SCE_SHADER_BAD_INDEX -1

/* :-' */
typedef void (*SCE_FShaderSetParamfv)(int, size_t, float*);
typedef void (*SCE_FShaderSetMatrix)(int, SCE_TMatrix4);

typedef struct sce_sshaderparam SCE_SShaderParam;
struct sce_sshaderparam {
    void *param;
    int index;
    int size;
    SCE_FShaderSetParamfv setfv;
    SCE_FShaderSetMatrix setm;
    SCE_SListIterator it;
};

typedef struct sce_sshader SCE_SShader;
struct sce_sshader {
    SCE_RShaderGLSL *v, *p;      /* vertex/pixel shader */
    SCE_RProgram *p_glsl;        /* program GLSL */

    char **res[2];               /* ressources */
    char *vs_source, *ps_source; /* sources principales */
    char *vs_addsrc, *ps_addsrc; /* code source additif (defines, ...) */
    int ready;                   /* true; le shader peut etre utilise */

    /* parametres a envoyer a chaque utilisation du shader (automatiques)
       (pointeurs dont les donnees pointees sont susceptibles de changer) */
    SCE_SList params_i;          /* entiers */
    SCE_SList params_f;          /* flottants */
    SCE_SList params_m;          /* matrices */

    SCE_SSceneResource s_resource;
};


int SCE_Init_Shader (void);
void SCE_Quit_Shader (void);

int SCE_Shader_GetResourceType (void);

void SCE_Shader_Init (SCE_SShader*);
SCE_SShader* SCE_Shader_Create (void);
void SCE_Shader_Delete (SCE_SShader*);

SCE_SSceneResource* SCE_Shader_GetSceneResource (SCE_SShader*);
int SCE_Shader_GetType (SCE_SShader*);

/* charge (char**) un fichier contenant le code du vertex shader
   et/ou le code du pixel shader (prefixes : [Vertex Shader] [Pixel Shader]) */
void* SCE_Shader_LoadSourceFromFile (FILE*, const char*, void*);

/* cree un shader a partir de deux fichiers, le premier contient le code du
   vertex shader, le second le code du pixel shader. un des deux fichiers peut
   contenir les deux types de code ensemble */
/*SCE_SShader* SCE_Shader_CreateFromFile (const char*, const char*);*/
SCE_SShader* SCE_Shader_Load (const char*, const char*, int);

/* construit un shader */
int SCE_Shader_Build (SCE_SShader*);

/* ajoute un morceau de code source qui sera ajoute
   au debut du code source par defaut */
int SCE_Shader_AddSource (SCE_SShader*, int, const char*);


/* retourne l'index d'une variable de shader */
int SCE_Shader_GetIndex (SCE_SShader*, const char*);
/* retourne l'index d'une variable d'attribut (fonctionne que pour GLSL) */
int SCE_Shader_GetAttribIndex (SCE_SShader*, const char*);
/*
int SCE_Shader_GetParam(SCE_SShader*, const char*);
float SCE_Shader_GetParamf(SCE_SShader*, const char*);
float* SCE_Shader_GetParamfv(SCE_SShader*, const char*);
*/

void SCE_Shader_Param (const char*, int);
void SCE_Shader_Paramf (const char*, float);
#define SCE_Shader_Paramfv(a, b, c, d) SCE_Shader_Param1fv(a, b, c, d)
void SCE_Shader_Param1fv (const char*, size_t, float*);
void SCE_Shader_Param2fv (const char*, size_t, float*);
void SCE_Shader_Param3fv (const char*, size_t, float*);
void SCE_Shader_Param4fv (const char*, size_t, float*);

void SCE_Shader_SetParam (int, int);
void SCE_Shader_SetParamf (int, float);
#define SCE_Shader_SetParamfv(a, b, d) SCE_Shader_SetParam1fv(a, b, d)
void SCE_Shader_SetParam1fv (int, size_t, float*);
void SCE_Shader_SetParam2fv (int, size_t, float*);
void SCE_Shader_SetParam3fv (int, size_t, float*);
void SCE_Shader_SetParam4fv (int, size_t, float*);
void SCE_Shader_SetMatrix3 (int, SCE_TMatrix3);
void SCE_Shader_SetMatrix4 (int, SCE_TMatrix4);

/* ajoute des parametres variables a envoyer a chaque utilisation du shader */
int SCE_Shader_AddParamv (SCE_SShader*, const char*, void*);
int SCE_Shader_AddParamfv (SCE_SShader*, const char*, int, int, void*);
int SCE_Shader_AddMatrix (SCE_SShader*, const char*, int, void*);

void SCE_Shader_Active (int);
void SCE_Shader_Enable (void);
void SCE_Shader_Disable (void);

void SCE_Shader_Use (SCE_SShader*);

void SCE_Shader_Lock (void);
void SCE_Shader_Unlock (void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
