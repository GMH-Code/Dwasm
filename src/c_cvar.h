/*
 * PrBoomX: PrBoomX is a fork of PrBoom-Plus with quality-of-play upgrades. 
 *
 * Copyright (C) 2023  JadingTsunami
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * DESCRIPTION
 * ===========
 *
 * This is the Console Variable (CVAR) system for prboomX. CVARs are key-value
 * pairs that control various aspects of gameplay, rendering, etc. Ideally
 * many/most settings would migrate to CVARs. Thus it's important that lookup be
 * kept fast.
 * 
 * To achieve this, CVAR keys are stored in a hash map based on a Pearson hash
 * of the key string. This means for most lookups, the key string must be
 * scanned twice (once to generate the hash, and once again to compare the
 * retrieved key vs. the target key). Of course, worst-case performance becomes
 * linear. If that happens, using a different data structure such as an AVL
 * tree could be used to mitigate this.
 * 
 * CVAR types are inferred based on the contents. Default values are used if a
 * wholly invalid value is supplied.
 * 
 * Flags control various aspects of CVAR handling such as whether changes are
 * written to the user's settings file. Flags are accumulated even when values
 * are not overwritten.
 */

#ifndef __C_CVAR__
#define __C_CVAR__

#include "doomtype.h"
#include <stdio.h>

typedef enum cvartype_e {
    CVAR_TYPE_INT,
    CVAR_TYPE_FLOAT,
    CVAR_TYPE_STRING,
    CVAR_TYPE_MAX,
    CVAR_TYPE_INVALID
} cvartype_t;

typedef enum cvarstatus_e {
    CVAR_STATUS_OK,
    CVAR_STATUS_INVALID_KEY,
    CVAR_STATUS_INVALID_TYPE,
    CVAR_STATUS_INVALID_VALUE,
    CVAR_STATUS_WRONG_TYPE,
    CVAR_STATUS_KEY_NOT_FOUND,
    CVAR_STATUS_ALREADY_EXISTS,
    CVAR_STATUS_MAX
} cvarstatus_t;

typedef enum cvarflags_e {
    CVAR_FLAG_INIT = 0, /* initialized and not set */
    CVAR_FLAG_ARCHIVE = 1, /* write to config file upon exit */
} cvarflags_t;

/* Create or verify that all default CVARs exist */
void C_CvarInit();

dboolean C_CvarExists(const char* key);

/* Logical OR the supplied flags with the CVAR's existing flags. */
/* This function can NOT clear flag bits, only set them */
cvarstatus_t C_CvarApplyFlags(const char* key, cvarflags_t flags);

cvartype_t C_CvarGetType(const char* key, cvarstatus_t* status);

/* export all archive-flagged cvars to a console-executable file */
/* this is used to generated prboomx_console.cfg */
void C_CvarExportToFile(FILE* f);

/* returns true if:
 *  key exists
 *   and
 *  is a nonzero or non-null value
 */
dboolean C_CvarIsSet(const char* key);

/* Set/Clear a CVAR (boolean flag), create if it does not exist */
cvarstatus_t C_CvarSet(const char* key);
cvarstatus_t C_CvarClear(const char* key);

/* These functions will interpret the content as the desired type.
 * For instance, "string" will return a string, no matter whether
 * the underlying type is a float or int.
 * Float/int will convert between types if needed.
 * The status will indicate the type conversion.
 * If the status returned is CVAR_STATUS_OK, then the native
 * and requested type matched.
 */
int C_CvarGetAsInt(const char* key, cvarstatus_t* status);
float C_CvarGetAsFloat(const char* key, cvarstatus_t* status);
char* C_CvarGetAsString(const char* key, cvarstatus_t* status);

/* If the CVAR exists, the existing type and value are overwritten. */
/* Flags are OR'd with the existing flag set, flags are NOT cleared! */
cvarstatus_t C_CvarCreateOrUpdate(const char* key, const char* value, cvarflags_t flags);

/* does NOT overwrite if exists; returns failure status */
/* however, create operation WILL update flags! */
cvarstatus_t C_CvarCreate(const char* key, const char* value, cvarflags_t flags);

cvarstatus_t C_CvarDelete(const char* key);

/* will return a command completion, either:
 * raw CVAR (show value)
 * "set" command with CVAR completed
 * "unset" command with CVAR completed
 */
const char* C_CvarComplete(const char* partial);

/* NULL is returned for OK status (not an error)
 * NULL is returned for illegal status
 */
const char* C_CvarErrorToString(cvarstatus_t status);

#endif
