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
 */

#include "c_cvar.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include "c_cmd.h"

typedef union cvarval_s {
    int intVal;
    float floatVal;
} cvarval_t;

typedef struct cvar_s {
    char* key;
    unsigned int flags;
    /* fast access path for "evalutes true" */
    dboolean is_set;
    dboolean modified;
    cvartype_t type;
    cvarval_t numValue;
    char* stringVal;
    struct cvar_s* next;
    struct cvar_s* prev;
} cvar_t;

cvar_t* cvar_map[256] = {0};

/* RFC 3074 permutation table */
static const uint8_t PearsonHashT[256] ={
	0xfb, 0xaf, 0x77, 0xd7, 0x51, 0x0e, 0x4f, 0xbf, 0x67, 0x31, 0xb5, 0x8f, 0xba, 0x9d, 0x00, 0xe8,
	0x1f, 0x20, 0x37, 0x3c, 0x98, 0x3a, 0x11, 0xed, 0xae, 0x46, 0xa0, 0x90, 0xdc, 0x5a, 0x39, 0xdf,
	0x3b, 0x03, 0x12, 0x8c, 0x6f, 0xa6, 0xcb, 0xc4, 0x86, 0xf3, 0x7c, 0x5f, 0xde, 0xb3, 0xc5, 0x41,
	0xb4, 0x30, 0x24, 0x0f, 0x6b, 0x2e, 0xe9, 0x82, 0xa5, 0x1e, 0x7b, 0xa1, 0xd1, 0x17, 0x61, 0x10,
	0x28, 0x5b, 0xdb, 0x3d, 0x64, 0x0a, 0xd2, 0x6d, 0xfa, 0x7f, 0x16, 0x8a, 0x1d, 0x6c, 0xf4, 0x43,
	0xcf, 0x09, 0xb2, 0xcc, 0x4a, 0x62, 0x7e, 0xf9, 0xa7, 0x74, 0x22, 0x4d, 0xc1, 0xc8, 0x79, 0x05,
	0x14, 0x71, 0x47, 0x23, 0x80, 0x0d, 0xb6, 0x5e, 0x19, 0xe2, 0xe3, 0xc7, 0x4b, 0x1b, 0x29, 0xf5,
	0xe6, 0xe0, 0x2b, 0xe1, 0xb1, 0x1a, 0x9b, 0x96, 0xd4, 0x8e, 0xda, 0x73, 0xf1, 0x49, 0x58, 0x69,
	0x27, 0x72, 0x3e, 0xff, 0xc0, 0xc9, 0x91, 0xd6, 0xa8, 0x9e, 0xdd, 0x94, 0x9a, 0x7a, 0x0c, 0x54,
	0x52, 0xa3, 0x2c, 0x8b, 0xe4, 0xec, 0xcd, 0xf2, 0xd9, 0x0b, 0xbb, 0x92, 0x9f, 0x40, 0x56, 0xef,
	0xc3, 0x2a, 0x6a, 0xc6, 0x76, 0x70, 0xb8, 0xac, 0x57, 0x02, 0xad, 0x75, 0xb0, 0xe5, 0xf7, 0xfd,
	0x89, 0xb9, 0x63, 0xa4, 0x66, 0x93, 0x2d, 0x42, 0xe7, 0x34, 0x8d, 0xd3, 0xc2, 0xce, 0xf6, 0xee,
	0x38, 0x6e, 0x4e, 0xf8, 0x3f, 0xf0, 0xbd, 0x5d, 0x5c, 0x33, 0x35, 0xb7, 0x13, 0xab, 0x48, 0x32,
	0x21, 0x68, 0x65, 0x45, 0x08, 0xfc, 0x53, 0x78, 0x4c, 0x87, 0x55, 0x36, 0xca, 0x7d, 0xbc, 0xd5,
	0x60, 0xeb, 0x88, 0xd0, 0xa2, 0x81, 0xbe, 0x84, 0x9c, 0x26, 0x2f, 0x01, 0x07, 0xfe, 0x18, 0x04,
	0xd8, 0x83, 0x59, 0x15, 0x1c, 0x85, 0x25, 0x99, 0x95, 0x50, 0xaa, 0x44, 0x06, 0xa9, 0xea, 0x97 };


/* index-aligned to cvarstatus_t types */
const char* cvarstatus_error_strings[] =
{
    NULL,
    "Invalid CVAR key",
    "Invalid CVAR type",
    "Invalid CVAR value",
    "Wrong CVAR type supplied",
    "CVAR not found",
    "CVAR already exists",
    NULL
};

static uint8_t HashString(const char* string)
{
    uint8_t hash = 0;
    char c;
    if (!string)
        return hash;

    while ((c = *string++)) {
        hash = PearsonHashT[tolower(c) ^ hash];
    }

    return hash;
}

void C_CvarInit()
{
    static dboolean initialized = false;

    if (!initialized) {
        initialized = true;
        C_CvarCreate("allmap_always", "0", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("plat_skip", "0", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("regenerate", "0", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("hudadd_showfps", "0", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("showfps", "0", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("r_drawplayersprites", "1", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("overunder", "0", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("announce_100p_max", "0", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("weapon_bob_only", "0", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("autofire_rocketlauncher", "0", CVAR_FLAG_ARCHIVE);
        C_CvarCreate("autofire_bfg", "0", CVAR_FLAG_ARCHIVE);
    }
}

dboolean C_CvarTypeIsValid(cvartype_t type)
{
    return (type < CVAR_TYPE_MAX && type >= CVAR_TYPE_INT);
}

dboolean C_CvarKeyIsValid(const char* key)
{
    return key && isalnum(*key);
}

static cvar_t* C_CvarFindWithHash(const char* key, cvarstatus_t* status, uint8_t* cvar_hash)
{
    uint8_t hash;
    cvar_t* cvar;
    dboolean found = false;

    /* illegal to call this with NULL status, unlike
     * the public-facing functions
     */
    assert(status);

    if (!C_CvarKeyIsValid(key)) {
        *status = CVAR_STATUS_INVALID_KEY;
        return NULL;
    }

    hash = HashString(key);
    cvar = cvar_map[hash];

    while (cvar) {
        if (strcasecmp(cvar->key, key) == 0) {
            found = true;
            break;
        } else {
            cvar = cvar->next;
        }
    }

    if (found) {
        *status = CVAR_STATUS_OK;
    } else {
        *status = CVAR_STATUS_KEY_NOT_FOUND;
        cvar = NULL;
    }

    /* supply calculated hash even if key isn't found */
    if (cvar_hash)
        *cvar_hash = hash;

    return cvar;
}

/* return NULL if not found */
static cvar_t* C_CvarFind(const char* key, cvarstatus_t* status)
{
    return C_CvarFindWithHash(key, status, NULL);
}

static dboolean C_CvarIsSetInternal(cvar_t* cvar)
{
    assert(cvar);
    switch(cvar->type) {
        case CVAR_TYPE_INT:
            return cvar->numValue.intVal != 0;
            break;
        case CVAR_TYPE_FLOAT:
            return cvar->numValue.floatVal != 0;
            break;
        case CVAR_TYPE_STRING:
            return strlen(cvar->stringVal) > 0;
            break;
        case CVAR_TYPE_MAX:
        default:
            break;
    }

    return false;
}

dboolean C_CvarExists(const char* key)
{
    cvarstatus_t status;
    C_CvarFind(key, &status);
    return status == CVAR_STATUS_OK;
}

static cvartype_t C_CvarInferType(const char* value, cvarval_t* converted)
{
    float fvalue;
    int ivalue;
    char* endptr = NULL;
    char* endarg = NULL;

    /* value must be at least one char to be valid */
    if (!value || !*value)
        return CVAR_TYPE_INVALID;

    cvartype_t type = CVAR_TYPE_STRING;

    endarg = value + strlen(value);

    ivalue = strtol(value, &endptr, 0);

    if (endarg == endptr) {
        type = CVAR_TYPE_INT;
        if (converted)
            converted->intVal = ivalue;
    } else {
        fvalue = strtof(value, &endptr);

        if (endarg == endptr) {
            /* it's a float */
            type = CVAR_TYPE_FLOAT;
            if (converted)
                converted->floatVal = fvalue;
        }
    }

    /* if neither, it's a string */
    return type;
}

cvarstatus_t C_CvarCreateOrUpdate(const char* key, const char* value, cvarflags_t flags)
{
    cvarstatus_t status;
    cvartype_t type;
    cvarval_t convertedValue;
    cvar_t* cvar;
    uint8_t hash;
    char c;

    if (key)
        for (c = *key; c; c++)
            c = tolower(c);

    cvar = C_CvarFind(key, &status);

    /* validate inputs */
    if (status == CVAR_STATUS_INVALID_KEY)
        return CVAR_STATUS_INVALID_KEY;

    type = C_CvarInferType(value, &convertedValue);
    if (!C_CvarTypeIsValid(type))
        return CVAR_STATUS_INVALID_TYPE;

    if (cvar) {
        /* exists, update, but keep flags */
        flags |= cvar->flags;

        /* set modified flag */
        if (!cvar->modified) {
            if (type == CVAR_TYPE_INT) {
                cvar->modified = (cvar->numValue.intVal != convertedValue.intVal);
            } else if (type == CVAR_TYPE_FLOAT) {
                cvar->modified = (cvar->numValue.floatVal != convertedValue.floatVal);
            } else {
                cvar->modified = (strcasecmp(value, cvar->stringVal) != 0);
            }
        }

        free(cvar->stringVal);
    } else {
        /* create brand new cvar */
        hash = HashString(key);
        /* Note: add memset(cvar, 0, sizeof(cvar_t)
         * here if any future fields are not 
         * initialized here. Right now every field
         * is set to something so this isn't necessary
         */
        cvar = malloc(sizeof(cvar_t));
        cvar->key = strdup(key);
        cvar->modified = false;

        /* register in hash map */
        if (cvar_map[hash]) {
            cvar_t* noden = cvar_map[hash];
            cvar_t* nodep = NULL;

            while (noden) {
                nodep = noden;
                noden = noden->next;
            }

            nodep->next = cvar;
            cvar->next = NULL;
            cvar->prev = nodep;
        } else {
            cvar->next = NULL;
            cvar->prev = NULL;
            cvar_map[hash] = cvar;
        }
    }

    cvar->flags = flags;
    cvar->type = type;
    cvar->numValue = convertedValue;
    cvar->stringVal = strdup(value);

    cvar->is_set = C_CvarIsSetInternal(cvar);

    return CVAR_STATUS_OK;
}

cvarstatus_t C_CvarCreate(const char* key, const char* value, cvarflags_t flags)
{
    cvarstatus_t status;
    cvar_t* cvar;
    char c;

    if (key)
        for (c = *key; c; c++)
            c = tolower(c);

    cvar = C_CvarFind(key, &status);

    if (cvar) {
        cvar->flags |= flags;
        return CVAR_STATUS_ALREADY_EXISTS;
    } else {
        return C_CvarCreateOrUpdate(key, value, flags);
    }
}

cvarstatus_t C_CvarDelete(const char* key)
{
    cvarstatus_t status;
    cvar_t* cvar;
    uint8_t hash;

    cvar = C_CvarFindWithHash(key, &status, &hash);

    if (status != CVAR_STATUS_OK)
        return status;

    if (cvar->prev) {
        cvar->prev->next = cvar->next;
        if (cvar->next)
            cvar->next->prev = cvar->prev;
    } else if (cvar->next) {
        cvar_map[hash] = cvar->next;
        cvar->next->prev = NULL;
    } else {
        cvar_map[hash] = NULL;
    }

    free(cvar->stringVal);
    free(cvar->key);
    free(cvar);

    return CVAR_STATUS_OK;
}

void C_CvarExportToFile(FILE* f)
{
    int i;
    cvar_t* c;

    for (i = 0; i < 256; i++) {
        c = cvar_map[i];

        while (c) {
            if (c->modified && (c->flags & CVAR_FLAG_ARCHIVE)) {
                switch (c->type) {
                    case CVAR_TYPE_INT:
                        fprintf(f, "set %s %d\n", c->key, c->numValue.intVal);
                        break;
                    case CVAR_TYPE_FLOAT:
                        fprintf(f, "set %s %f\n", c->key, c->numValue.floatVal);
                        break;
                    case CVAR_TYPE_STRING:
                        fprintf(f, "set %s %s\n", c->key, c->stringVal);
                        break;
                    case CVAR_TYPE_MAX:
                    default:
                        /* silently skip broken cvars */
                        break;
                }
            }
            c = c->next;
        }
    }
}

/* returns true if:
 *  key exists
 *   and
 *  is a "nonzero"/non-null value, or is no-value type
 */
dboolean C_CvarIsSet(const char* key)
{
    cvarstatus_t s;
    cvar_t* cvar;

    cvar = C_CvarFind(key, &s);

    return cvar && cvar->is_set;
}

/* Set/Clear a CVAR (boolean flag), create if it does not exist */
cvarstatus_t C_CvarSet(const char* key)
{
    return C_CvarCreateOrUpdate(key, "1", 0);
}

cvarstatus_t C_CvarClear(const char* key)
{
    return C_CvarCreateOrUpdate(key, "0", 0);
}

int C_CvarGetAsInt(const char* key, cvarstatus_t* status)
{
    cvarstatus_t s;
    cvar_t* cvar;
    int value = -1;

    cvar = C_CvarFind(key, &s);

    if (cvar) {
        if (cvar->type == CVAR_TYPE_INT)
            value = cvar->numValue.intVal;
        else {
            /* try to convert but return wrong type
             * status to warn the user
             */
            if (cvar->type == CVAR_TYPE_FLOAT)
                value = (int)cvar->numValue.floatVal;
            s = CVAR_STATUS_WRONG_TYPE;
        }
    }

    if (status)
        *status = s;

    return value;
}

float C_CvarGetAsFloat(const char* key, cvarstatus_t* status)
{
    cvarstatus_t s;
    cvar_t* cvar;
    float value = NAN;

    cvar = C_CvarFind(key, &s);

    if (cvar) {
        if (cvar->type == CVAR_TYPE_FLOAT)
            value = cvar->numValue.floatVal;
        else {
            if (cvar->type == CVAR_TYPE_INT)
                value = (float)cvar->numValue.intVal;
            s = CVAR_STATUS_WRONG_TYPE;
        }
    }

    if (status)
        *status = s;

    return value;
}
char* C_CvarGetAsString(const char* key, cvarstatus_t* status)
{
    cvarstatus_t s;
    cvar_t* cvar;
    char* value = NULL;

    cvar = C_CvarFind(key, &s);

    if (cvar) {
        if (cvar->type != CVAR_TYPE_STRING)
            s = CVAR_STATUS_WRONG_TYPE;
        value = cvar->stringVal;
    }

    if (status)
        *status = s;

    return value;
}

cvartype_t C_CvarGetType(const char* key, cvarstatus_t* status)
{
    cvarstatus_t s;
    cvar_t* cvar;
    cvartype_t type = CVAR_TYPE_MAX;

    cvar = C_CvarFind(key, &s);

    if (cvar)
        type = cvar->type;

    if (status)
        *status = s;

    return type;
}

cvarstatus_t C_CvarApplyFlags(const char* key, cvarflags_t flags)
{
    cvarstatus_t s;
    cvar_t* cvar;

    cvar = C_CvarFind(key, &s);

    if (cvar)
        cvar->flags |= flags;

    return s;
}

static cvarstatus_t C_CvarOverwriteFlags(const char* key, cvarflags_t flags)
{
    cvarstatus_t s;
    cvar_t* cvar;

    cvar = C_CvarFind(key, &s);

    if (cvar)
        cvar->flags = flags;

    return s;
}

const char* C_CvarComplete(const char* partial)
{
    int i;
    int partial_len;
    /* because this handler can send back commands
     * and not just references, it can send a reference
     * to a static completion buffer also.
     */
    dboolean set_cmd = false;
    dboolean unset_cmd = false;
    cvar_t* cvar;
    static char* completion_buffer = NULL;

    if (!completion_buffer)
        completion_buffer = malloc(sizeof(char)*(CONSOLE_LINE_LENGTH_MAX+1));

    if (partial && *partial) {
        partial_len = strlen(partial);
    } else {
        return NULL;
    }

    if (strncasecmp(partial, "set ", 4) == 0) {
        set_cmd = true;
        partial += 4;
        partial_len -= 4;
    } else if (strncasecmp(partial, "unset ", 6) == 0) {
        unset_cmd = true;
        partial += 6;
        partial_len -= 6;
    }

    for (i = 0; i < 256; i++) {
        for (cvar = cvar_map[i]; cvar; cvar = cvar->next) {
            if (cvar && strncasecmp(cvar->key, partial, partial_len) == 0) {
                if (set_cmd) {
                    snprintf(completion_buffer, CONSOLE_LINE_LENGTH_MAX, "set %s", cvar->key);
                    return completion_buffer;
                } else if (unset_cmd) {
                    snprintf(completion_buffer, CONSOLE_LINE_LENGTH_MAX, "unset %s", cvar->key);
                    return completion_buffer;
                } else {
                    return cvar->key;
                }
            }
        }
    }

    return NULL;
}

const char* C_CvarErrorToString(cvarstatus_t status)
{
    if (status > CVAR_STATUS_OK && status < CVAR_STATUS_MAX)
        return cvarstatus_error_strings[status];
    else
        return NULL;
}
