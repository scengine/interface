/*------------------------------------------------------------------------------
    SCEngine - A 3D real time rendering engine written in the C language
    Copyright (C) 2006-2010  Antony Martin <martin(dot)antony(at)yahoo(dot)fr>

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
   updated: 06/03/2010 */

#include <ctype.h>
#include <SCE/utils/SCEUtils.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCEShaders.h"


typedef void (*SCE_FShaderDelete)(SCE_SShader*);
#ifdef SCE_USE_CG
static void SCE_Shader_DeleteCG (SCE_SShader*);
#endif
static void SCE_Shader_DeleteGLSL (SCE_SShader*);

typedef int (*SCE_FShaderBuild)(SCE_SShader*);
#ifdef SCE_USE_CG
static int SCE_Shader_BuildCG (SCE_SShader*);
#endif
static int SCE_Shader_BuildGLSL (SCE_SShader*);

/*******/
typedef int (*SCE_FShaderGetIndex)(SCE_SShader*, int, const char*);
#ifdef SCE_USE_CG
static int SCE_Shader_GetIndexCG (SCE_SShader*, int, const char*);
#endif
static int SCE_Shader_GetIndexGLSL (SCE_SShader*, int, const char*);

typedef int (*SCE_FShaderGetAttribIndex)(SCE_SShader*, const char*);
#ifdef SCE_USE_CG
static int SCE_Shader_GetAttribIndexCG (SCE_SShader*, const char*);
#endif
static int SCE_Shader_GetAttribIndexGLSL (SCE_SShader*, const char*);

/*******/
typedef void (*SCE_FShaderParam)(int, const char*, int);
#ifdef SCE_USE_CG
static void SCE_Shader_ParamCG (int, const char*, int);
#endif
static void SCE_Shader_ParamGLSL (int, const char*, int);

typedef void (*SCE_FShaderParamf)(int, const char*, float);
#ifdef SCE_USE_CG
static void SCE_Shader_ParamfCG (int, const char*, float);
#endif
static void SCE_Shader_ParamfGLSL (int, const char*, float);

typedef void (*SCE_FShaderParamfv)(int, const char*, size_t, float*);
#ifdef SCE_USE_CG
static void SCE_Shader_Param1fvCG (int, const char*, size_t, float*);
static void SCE_Shader_Param2fvCG (int, const char*, size_t, float*);
static void SCE_Shader_Param3fvCG (int, const char*, size_t, float*);
static void SCE_Shader_Param4fvCG (int, const char*, size_t, float*);
#endif
static void SCE_Shader_Param1fvGLSL (int, const char*, size_t, float*);
static void SCE_Shader_Param2fvGLSL (int, const char*, size_t, float*);
static void SCE_Shader_Param3fvGLSL (int, const char*, size_t, float*);
static void SCE_Shader_Param4fvGLSL (int, const char*, size_t, float*);

/*******/
typedef void (*SCE_FShaderSetParam)(int, int);
#ifdef SCE_USE_CG
static void SCE_Shader_SetParamCG (int, int);
#endif
static void SCE_Shader_SetParamGLSL (int, int);

typedef void (*SCE_FShaderSetParamf)(int, float);
#ifdef SCE_USE_CG
static void SCE_Shader_SetParamfCG (int, float);
#endif
static void SCE_Shader_SetParamfGLSL (int, float);

/* already defined in SCEShaders.h */
/*typedef void (*SCE_FShaderSetParamfv)(int, size_t, float*);*/
#ifdef SCE_USE_CG
static void SCE_Shader_SetParam1fvCG (int, size_t, float*);
static void SCE_Shader_SetParam2fvCG (int, size_t, float*);
static void SCE_Shader_SetParam3fvCG (int, size_t, float*);
static void SCE_Shader_SetParam4fvCG (int, size_t, float*);
#endif
static void SCE_Shader_SetParam1fvGLSL (int, size_t, float*);
static void SCE_Shader_SetParam2fvGLSL (int, size_t, float*);
static void SCE_Shader_SetParam3fvGLSL (int, size_t, float*);
static void SCE_Shader_SetParam4fvGLSL (int, size_t, float*);
/*******/

/* already defined in SCEShaders.h */
/*typedef void (*SCE_FShaderSetMatrix)(int, SCE_TMatrix4);*/
#ifdef SCE_USE_CG
static void SCE_Shader_SetMatrix3CG (int, SCE_TMatrix4);
static void SCE_Shader_SetMatrix4CG (int, SCE_TMatrix4);
#endif
static void SCE_Shader_SetMatrix3GLSL (int, SCE_TMatrix4);
static void SCE_Shader_SetMatrix4GLSL (int, SCE_TMatrix4);
/*******/

typedef void (*SCE_FShaderUse)(SCE_SShader*);
#ifdef SCE_USE_CG
static void SCE_Shader_UseCG (SCE_SShader*);
#endif
static void SCE_Shader_UseGLSL (SCE_SShader*);


static const SCE_FShaderDelete Delete[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_DeleteCG,
#else
    NULL,
#endif
    SCE_Shader_DeleteGLSL
};

/*******/
static const SCE_FShaderBuild Build[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_BuildCG,
#else
    NULL,
#endif
    SCE_Shader_BuildGLSL
};

/*******/
static const SCE_FShaderGetIndex GetIndex[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_GetIndexCG,
#else
    NULL,
#endif
    SCE_Shader_GetIndexGLSL
};

static const SCE_FShaderGetAttribIndex GetAttribIndex[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_GetAttribIndexCG,
#else
    NULL,
#endif
    SCE_Shader_GetAttribIndexGLSL
};

/*******/
static const SCE_FShaderParam Param[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_ParamCG,
#else
    NULL,
#endif
    SCE_Shader_ParamGLSL
};

static const SCE_FShaderParamf Paramf[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_ParamfCG,
#else
    NULL,
#endif
    SCE_Shader_ParamfGLSL
};


static const SCE_FShaderParamfv Param1fv[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_Param1fvCG,
#else
    NULL,
#endif
    SCE_Shader_Param1fvGLSL
};
static const SCE_FShaderParamfv Param2fv[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_Param2fvCG,
#else
    NULL,
#endif
    SCE_Shader_Param2fvGLSL
};
static const SCE_FShaderParamfv Param3fv[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_Param3fvCG,
#else
    NULL,
#endif
    SCE_Shader_Param3fvGLSL
};
static const SCE_FShaderParamfv Param4fv[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_Param4fvCG,
#else
    NULL,
#endif
    SCE_Shader_Param4fvGLSL
};


/*******/
static const SCE_FShaderSetParam SetParam[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_SetParamCG,
#else
    NULL,
#endif
    SCE_Shader_SetParamGLSL
};


static const SCE_FShaderSetParamf SetParamf[2] =\
{
#ifdef SCE_USE_CG
    SCE_Shader_SetParamfCG,
#else
    NULL,
#endif
    SCE_Shader_SetParamfGLSL
};


static const SCE_FShaderSetParamfv SetParam1fv[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_SetParam1fvCG,
#else
    NULL,
#endif
    SCE_Shader_SetParam1fvGLSL
};
static const SCE_FShaderSetParamfv SetParam2fv[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_SetParam2fvCG,
#else
    NULL,
#endif
    SCE_Shader_SetParam2fvGLSL
};
static const SCE_FShaderSetParamfv SetParam3fv[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_SetParam3fvCG,
#else
    NULL,
#endif
    SCE_Shader_SetParam3fvGLSL
};
static const SCE_FShaderSetParamfv SetParam4fv[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_SetParam4fvCG,
#else
    NULL,
#endif
    SCE_Shader_SetParam4fvGLSL
};


static const SCE_FShaderSetMatrix SetMatrix3[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_SetMatrix3CG,
#else
    NULL,
#endif
    SCE_Shader_SetMatrix3GLSL
};

static const SCE_FShaderSetMatrix SetMatrix4[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_SetMatrix4CG,
#else
    NULL,
#endif
    SCE_Shader_SetMatrix4GLSL
};


/*******/
static const SCE_FShaderUse Use[2] =
{
#ifdef SCE_USE_CG
    SCE_Shader_UseCG,
#else
    NULL,
#endif
    SCE_Shader_UseGLSL
};

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
    SCE_Media_Register (resource_source_type,
                        ".glsl .vert .frag"
#ifdef SCE_USE_CG
                        " .cg .vcg .pcg .fcg .cgvs .cgps"
#endif
                        ,SCE_Shader_LoadSourceFromFile, NULL);
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
    shader->p_glsl = NULL;
    shader->v = shader->p = NULL;

    shader->type = SCE_UNKNOWN_SHADER;
    shader->ready = SCE_FALSE;
    shader->res[0] = shader->res[1] = NULL;
    shader->vs_source = shader->ps_source = NULL;
    shader->vs_addsrc = shader->ps_addsrc = NULL;

    shader->params_i = shader->params_f = shader->params_m = NULL;
    SCE_SceneResource_Init (&shader->s_resource);
    SCE_SceneResource_SetResource (&shader->s_resource, shader);
}

SCE_SShader* SCE_Shader_Create (int type)
{
    SCE_SShader *shader = NULL;

    SCE_btstart ();
    shader = SCE_malloc (sizeof *shader);
    if (!shader)
        SCEE_LogSrc ();
    else
    {
#define SCE_SHDASSERT(c)if(c){\
    SCE_Shader_Delete (shader);\
    SCEE_LogSrc ();\
    SCE_btend ();\
    return NULL;\
}
        SCE_Shader_Init (shader);
        shader->type = type;
        SCE_SHDASSERT (!(shader->params_i =
                         SCE_List_Create (SCE_Shader_DeleteParam)))
        SCE_SHDASSERT (!(shader->params_f =
                         SCE_List_Create (SCE_Shader_DeleteParam)))
        SCE_SHDASSERT (!(shader->params_m =
                         SCE_List_Create (SCE_Shader_DeleteParam)))
#undef SCE_SHDASSERT
    }
    SCE_btend ();
    return shader;
}


#ifdef SCE_USE_CG
static void SCE_Shader_DeleteCG (SCE_SShader *shader)
{
    SCE_btstart ();
    SCE_RDeleteShaderCG (shader->v);
    SCE_RDeleteShaderCG (shader->p);
    SCE_btend ();
}
#endif
static void SCE_Shader_DeleteGLSL (SCE_SShader *shader)
{
    SCE_btstart ();
    SCE_RDeleteProgram (shader->p_glsl);
    SCE_RDeleteShaderGLSL (shader->v);
    SCE_RDeleteShaderGLSL (shader->p);
    SCE_btend ();
}
/* revise le 19/10/2007 */
void SCE_Shader_Delete (SCE_SShader *shader)
{
    SCE_btstart ();
    if (shader)
    {
        if (!SCE_Resource_Free (shader))
            return;
        SCE_SceneResource_RemoveResource (&shader->s_resource);
        Delete[shader->type] (shader);

        SCE_free (shader->vs_addsrc);
        SCE_free (shader->ps_addsrc);

        if (SCE_Resource_Free (shader->res[0]))
        {
            SCE_free (shader->res[0][0]);
            SCE_free (shader->res[0][1]);
            SCE_free (shader->res[0]);
        }
        if (SCE_Resource_Free (shader->res[1]))
        {
            SCE_free (shader->res[1][0]);
            SCE_free (shader->res[1][1]);
            SCE_free (shader->res[1]);
        }

        SCE_List_Delete (shader->params_m);
        SCE_List_Delete (shader->params_f);
        SCE_List_Delete (shader->params_i);
        SCE_free (shader);
    }
    SCE_btend ();
}


SCE_SSceneResource* SCE_Shader_GetSceneResource (SCE_SShader *shader)
{
    return &shader->s_resource;
}


int SCE_Shader_GetLanguage (SCE_SShader *shader)
{
    return shader->type;
}

int SCE_Shader_GetType (SCE_SShader *shader)
{
    if ((shader->ps_source != NULL || shader->ps_addsrc != NULL) &&
        (shader->vs_source != NULL || shader->vs_addsrc != NULL) )
        return SCE_UNKNOWN_SHADER;
    else if (shader->ps_source != NULL || shader->ps_addsrc != NULL)
        return SCE_PIXEL_SHADER;
    else
        return SCE_VERTEX_SHADER;
}


/* retourne le type d'un shader d'apres son extension */
static void SCE_Shader_SearchTypes (const char *ext, int *type)
{
    SCE_btstart ();
    #define SCE_SHADER_FOR(str, val, val2)\
    if (SCE_String_Cmp (str, ext, SCE_FALSE) == 0)\
    {\
        type[0] = val;\
        type[1] = val2;\
    }\
    else

    /* NOTE: c'est bien statique tout ça... il faudrait rajouter une
             fonctionnalite de stockage d'extensions */
    SCE_SHADER_FOR (".glsl", SCE_GLSL_SHADER, SCE_UNKNOWN_SHADER)
    SCE_SHADER_FOR (".vert", SCE_GLSL_SHADER, SCE_VERTEX_SHADER)
    SCE_SHADER_FOR (".frag", SCE_GLSL_SHADER, SCE_PIXEL_SHADER)

#ifdef SCE_USE_CG
    SCE_SHADER_FOR (".cg", SCE_RG_SHADER, SCE_UNKNOWN_SHADER)
    SCE_SHADER_FOR (".vcg", SCE_RG_SHADER, SCE_VERTEX_SHADER)
    SCE_SHADER_FOR (".pcg", SCE_RG_SHADER, SCE_PIXEL_SHADER)
    SCE_SHADER_FOR (".fcg", SCE_RG_SHADER, SCE_PIXEL_SHADER)
    SCE_SHADER_FOR (".cgvs", SCE_RG_SHADER, SCE_VERTEX_SHADER)
    SCE_SHADER_FOR (".cgps", SCE_RG_SHADER, SCE_PIXEL_SHADER)
#endif
    {
        SCEE_Log (SCE_INVALID_ARG);
        SCEE_LogMsg ("'%s' is not a valid shader source extension", ext);
        type[0] = type[1] = SCE_UNKNOWN_SHADER;
    }
#undef SCE_SHADER_FOR
    SCE_btend ();
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

    SCE_btstart ();
    lenstr = strlen (str);
    curo = ftell (fp);

    while (1)
    {
        fseek (fp, curo+cur, SEEK_SET);
        cur++;

        for (i=0; i<lenstr; i++)
        {
            buf[i] = fgetc (fp);
            if (buf[i] == EOF) /* fin du fichier */
            {
                fseek (fp, curo, SEEK_SET);
                SCE_btend ();
                return 0;
            }
        }

        if (SCE_String_Cmp (str, buf, SCE_FALSE) == 0)
        {
            if (!at_end)
                fseek (fp, -lenstr, SEEK_CUR);
            SCE_btend ();
            return 1;
        }
    }

    fseek (fp, curo, SEEK_SET);

    SCE_btend ();
    return 0;
}

/* charge du texte a partir d'un fichier, jusqu'a end */
static char* SCE_Shader_LoadSource (FILE *fp, long end)
{
    char *src = NULL;
    long len = ftell (fp);
    long curpos;

    SCE_btstart ();
    if (end == EOF)
    {
        /* on calcul la distance qu'il y a jusqu'a la fin du fichier */
        curpos = len;
        fseek (fp, 0, SEEK_END);
        len = ftell (fp) - len;
        fseek (fp, curpos, SEEK_SET);
    }
    else
        len = end - len;

    /* dans le cas ou la zone serait vide, aucun code de shader du type prefixe
       n'est present a cet endroit, mais pas la peine d'en faire un fromage */
    if (len <= 0)
    {
        SCE_btend ();
        return NULL;
    }

    src = SCE_malloc (len+1);
    if (!src)
    {
        SCEE_LogSrc ();
        SCE_btend ();
        return NULL;
    }
    src[len] = '\0';

    fread (src, 1, len, fp);

    SCE_btend ();
    return src;
}

static void* SCE_Shader_LoadSources (FILE *fp, const char *fname)
{
    char **srcs = NULL;
    long vpos, ppos, vend, pend;
    int type[2];

    SCE_btstart ();
    srcs = SCE_malloc (2 * sizeof *srcs);
    if (!srcs)
    {
        SCEE_LogSrc ();
        SCE_btend ();
        return NULL;
    }
    srcs[0] = srcs[1] = NULL;

    /* recherche du type du shader */
    SCE_Shader_SearchTypes (SCE_String_GetExt ((char*)fname), type);
    if (type[1] == SCE_UNKNOWN_SHADER)
    {
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
    }
    else
    {
        /* le fichier contient le code de type[1] */
        srcs[0] = SCE_Shader_LoadSource (fp, EOF);

        if (type[1] == SCE_PIXEL_SHADER)
        {
            srcs[1] = srcs[0];
            srcs[0] = NULL;
        }
    }
    SCE_btend ();
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

    SCE_btstart ();
    srcs = SCE_Shader_LoadSources (fp, fname);
    if (!srcs)
    {
        SCEE_LogSrc ();
        SCE_btend ();
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
            if (!isrcs)
            {
                SCEE_LogSrc ();
                SCE_btend ();
                return NULL;
            }
            /* si le fichier contient des donnees du meme type */
            if (isrcs[j])
            {
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
    SCE_btend ();
    return srcs;
}

static void* SCE_Shader_LoadResource (const char *name, int force, void *data)
{
    SCE_SShader *shader = NULL;
    char **srcs1 = NULL, **srcs2 = NULL;
    char *vsource = NULL, *psource = NULL;
    int type[2];
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
        if (ptr2 != NULL) {     /* both */
            pname = &ptr[1];
        } else {                /* no pixel shader */
            vname = vnamebuf;
        }
    }
    *ptr = 0;
    if (ptr2)
        *ptr2 = 0;

    if (force > 0)
        force--;

    if (vname) {
        SCE_Shader_SearchTypes (SCE_String_GetExt ((char*)vname), type);

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
        ttemp = type[0];
    }

    if (pname) {
        SCE_Shader_SearchTypes (SCE_String_GetExt ((char*)pname), type);

        srcs2 = SCE_Resource_Load (resource_source_type, pname, force, NULL);
        if (!srcs2) {
            SCEE_LogSrc ();
            return NULL;
        }

        /* on vient de charger un fichier contenant les deux codes */
        if (srcs2[0] && srcs2[1])
            vsource = srcs2[0];

        psource = srcs2[1];

        if (ttemp != type[0] && vname) {
            /* le type du pixel shader differe de celui du vertex shader */
            if (SCE_Resource_Free (srcs1)) {
                SCE_free (vsource);
                SCE_free (psource);
                SCE_free (srcs1);
            }
            SCEE_Log (SCE_INVALID_ARG);
            SCEE_LogMsg ("you can't load a %s vertex shader with"
                         " a %s pixel shader",
                         (ttemp == SCE_GLSL_SHADER) ? "GLSL" : "Cg",
                         (type[0] == SCE_GLSL_SHADER) ? "GLSL" : "Cg");
            return NULL;
        }
    }

    shader = SCE_Shader_Create (type[0]);
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


#define SCE_SHADER_BUILDFUNC(bign, add)\
static int SCE_Shader_Build##bign (SCE_SShader *shader)\
{\
    SCE_btstart ();\
    if (shader->vs_source)\
    {\
        shader->v = SCE_RCreateShader##bign (SCE_VERTEX_SHADER);\
        if (!shader->v)\
        {\
            SCEE_LogSrc ();\
            SCE_btend ();\
            return SCE_ERROR;\
        }\
        SCE_RSetShader##bign##Source (shader->v, shader->vs_source);\
        if (SCE_RBuildShader##bign (shader->v) < 0)\
        {\
            SCE_RDeleteShader##bign (shader->v);\
            SCEE_LogSrc ();\
            SCE_btend ();\
            return SCE_ERROR;\
        }\
    }\
\
    if (shader->ps_source)\
    {\
        shader->p = SCE_RCreateShader##bign (SCE_PIXEL_SHADER);\
        if (!shader->p)\
        {\
            SCEE_LogSrc ();\
            SCE_btend ();\
            return SCE_ERROR;\
        }\
        SCE_RSetShader##bign##Source (shader->p, shader->ps_source);\
        if (SCE_RBuildShader##bign (shader->p) < 0)\
        {\
            SCE_RDeleteShader##bign (shader->p);\
            SCEE_LogSrc ();\
            SCE_btend ();\
            return SCE_ERROR;\
        }\
    }\
    add\
    SCE_btend ();\
    return SCE_OK;\
}

#ifdef SCE_USE_CG
SCE_SHADER_BUILDFUNC (CG, )
#endif
SCE_SHADER_BUILDFUNC
(GLSL, 
 shader->p_glsl = SCE_RCreateProgram ();
 if (!shader->p_glsl)
 {
     SCE_RDeleteShaderGLSL (shader->v);
     SCE_RDeleteShaderGLSL (shader->p);
     SCEE_LogSrc ();
     SCE_btend ();
     return SCE_ERROR;
 }
 if (shader->v)
     SCE_RSetProgramShader (shader->p_glsl, shader->v, 1);
 if (shader->p)
     SCE_RSetProgramShader (shader->p_glsl, shader->p, 1);
 if (SCE_RBuildProgram (shader->p_glsl) < 0)
 {
     SCE_RDeleteProgram (shader->p_glsl);
     SCE_RDeleteShaderGLSL (shader->v);
     SCE_RDeleteShaderGLSL (shader->p);
     SCEE_LogSrc ();
     SCE_btend ();
     return SCE_ERROR;
 }
)

int SCE_Shader_Build (SCE_SShader *shader)
{
    SCE_btstart ();
#ifdef SCE_DEBUG
    /* si le shader ne contient aucun code source */
    if (!shader->vs_source && !shader->ps_source &&
        !shader->vs_addsrc && !shader->ps_addsrc)
    {
        SCEE_Log (SCE_INVALID_OPERATION);
        SCEE_LogMsg ("this shader don't have a source code!");
        SCE_btend ();
        return SCE_ERROR;
    }
#endif

    if (shader->vs_source || shader->vs_addsrc)
    {
        shader->vs_source = SCE_String_CatDup (shader->vs_addsrc, shader->vs_source);
        if (!shader->vs_source)
        {
            SCEE_LogSrc ();
            SCE_btend ();
            return SCE_ERROR;
        }
    }
    if (shader->ps_source || shader->ps_addsrc)
    {
        shader->ps_source = SCE_String_CatDup (shader->ps_addsrc, shader->ps_source);
        if (!shader->ps_source)
        {
            SCEE_LogSrc ();
            SCE_btend ();
            return SCE_ERROR;
        }
    }

    if (Build[shader->type] (shader) < 0)
    {
        SCEE_LogSrc ();
        SCE_btend ();
        return SCE_ERROR;
    }

    shader->ready = SCE_TRUE;
    SCE_btend ();
    return SCE_OK;
}


/* revise le 19/10/2007 */
int SCE_Shader_AddSource (SCE_SShader *shader, int type, const char *src)
{
    /* taille de la reallocation */
    size_t realen;
    int first_alloc = 1;
    char *addsrc = NULL;

    if (!src)
        return SCE_OK;

#ifdef SCE_DEBUG
    if (type != SCE_PIXEL_SHADER && type != SCE_VERTEX_SHADER &&
        type != SCE_UNKNOWN_SHADER) {
        SCEE_Log (SCE_INVALID_ARG);
        return SCE_ERROR;
    }
#endif

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


#ifdef SCE_USE_CG
static int SCE_Shader_GetIndexCG (SCE_SShader *s, int t, const char *n)
{
    if (t == SCE_PIXEL_SHADER)
        return (int)SCE_RGetShaderCGIndex (s->p, n);
    else
        return (int)SCE_RGetShaderCGIndex (s->v, n);
}
#endif
static int SCE_Shader_GetIndexGLSL (SCE_SShader *s, int t, const char *n)
{
    (void)t;
    return SCE_RGetProgramIndex (s->p_glsl, n);
}

int SCE_Shader_GetIndex (SCE_SShader *shader, int type, const char *name)
{
#ifdef SCE_DEBUG
    if (!shader || !name)
    {
        SCEE_Log (SCE_INVALID_ARG);
        return SCE_SHADER_BAD_INDEX;
    }
    if (!shader->ready)
    {
        SCEE_Log (SCE_INVALID_OPERATION);
        SCEE_LogMsg ("you can't use a non-built shader");
        return SCE_SHADER_BAD_INDEX;
    }
#endif

    return GetIndex[shader->type] (shader, type, name);
}


#ifdef SCE_USE_CG
static int SCE_Shader_GetAttribIndexCG (SCE_SShader *s, const char *n)
{
    return 0; /* TODO: undefined... */
}
#endif
static int SCE_Shader_GetAttribIndexGLSL (SCE_SShader *s, const char *n)
{
    return SCE_RGetProgramAttribIndex (s->p_glsl, n);
}

int SCE_Shader_GetAttribIndex (SCE_SShader *shader, const char *name)
{
    return GetAttribIndex[shader->type] (shader, name);
}


#ifdef SCE_USE_CG
static void SCE_Shader_ParamCG (int t, const char *n, int v)
{
    SCE_RSetShaderCGParam ((CGparameter)SCE_Shader_GetIndexCG (used, t, n), v);
}
#endif
static void SCE_Shader_ParamGLSL (int t, const char *n, int v)
{
    SCE_RSetProgramParam (SCE_RGetProgramIndex (used->p_glsl, n), v);
}
void SCE_Shader_Param (int type, const char *name, int val)
{
    Param[used->type] (type, name, val);
}


#ifdef SCE_USE_CG
static void SCE_Shader_ParamfCG (int t, const char *n, float v)
{
    SCE_RSetShaderCGParamf ((CGparameter)SCE_Shader_GetIndexCG (used, t, n), v);
}
#endif
static void SCE_Shader_ParamfGLSL (int t, const char *n, float v)
{
    SCE_RSetProgramParamf (SCE_RGetProgramIndex (used->p_glsl, n), v);
}
void SCE_Shader_Paramf (int type, const char *name, float val)
{
    Paramf[used->type] (type, name, val);
}


#define SCE_RG_PARAM_FUNC(n)\
static void SCE_Shader_Param##n##fvCG (int t, const char *na, size_t s, float *v)\
{\
    SCE_RSetShaderCGParam##n##fv ((CGparameter)SCE_Shader_GetIndexCG\
                                  (used, t, na), s, v);\
}
#define SCE_GLSL_PARAM_FUNC(n)\
static void SCE_Shader_Param##n##fvGLSL (int t, const char *na,\
                                         size_t s, float *v)\
{\
    SCE_RSetProgramParam##n##fv (SCE_RGetProgramIndex (used->p_glsl, na), s, v);\
}\
void SCE_Shader_Param##n##fv (int type, const char *name, size_t size, float *val)\
{\
    Param##n##fv[used->type] (type, name, size, val);\
}
#ifdef SCE_USE_CG
SCE_RG_PARAM_FUNC(1)
SCE_RG_PARAM_FUNC(2)
SCE_RG_PARAM_FUNC(3)
SCE_RG_PARAM_FUNC(4)
#endif
SCE_GLSL_PARAM_FUNC(1)
SCE_GLSL_PARAM_FUNC(2)
SCE_GLSL_PARAM_FUNC(3)
SCE_GLSL_PARAM_FUNC(4)
#undef SCE_GLSL_PARAM_FUNC
#undef SCE_RG_PARAM_FUNC


#ifdef SCE_USE_CG
static void SCE_Shader_SetParamCG (int i, int v)
{
    SCE_RSetShaderCGParam ((CGparameter)i, v);
}
#endif
static void SCE_Shader_SetParamGLSL (int i, int v)
{
    SCE_RSetProgramParam (i, v);
}
void SCE_Shader_SetParam (int index, int val)
{
    SetParam[used->type] (index, val);
}


#ifdef SCE_USE_CG
static void SCE_Shader_SetParamfCG (int i, float v)
{
    SCE_RSetShaderCGParamf ((CGparameter)i, v);
}
#endif
static void SCE_Shader_SetParamfGLSL (int i, float v)
{
    SCE_RSetProgramParamf (i, v);
}
void SCE_Shader_SetParamf (int index, float val)
{
    SetParamf[used->type] (index, val);
}


#define SCE_RG_SETPARAM_FUNC(n)\
static void SCE_Shader_SetParam##n##fvCG (int i, size_t s, float *v)\
{\
    SCE_RSetShaderCGParam##n##fv ((CGparameter)i, s, v);\
}
#define SCE_GLSL_SETPARAM_FUNC(n)\
static void SCE_Shader_SetParam##n##fvGLSL (int i, size_t s, float *v)\
{\
    SCE_RSetProgramParam##n##fv (i, s, v);\
}\
void SCE_Shader_SetParam##n##fv (int index, size_t size, float *val)\
{\
    SetParam##n##fv[used->type] (index, size, val);\
}
#ifdef SCE_USE_CG
SCE_RG_SETPARAM_FUNC(1)
SCE_RG_SETPARAM_FUNC(2)
SCE_RG_SETPARAM_FUNC(3)
SCE_RG_SETPARAM_FUNC(4)
#endif
SCE_GLSL_SETPARAM_FUNC(1)
SCE_GLSL_SETPARAM_FUNC(2)
SCE_GLSL_SETPARAM_FUNC(3)
SCE_GLSL_SETPARAM_FUNC(4)
#undef SCE_GLSL_SETPARAM_FUNC
#undef SCE_RG_SETPARAM_FUNC

#ifdef SCE_USE_CG
static void SCE_Shader_SetMatrix3CG (int i, SCE_TMatrix4 m)
{
    /* TODO: where are Cg API's funcs to manage 3x3 matrices ? */
    /* kick warnings */
    i = 0; m = NULL;
}
#endif
static void SCE_Shader_SetMatrix3GLSL (int i, SCE_TMatrix4 m)
{
    SCE_RSetProgramMatrix3 (i, 1, m); /* count forced */
}
void SCE_Shader_SetMatrix3 (int index, SCE_TMatrix4 m)
{
    SetMatrix3[used->type] (index, m);
}

#ifdef SCE_USE_CG
static void SCE_Shader_SetMatrix4CG (int i, SCE_TMatrix4 m)
{
    SCE_RSetShaderCGMatrix ((CGparameter)i, m);
}
#endif
static void SCE_Shader_SetMatrix4GLSL (int i, SCE_TMatrix4 m)
{
    SCE_RSetProgramMatrix4 (i, 1, m); /* count forced */
}
void SCE_Shader_SetMatrix4 (int index, SCE_TMatrix4 m)
{
    SetMatrix4[used->type] (index, m);
}

/**
 * \brief Adds a constant parameter
 * \param type shader's type where send the parameter (can be SCE_PIXEL_SHADER
 * or SCE_VERTEX_SHADER)
 * \param n the name of the parameter in the shader source code
 * \param p pointer to the values that will be send
 *
 * Adds a parameter that will be send on each call of SCE_Shader_Use(\p shader).
 *
 * \sa SCE_Shader_AddParamfv()
 */
int SCE_Shader_AddParamv (SCE_SShader *shader, int type, const char *n, void *p)
{
    SCE_SShaderParam *param = NULL;
    SCE_btstart ();
    if (!(param = SCE_malloc (sizeof *param)))
    {
        SCEE_LogSrc ();
        SCE_btend ();
        return SCE_ERROR;
    }
    SCE_Shader_InitParam (param);
    SCE_List_Appendl (shader->params_i, &param->it);
    param->param = p;
    /* necessite que le shader ait deja ete construit */
    param->index = SCE_Shader_GetIndex (shader, type, n);
    SCE_btend ();
    return SCE_OK;
}
/**
 * \brief Adds a constant floating parameter
 * \param type shader's type where send the parameter (can be SCE_PIXEL_SHADER
 * or SCE_VERTEX_SHADER)
 * \param n the name of the parameter in the shader source code
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
int SCE_Shader_AddParamfv (SCE_SShader *shader, int type, const char *n,
                           int num, int size, void *p)
{
    SCE_SShaderParam *param = NULL;
    SCE_btstart ();
    if (!(param = SCE_malloc (sizeof *param)))
    {
        SCEE_LogSrc ();
        SCE_btend ();
        return SCE_ERROR;
    }
    SCE_Shader_InitParam (param);
    SCE_List_Appendl (shader->params_f, &param->it);
    param->param = p;
    switch (num)
    {
#define SCE_SHDCASE(n) case n: param->setfv = SCE_Shader_SetParam##n##fv; break;
    SCE_SHDCASE (1)
    SCE_SHDCASE (2)
    SCE_SHDCASE (3)
    SCE_SHDCASE (4)
#undef SCE_SHDCASE
    }
    param->size = size;
    /* necessite que le shader ait deja ete construit */
    param->index = SCE_Shader_GetIndex (shader, type, n);
    /* NOTE: et la on ne verifie pas index... ? bof ca fait chier */
    SCE_btend ();
    return SCE_OK;
}
/* ajoute le 23/09/2008 */
/**
 * \brief Adds a constant matrix
 * \param type shader's type where send the matrix (can be SCE_PIXEL_SHADER
 * or SCE_VERTEX_SHADER)
 * \param n the name of the matrix in the shader source code
 * \param p pointer to the matrix
 *
 * Adds a matrix that will be send on each call of SCE_Shader_Use(\p shader).
 *
 * \sa SCE_Shader_AddParamfv()
 */
int SCE_Shader_AddMatrix (SCE_SShader *shader, int type, const char *n,
                          int size, void *p)
{
    SCE_SShaderParam *param = NULL;
    SCE_btstart ();
    if (!(param = SCE_malloc (sizeof *param)))
    {
        SCEE_LogSrc ();
        SCE_btend ();
        return SCE_ERROR;
    }
    SCE_Shader_InitParam (param);
    SCE_List_Appendl (shader->params_m, &param->it);
    param->param = p;
    /* necessite que le shader ait deja ete construit */
    param->index = SCE_Shader_GetIndex (shader, type, n);
    if (size == 3)
        param->setm = SCE_Shader_SetMatrix3;
    else
        param->setm = SCE_Shader_SetMatrix4;
    SCE_btend ();
    return SCE_OK;
}


/* ajoute le 14/06/2007 */
void SCE_Shader_Active (int enabled)
{
    sce_shd_enabled = enabled;
}
/* ajoute le 14/06/2007 */
void SCE_Shader_Enable (void)
{
    sce_shd_enabled = SCE_TRUE;
}
/* ajoute le 14/06/2007 */
void SCE_Shader_Disable (void)
{
    sce_shd_enabled = SCE_FALSE;
}

/* ajoute le 05/05/2008 */
/* revise le 23/09/2008 */
static void SCE_Shader_SetParams (SCE_SShader *shader)
{
    SCE_SListIterator *i = NULL;
    SCE_SShaderParam *p = NULL;
    SCE_List_ForEach (i, shader->params_i)
    {
        p = SCE_List_GetData (i);
        SCE_Shader_SetParam (p->index, *((int*)p->param));
    }
    SCE_List_ForEach (i, shader->params_f)
    {
        p = SCE_List_GetData (i);
        p->setfv (p->index, p->size, p->param);
    }
    SCE_List_ForEach (i, shader->params_m)
    {
        p = SCE_List_GetData (i);
        p->setm (p->index, p->param);
    }
}

#ifdef SCE_USE_CG
static void SCE_Shader_UseCG (SCE_SShader *shader)
{
    if (!sce_shd_enabled)
        return;
    else if (shader)
    {
        if (shader->v)
            SCE_RUseShaderCG (shader->v);
        if (shader->p)
            SCE_RUseShaderCG (shader->p);
    }
    else
        SCE_RUseShaderCG (NULL);
}
#endif
/* revise le 12/01/2008 */
static void SCE_Shader_UseGLSL (SCE_SShader *shader)
{
    if (!sce_shd_enabled)
        return;
    else if (shader)
        SCE_RUseProgram (shader->p_glsl);
    else
        SCE_RUseProgram (NULL);
}
/* revise le 28/11/2008 */
void SCE_Shader_Use (SCE_SShader *shader)
{
    static void *pixelshader = NULL;
    static int vs_binded = SCE_FALSE;

    if (!shader)
    {
        if (used)
        {
            Use[used->type] (NULL);
            Use[used->type] (NULL);
            used = NULL;
        }

        pixelshader = NULL;
        vs_binded = SCE_FALSE;
    }
    else if (shader == used)
        SCE_Shader_SetParams (shader);
    else
    {
        if (used && used->type != shader->type)
        {
            Use[used->type] (NULL); /* pixel shader */
            Use[used->type] (NULL); /* vertex shader */
            vs_binded = SCE_FALSE;
            pixelshader = NULL;
        }

        if (shader->type == SCE_GLSL_SHADER)
            Use[shader->type] (shader);
        else
        {
            /* analyse du type du shader */
            if (!shader->v)
            {
                /* aucun vertex shader n'est present, on verifie s'il y en
                   a deja un d'actif, si oui, on active le pixel shader
                   du shader envoye, sinon, on retient l'adresse du pixel shader
                   pour une activation ulterieur */
                if (vs_binded)
                    Use[shader->type] (shader);
                else
                    pixelshader = shader->p;
            }
            else
            {
                void *tmp = shader->p;

                if (pixelshader && !tmp)
                {
                    shader->p = pixelshader;
                    pixelshader = NULL;
                }

                /* un vertex shader est present, on l'active */
                Use[shader->type] (shader);
                /* on remet son pixel shader au shader */
                shader->p = tmp;
                /* un vertex shader est binde */
                vs_binded = SCE_TRUE;
            }
        }
        /* prendre soin d'affecter used avant d'appeler SetParams() */
        used = shader;
        /* envoie des parametres automatiques */
        SCE_Shader_SetParams (shader);
    }
}
