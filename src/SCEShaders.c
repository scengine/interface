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

#include <ctype.h>
#include <SCE/utils/SCEUtils.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCEShaders.h"

static int is_init = SCE_FALSE;

static int resource_source_type = 0;
static int resource_shader_type = 0;

static int sce_shd_enabled = SCE_FALSE;

static SCE_SShader *used = NULL;

static void* SCE_Shader_LoadResource (const char*, int, void*);

int SCE_Init_Shader (void)
{
    if (is_init)
        return SCE_OK;

    /* register source loader */
    resource_source_type = SCE_Resource_RegisterType (SCE_TRUE, NULL, NULL);
    if (resource_source_type < 0)
        goto fail;
    /* TODO: may fail */
    SCE_Media_Register (resource_source_type, ".glsl .vert .frag",
                        SCE_Shader_LoadSourceFromFile, NULL);
    /* register shader loader */
    resource_shader_type = SCE_Resource_RegisterType (
        SCE_FALSE, SCE_Shader_LoadResource, NULL);
    if (resource_shader_type < 0)
        goto fail;

    is_init = SCE_TRUE;
    sce_shd_enabled = SCE_TRUE;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    SCEE_LogSrcMsg ("failed to initialize shaders manager");
    return SCE_ERROR;
}

void SCE_Quit_Shader (void)
{
    sce_shd_enabled = SCE_FALSE;
    is_init = SCE_FALSE;
    used = NULL;
}

int SCE_Shader_GetResourceType (void)
{
    return resource_shader_type;
}


static void SCE_Shader_InitParam (SCE_SShaderParam *sp)
{
    sp->param = NULL;
    sp->index = 0; /* TODO: constante nulle pour les indices
                      de parametres non-definie... */
    sp->size = 0;
    sp->setfv = NULL;
    sp->setm = NULL;
    SCE_List_InitIt (&sp->it);
    SCE_List_SetData (&sp->it, sp);
}
static void SCE_Shader_DeleteParam (void *p)
{
    SCE_free (p);
}

void SCE_Shader_Init (SCE_SShader *shader)
{
    shader->v = shader->p = NULL;
    shader->p_glsl = NULL;

    shader->ready = SCE_FALSE;
    shader->res[0] = shader->res[1] = NULL;
    shader->vs_source = shader->ps_source = NULL;
    shader->vs_addsrc = shader->ps_addsrc = NULL;

    SCE_List_Init (&shader->params_i);
    SCE_List_Init (&shader->params_f);
    SCE_List_Init (&shader->params_m);
    SCE_List_SetFreeFunc (&shader->params_i, SCE_Shader_DeleteParam);
    SCE_List_SetFreeFunc (&shader->params_f, SCE_Shader_DeleteParam);
    SCE_List_SetFreeFunc (&shader->params_m, SCE_Shader_DeleteParam);

    SCE_SceneResource_Init (&shader->s_resource);
    SCE_SceneResource_SetResource (&shader->s_resource, shader);
}

SCE_SShader* SCE_Shader_Create (void)
{
    SCE_SShader *shader = NULL;
    shader = SCE_malloc (sizeof *shader);
    if (!shader)
        SCEE_LogSrc ();
    else
        SCE_Shader_Init (shader);
    return shader;
}


void SCE_Shader_Delete (SCE_SShader *shader)
{
    if (shader) {
        if (!SCE_Resource_Free (shader))
            return;
        SCE_SceneResource_RemoveResource (&shader->s_resource);
        SCE_RDeleteProgram (shader->p_glsl);
        SCE_RDeleteShaderGLSL (shader->v);
        SCE_RDeleteShaderGLSL (shader->p);

        SCE_free (shader->vs_addsrc);
        SCE_free (shader->ps_addsrc);

        if (SCE_Resource_Free (shader->res[0])) {
            SCE_free (shader->res[0][0]);
            SCE_free (shader->res[0][1]);
            SCE_free (shader->res[0]);
        }
        if (SCE_Resource_Free (shader->res[1])) {
            SCE_free (shader->res[1][0]);
            SCE_free (shader->res[1][1]);
            SCE_free (shader->res[1]);
        }

        SCE_List_Clear (&shader->params_m);
        SCE_List_Clear (&shader->params_f);
        SCE_List_Clear (&shader->params_i);
        SCE_free (shader);
    }
}


SCE_SSceneResource* SCE_Shader_GetSceneResource (SCE_SShader *shader)
{
    return &shader->s_resource;
}

/* retourne le type d'un shader d'apres son extension */
static int SCE_Shader_SearchTypes (const char *ext)
{
#define SCE_SHADER_FOR(str, t)                     \
    if (SCE_String_Cmp (str, ext, SCE_FALSE) == 0) \
        return t;                                  \
    else

    SCE_SHADER_FOR (".glsl", SCE_UNKNOWN_SHADER)
    SCE_SHADER_FOR (".vert", SCE_VERTEX_SHADER)
    SCE_SHADER_FOR (".frag", SCE_PIXEL_SHADER) {
        SCEE_Log (SCE_INVALID_ARG);
        SCEE_LogMsg ("'%s' is not a valid shader source extension", ext);
        return SCE_UNKNOWN_SHADER;
    }
#undef SCE_SHADER_FOR
}


/* NOTE: a deplacer */
/* positionne le curseur de fp sur la prochaine occurrence a str
   retourne 1 si la fonction a trouve une occurrence */
static int SCE_Shader_SetPosFile (FILE *fp, const char *str, int at_end)
{
    size_t i = 0;
    long cur = 0, curo = 0;
    char buf[256] = {0};
    size_t lenstr = 0;

    lenstr = strlen (str);
    curo = ftell (fp);

    while (1) {
        fseek (fp, curo+cur, SEEK_SET);
        cur++;

        for (i = 0; i < lenstr; i++) {
            buf[i] = fgetc (fp);
            if (buf[i] == EOF) { /* fin du fichier */
                fseek (fp, curo, SEEK_SET);
                return 0;
            }
        }

        if (SCE_String_Cmp (str, buf, SCE_FALSE) == 0) {
            if (!at_end)
                fseek (fp, -lenstr, SEEK_CUR);
            return 1;
        }
    }

    fseek (fp, curo, SEEK_SET);

    return 0;
}

/* charge du texte a partir d'un fichier, jusqu'a end */
static char* SCE_Shader_LoadSource (FILE *fp, long end)
{
    char *src = NULL;
    long curpos;
    long len = ftell (fp);

    if (end != EOF) {
        len = end - len;
    } else { /* on calcul la distance qu'il y a jusqu'a la fin du fichier */
        curpos = len;
        fseek (fp, 0, SEEK_END);
        len = ftell (fp) - len;
        fseek (fp, curpos, SEEK_SET);
    }

    /* dans le cas ou la zone serait vide, aucun code de shader du type prefixe
       n'est present a cet endroit, mais pas la peine d'en faire un fromage */
    if (len <= 0)
        return NULL;

    src = SCE_malloc (len + 1);
    if (!src) {
        SCEE_LogSrc ();
        return NULL;
    }
    src[len] = '\0';

    fread (src, 1, len, fp);

    return src;
}

static void* SCE_Shader_LoadSources (FILE *fp, const char *fname)
{
    char **srcs = NULL;
    long vpos, ppos, vend, pend;
    int type;

    if (!(srcs = SCE_malloc (2 * sizeof *srcs))) {
        SCEE_LogSrc ();
        return NULL;
    }
    srcs[0] = srcs[1] = NULL;

    /* recherche du type du shader */
    type = SCE_Shader_SearchTypes (SCE_String_GetExt ((char*)fname));
    if (type == SCE_UNKNOWN_SHADER) {
        /* le fichier contient le code du vertex et du pixel shader */
        /* recuperation de la position du code du vertex shader */
        SCE_Shader_SetPosFile (fp, "[vertex shader]", SCE_FALSE);
        vpos = ftell (fp);
        vend = vpos + 15;   /* 16 = strlen("[vertex shader]"); */
        rewind (fp);

        /* recuperation de la position du code du pixel shader */
        SCE_Shader_SetPosFile (fp, "[pixel shader]", SCE_FALSE);
        ppos = ftell (fp);
        pend = ppos + 14;   /* 15 = strlen("[pixel shader]"); */

        /* lecture du code du vertex shader */
        fseek (fp, vend, SEEK_SET);
        srcs[0] = SCE_Shader_LoadSource (fp, (vpos > ppos) ? EOF : ppos);

        /* lecture du code du pixel shader */
        fseek (fp, pend, SEEK_SET);
        srcs[1] = SCE_Shader_LoadSource (fp, (ppos > vpos) ? EOF : vpos);
    } else {
        /* le fichier contient le code de type[1] */
        srcs[0] = SCE_Shader_LoadSource (fp, EOF);

        if (type == SCE_PIXEL_SHADER) {
            srcs[1] = srcs[0];
            srcs[0] = NULL;
        }
    }
    return srcs;
}

/* TODO: w00t */
#define SCE_SHADER_INCLUDE "#include"

void* SCE_Shader_LoadSourceFromFile (FILE *fp, const char *fname, void *uusd)
{
    int i, j;
    char buf[BUFSIZ] = {0};
    char *ptr = NULL;
    char **srcs = NULL;

    srcs = SCE_Shader_LoadSources (fp, fname);
    if (!srcs) {
        SCEE_LogSrc ();
        return NULL;
    }

    for (j = 0; j < 2; j++)
        if (srcs[j] && (ptr = strstr (srcs[j], SCE_SHADER_INCLUDE)))
        {
            size_t len, diff = ptr - srcs[j];
            char *tsrc = NULL;
            char **isrcs = NULL;
            /* include trouve dans le vertex shader */
            /* lecture du nom du fichier (forme #include <filename>) */
            memset (buf, '\0', BUFSIZ);
            i = 0;
            while (*ptr++ != '<');
            while (*ptr != '>')
                buf[i++] = *ptr++;
            /* lecture des fichiers inclus de maniere recursive */
            /* TODO: include path of fname into buf */
            isrcs = SCE_Resource_Load (resource_source_type, buf, SCE_FALSE,
                                       NULL);
            if (!isrcs) {
                SCEE_LogSrc ();
                return NULL;
            }
            /* si le fichier contient des donnees du meme type */
            if (isrcs[j]) {
                len = strlen (srcs[j]) + strlen (isrcs[j]) + 1;
                tsrc = SCE_malloc (len);
                memset (tsrc, '\0', len);
                strncpy (tsrc, srcs[j], diff);
                strcat (tsrc, isrcs[j]);
                strcat (tsrc, ptr);
                SCE_free (srcs[j]);
                srcs[j] = tsrc;
            }
        }

    return srcs;
}

static void* SCE_Shader_LoadResource (const char *name, int force, void *data)
{
    SCE_SShader *shader = NULL;
    char **srcs1 = NULL, **srcs2 = NULL;
    char *vsource = NULL, *psource = NULL;
    int ttemp = SCE_UNKNOWN_SHADER;
    char vnamebuf[256] = {0};   /* TODO: fixed size here */
    char *vname = NULL, *pname = NULL, *ptr = NULL, *ptr2 = NULL;
    unsigned int i;

    strcpy (vnamebuf, name);
    ptr = strchr (vnamebuf, '/');
    ptr2 = strchr (&ptr[1], '/');
    if (ptr == vnamebuf) {      /* no vertex shader */
        pname = &vnamebuf[1];
    } else {
        vname = vnamebuf;
        if (ptr2 != NULL)       /* both */
            pname = &ptr[1];
        else                    /* no pixel shader */
            vname = vnamebuf;
    }
    *ptr = 0;
    if (ptr2)
        *ptr2 = 0;

    if (force > 0)
        force--;

    if (vname) {
        srcs1 = SCE_Resource_Load (resource_source_type, vname, force, NULL);
        if (!srcs1) {
            SCEE_LogSrc ();
            return NULL;
        }

        /* on vient de charger un fichier contenant les deux codes */
        if (srcs1[0] && srcs1[1])
            pname = NULL;
        vsource = srcs1[0];
        psource = srcs1[1];
    }

    if (pname) {
        srcs2 = SCE_Resource_Load (resource_source_type, pname, force, NULL);
        if (!srcs2) {
            SCEE_LogSrc ();
            return NULL;
        }
        /* on vient de charger un fichier contenant les deux codes */
        if (srcs2[0] && srcs2[1])
            vsource = srcs2[0];
        psource = srcs2[1];
    }

    shader = SCE_Shader_Create ();
    if (!shader) {
        if (SCE_Resource_Free (srcs1)) {
            SCE_free (srcs1[0]);
            SCE_free (srcs1[1]);
            SCE_free (srcs1);
        }
        if (SCE_Resource_Free (srcs2)) {
            SCE_free (srcs2[0]);
            SCE_free (srcs2[1]);
            SCE_free (srcs2);
        }
        SCEE_LogSrc ();
        return NULL;
    }

    shader->res[0] = srcs1;
    shader->res[1] = srcs2;
    shader->vs_source = vsource;
    shader->ps_source = psource;

    return shader;
}


SCE_SShader* SCE_Shader_Load (const char *vname, const char *pname, int force)
{
    char buf[512] = {0};        /* TODO: fixed size here */
    if (vname)
        strcpy (buf, vname);
    /* char '/' used to separate names */
    if (pname) {
        strcat (buf, "/");
        strcat (buf, pname);
    }
    strcat (buf, "/resource");
    return SCE_Resource_Load (resource_shader_type, buf, force, NULL);
}


static int SCE_Shader_BuildGLSL (SCE_SShader *shader)
{
    if (shader->vs_source) {
        shader->v = SCE_RCreateShaderGLSL (SCE_VERTEX_SHADER);
        if (!shader->v) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
        SCE_RSetShaderGLSLSource (shader->v, shader->vs_source);
        if (SCE_RBuildShaderGLSL (shader->v) < 0) {
            SCE_RDeleteShaderGLSL (shader->v);
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
    }

    if (shader->ps_source) {
        shader->p = SCE_RCreateShaderGLSL (SCE_PIXEL_SHADER);
        if (!shader->p) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
        SCE_RSetShaderGLSLSource (shader->p, shader->ps_source);
        if (SCE_RBuildShaderGLSL (shader->p) < 0) {
            SCE_RDeleteShaderGLSL (shader->p);
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
    }

    shader->p_glsl = SCE_RCreateProgram ();
    if (!shader->p_glsl) {
        SCE_RDeleteShaderGLSL (shader->v);
        SCE_RDeleteShaderGLSL (shader->p);
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    if (shader->v)
        SCE_RSetProgramShader (shader->p_glsl, shader->v, 1);
    if (shader->p)
        SCE_RSetProgramShader (shader->p_glsl, shader->p, 1);
    if (SCE_RBuildProgram (shader->p_glsl) < 0) {
        SCE_RDeleteProgram (shader->p_glsl);
        SCE_RDeleteShaderGLSL (shader->v);
        SCE_RDeleteShaderGLSL (shader->p);
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}

int SCE_Shader_Build (SCE_SShader *shader)
{
    if (shader->vs_source || shader->vs_addsrc) {
        shader->vs_source = SCE_String_CatDup (shader->vs_addsrc, shader->vs_source);
        if (!shader->vs_source) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
    }
    if (shader->ps_source || shader->ps_addsrc) {
        shader->ps_source = SCE_String_CatDup (shader->ps_addsrc, shader->ps_source);
        if (!shader->ps_source) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
    }

    if (SCE_Shader_BuildGLSL (shader) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }

    shader->ready = SCE_TRUE;
    return SCE_OK;
}


int SCE_Shader_AddSource (SCE_SShader *shader, int type, const char *src)
{
    size_t realen;
    int first_alloc = 1;
    char *addsrc = NULL;

    if (!src)                   /* lol? */
        return SCE_OK;

    addsrc =
    (type == SCE_PIXEL_SHADER) ? shader->ps_addsrc : shader->vs_addsrc;

    /* + 2 car, 1: retour chariot. 2: caractere de fin de chaine */
    realen = strlen(src) + 2;

    if (addsrc) {
        first_alloc = 0;
        realen += strlen (addsrc);
    }

    addsrc = SCE_realloc (addsrc, realen);
    if (!addsrc) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    /* s'il s'agit de la premiere allocation, on initialise le contenu */
    if (first_alloc)
        memset (addsrc, '\0', realen);

    strcat (addsrc, src);

    /* affectation */
    if (type == SCE_PIXEL_SHADER)
        shader->ps_addsrc = addsrc;
    else
        shader->vs_addsrc = addsrc;

    return SCE_OK;
}


int SCE_Shader_GetIndex (SCE_SShader *shader, const char *name)
{
    return SCE_RGetProgramIndex (shader->p_glsl, name);
}

int SCE_Shader_GetAttribIndex (SCE_SShader *shader, const char *name)
{
    return SCE_RGetProgramAttribIndex (shader->p_glsl, name);
}

void SCE_Shader_Param (const char *name, int value)
{
    SCE_RSetProgramParam (SCE_RGetProgramIndex (used->p_glsl, name), value);
}

void SCE_Shader_Paramf (const char *name, float value)
{
    SCE_RSetProgramParamf (SCE_RGetProgramIndex (used->p_glsl, name), value);
}


#define SCE_GLSL_PARAM_FUNC(n)\
void SCE_Shader_Param##n##fv (const char *name, size_t size, float *value)\
{\
    SCE_RSetProgramParam##n##fv (SCE_RGetProgramIndex (used->p_glsl, name), size, value);\
}
SCE_GLSL_PARAM_FUNC(1)
SCE_GLSL_PARAM_FUNC(2)
SCE_GLSL_PARAM_FUNC(3)
SCE_GLSL_PARAM_FUNC(4)
#undef SCE_GLSL_PARAM_FUNC

void SCE_Shader_SetParam (int index, int value)
{
    SCE_RSetProgramParam (index, value);
}

void SCE_Shader_SetParamf (int index, float value)
{
    SCE_RSetProgramParamf (index, value);
}

#define SCE_GLSL_SETPARAM_FUNC(n)                                       \
    void SCE_Shader_SetParam##n##fv (int index, size_t size, float *value) \
    {                                                                   \
        SCE_RSetProgramParam##n##fv (index, size, value);               \
    }
SCE_GLSL_SETPARAM_FUNC(1)
SCE_GLSL_SETPARAM_FUNC(2)
SCE_GLSL_SETPARAM_FUNC(3)
SCE_GLSL_SETPARAM_FUNC(4)
#undef SCE_GLSL_SETPARAM_FUNC

void SCE_Shader_SetMatrix3 (int index, SCE_TMatrix4 m)
{
    SCE_RSetProgramMatrix3 (index, 1, m); /* count forced */
}
void SCE_Shader_SetMatrix4 (int index, SCE_TMatrix4 m)
{
    SCE_RSetProgramMatrix4 (index, 1, m); /* count forced */
}

/**
 * \brief Adds a constant parameter
 * \param name the name of the parameter in the shader source code
 * \param p pointer to the values that will be send
 *
 * Adds a parameter that will be send on each call of SCE_Shader_Use(\p shader).
 *
 * \sa SCE_Shader_AddParamfv()
 */
int SCE_Shader_AddParamv (SCE_SShader *shader, const char *name, void *p)
{
    SCE_SShaderParam *param = NULL;
    if (!(param = SCE_malloc (sizeof *param))) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    SCE_Shader_InitParam (param);
    SCE_List_Appendl (&shader->params_i, &param->it);
    param->param = p;
    /* necessite que le shader ait deja ete construit */
    param->index = SCE_Shader_GetIndex (shader, name);
    return SCE_OK;
}
/**
 * \brief Adds a constant floating parameter
 * \param name the name of the parameter in the shader source code
 * \param num vector's length, can be 1, 2, 3 or 4
 * \param size number of vectors in \p p (only if is an array, set it as 1
 * otherwise)
 * \param p pointer to the values that will be send
 *
 * Adds a floating parameter that will be send on each call of
 * SCE_Shader_Use(\p shader).
 *
 * \sa SCE_Shader_AddParamv()
 */
int SCE_Shader_AddParamfv (SCE_SShader *shader, const char *name,
                           int num, int size, void *p)
{
    SCE_SShaderParam *param = NULL;
    if (!(param = SCE_malloc (sizeof *param))) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    SCE_Shader_InitParam (param);
    SCE_List_Appendl (&shader->params_f, &param->it);
    param->param = p;
    switch (num) {
#define SCE_SHDCASE(n) case n: param->setfv = SCE_Shader_SetParam##n##fv; break;
    SCE_SHDCASE (1)
    SCE_SHDCASE (2)
    SCE_SHDCASE (3)
    SCE_SHDCASE (4)
#undef SCE_SHDCASE
    }
    param->size = size;
    /* necessite que le shader ait deja ete construit */
    param->index = SCE_Shader_GetIndex (shader, name);
    /* NOTE: et la on ne verifie pas index... ? bof ca fait chier */
    return SCE_OK;
}
/**
 * \brief Adds a constant matrix
 * \param name the name of the matrix in the shader source code
 * \param p pointer to the matrix
 *
 * Adds a matrix that will be send on each call of SCE_Shader_Use(\p shader).
 *
 * \sa SCE_Shader_AddParamfv()
 */
int SCE_Shader_AddMatrix (SCE_SShader *shader, const char *name,
                          int size, void *p)
{
    SCE_SShaderParam *param = NULL;
    if (!(param = SCE_malloc (sizeof *param))) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    SCE_Shader_InitParam (param);
    SCE_List_Appendl (&shader->params_m, &param->it);
    param->param = p;
    /* necessite que le shader ait deja ete construit */
    param->index = SCE_Shader_GetIndex (shader, name);
    if (size == 3)
        param->setm = SCE_Shader_SetMatrix3;
    else
        param->setm = SCE_Shader_SetMatrix4;
    return SCE_OK;
}

void SCE_Shader_Active (int enabled)
{
    sce_shd_enabled = enabled;
}
void SCE_Shader_Enable (void)
{
    sce_shd_enabled = SCE_TRUE;
}
void SCE_Shader_Disable (void)
{
    sce_shd_enabled = SCE_FALSE;
}

static void SCE_Shader_SetParams (SCE_SShader *shader)
{
    SCE_SListIterator *i = NULL;
    SCE_SShaderParam *p = NULL;
    SCE_List_ForEach (i, &shader->params_i) {
        p = SCE_List_GetData (i);
        SCE_Shader_SetParam (p->index, *((int*)p->param));
    }
    SCE_List_ForEach (i, &shader->params_f) {
        p = SCE_List_GetData (i);
        p->setfv (p->index, p->size, p->param);
    }
    SCE_List_ForEach (i, &shader->params_m) {
        p = SCE_List_GetData (i);
        p->setm (p->index, p->param);
    }
}

static void SCE_Shader_UseGLSL (SCE_SShader *shader)
{
    if (!sce_shd_enabled)
        return;
    else if (shader)
        SCE_RUseProgram (shader->p_glsl);
    else
        SCE_RUseProgram (NULL);
}
void SCE_Shader_Use (SCE_SShader *shader)
{
    if (!shader) {
        if (used) {
            SCE_Shader_UseGLSL (NULL);
            used = NULL;
        }
    } else if (shader == used)
        SCE_Shader_SetParams (shader);
    else {
        SCE_Shader_UseGLSL (shader);
        used = shader;        /* before setparams() call */
        SCE_Shader_SetParams (shader);
    }
}
