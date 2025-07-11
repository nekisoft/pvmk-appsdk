/*
 * Copyright (c) 2007 Vreixo Formoso
 * Copyright (c) 2007 Mario Danic
 * Copyright (c) 2009 - 2023 Thomas Schmitt
 * 
 * This file is part of the libisofs project; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License version 2 
 * or later as published by the Free Software Foundation. 
 * See COPYING file for details.
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <string.h>
#include <stdio.h>

#include "rockridge.h"
#include "node.h"
#include "ecma119_tree.h"
#include "writer.h"
#include "messages.h"
#include "image.h"
#include "aaip_0_2.h"
#include "libisofs.h"


#ifdef Libisofs_with_rrip_rR
#define ISO_ROCKRIDGE_IN_DIR_REC 128
#else
#define ISO_ROCKRIDGE_IN_DIR_REC 124
#endif

#define ISO_CE_ENTRY_SIZE  28


static
int susp_add_ES(Ecma119Image *t, struct susp_info *susp, int to_ce, int seqno);

static
int susp_make_CE(Ecma119Image *t, uint8_t **CE,
                 uint32_t block_offset, uint32_t byte_offset, uint32_t size);


static
int susp_append(Ecma119Image *t, struct susp_info *susp, uint8_t *data)
{
    susp->n_susp_fields++;
    susp->susp_fields = realloc(susp->susp_fields, sizeof(void*)
                                * susp->n_susp_fields);
    if (susp->susp_fields == NULL) {
        return ISO_OUT_OF_MEM;
    }
    susp->susp_fields[susp->n_susp_fields - 1] = data;
    susp->suf_len += data[2];
    return ISO_SUCCESS;
}

static
int susp_append_ce(Ecma119Image *t, struct susp_info *susp, uint8_t *data)
{
    int to_alloc = 1, ret;
    unsigned char *pad;
    uint8_t *CE;
    size_t next_alloc;

    if (data[0] &&
        (susp->ce_len + data[2] + ISO_CE_ENTRY_SIZE - 1) / BLOCK_SIZE !=
                                                   susp->ce_len / BLOCK_SIZE) {
        /* Linux fs/isofs wants byte_offset + ce_len <= BLOCK_SIZE.
           So this Continuation Area needs to end by a CE which points
           to the start of the next block.
        */ 
        to_alloc = 2;
        if ((susp->ce_len + ISO_CE_ENTRY_SIZE) % BLOCK_SIZE)
            to_alloc = 3; /* need a PAD pseudo entry */
    }

    if (susp->ce_susp_fields == NULL)
        susp->alloc_ce_susp_fields = 0;
    if (susp->n_ce_susp_fields + to_alloc > susp->alloc_ce_susp_fields) {
        next_alloc = susp->alloc_ce_susp_fields;
        while (susp->n_ce_susp_fields + to_alloc > next_alloc)
            next_alloc += ISO_SUSP_CE_ALLOC_STEP;
        susp->ce_susp_fields = realloc(susp->ce_susp_fields,
                                       sizeof(uint8_t *) * next_alloc);
        if (susp->ce_susp_fields == NULL)
            return ISO_OUT_OF_MEM;
        susp->alloc_ce_susp_fields = next_alloc;
    }

    if (to_alloc >= 2) {
        /* Insert CE entry (actual CE size later by susp_update_CE_sizes) */
        ret = susp_make_CE(t, &CE, (uint32_t) (susp->ce_block +
                                               susp->ce_len / BLOCK_SIZE + 1),
                           (uint32_t) 0, (uint32_t) 0);
        if (ret < 0)
            return ret;
        susp->ce_susp_fields[susp->n_ce_susp_fields] = CE;
        susp->ce_len += ISO_CE_ENTRY_SIZE;
        susp->n_ce_susp_fields++;
    }
    if (to_alloc >= 3) {


#ifdef Libisofs_ce_calc_debuG

        fprintf(stderr,
                  "\nlibburn_DEBUG: Inserting %d bytes of CE padding\n\n",
                  (int) (BLOCK_SIZE - (susp->ce_len % BLOCK_SIZE)));

#endif /* Libisofs_ce_calc_debuG */

        pad = malloc(1);
        if (pad == NULL)
            return ISO_OUT_OF_MEM;
        pad[0] = 0;
        susp->ce_susp_fields[susp->n_ce_susp_fields] = pad;
        if (susp->ce_len % BLOCK_SIZE)
            susp->ce_len += BLOCK_SIZE - (susp->ce_len % BLOCK_SIZE);
        susp->n_ce_susp_fields++;
    }
    susp->ce_susp_fields[susp->n_ce_susp_fields] = data;
    susp->n_ce_susp_fields++;

    if (data[0] == 0) {
        if (susp->ce_len % BLOCK_SIZE)
            susp->ce_len += BLOCK_SIZE - (susp->ce_len % BLOCK_SIZE);
    } else {
        susp->ce_len += data[2];
    }
    return ISO_SUCCESS;
}

static
uid_t px_get_uid(Ecma119Image *t, Ecma119Node *n)
{
    if (t->replace_uid) {
        return t->uid;
    } else {
        return n->node->uid;
    }
}

static
uid_t px_get_gid(Ecma119Image *t, Ecma119Node *n)
{
    if (t->replace_gid) {
        return t->gid;
    } else {
        return n->node->gid;
    }
}

static
mode_t px_get_mode(Ecma119Image *t, Ecma119Node *n)
{
    if ((n->type == ECMA119_DIR || n->type == ECMA119_PLACEHOLDER)) {
        if (t->replace_dir_mode) {
            return (n->node->mode & S_IFMT) | t->dir_mode;
        }
    } else {
        if (t->replace_file_mode) {
            return (n->node->mode & S_IFMT) | t->file_mode;
        }
    }
    return n->node->mode;
}

/**
 * Add a PX System Use Entry. The PX System Use Entry is used to add POSIX 
 * file attributes, such as access permissions or user and group id, to a 
 * ECMA 119 directory record. (RRIP, 4.1.1)
 */
static
int rrip_add_PX(Ecma119Image *t, Ecma119Node *n, struct susp_info *susp)
{
    uint8_t *PX = malloc(44);
    if (PX == NULL) {
        return ISO_OUT_OF_MEM;
    }

    PX[0] = 'P';
    PX[1] = 'X';
    if (t->opts->rrip_1_10_px_ino || !t->opts->rrip_version_1_10 ) {
        PX[2] = 44;
    } else {
        PX[2] = 36;
    }
    PX[3] = 1;
    iso_bb(&PX[4], (uint32_t) px_get_mode(t, n), 4);
    iso_bb(&PX[12], (uint32_t) n->nlink, 4);
    iso_bb(&PX[20], (uint32_t) px_get_uid(t, n), 4);
    iso_bb(&PX[28], (uint32_t) px_get_gid(t, n), 4);
    if (t->opts->rrip_1_10_px_ino || !t->opts->rrip_version_1_10) {
        iso_bb(&PX[36], (uint32_t) n->ino, 4);
    }

    return susp_append(t, susp, PX);
}

/**
 * Add to the given tree node a TF System Use Entry, used to record some
 * time stamps related to the file (RRIP, 4.1.6).
 */
static
int rrip_add_TF(Ecma119Image *t, Ecma119Node *n, struct susp_info *susp)
{
    IsoNode *iso;
    uint8_t *TF = malloc(5 + 3 * 7);
    if (TF == NULL) {
        return ISO_OUT_OF_MEM;
    }

    TF[0] = 'T';
    TF[1] = 'F';
    TF[2] = 5 + 3 * 7;
    TF[3] = 1;
    TF[4] = (1 << 1) | (1 << 2) | (1 << 3);
    
    iso = n->node;
    iso_datetime_7(&TF[5], t->replace_timestamps ? t->timestamp : iso->mtime,
                   t->opts->always_gmt);
    iso_datetime_7(&TF[12], t->replace_timestamps ? t->timestamp : iso->atime,
                   t->opts->always_gmt);
    iso_datetime_7(&TF[19], t->replace_timestamps ? t->timestamp : iso->ctime,
                   t->opts->always_gmt);
    return susp_append(t, susp, TF);
}

/**
 * Add a PL System Use Entry, used to record the location of the original 
 * parent directory of a directory which has been relocated.
 * 
 * This is special because it doesn't modify the susp fields of the directory
 * that gets passed to it; it modifies the susp fields of the ".." entry in
 * that directory.
 * 
 * See RRIP, 4.1.5.2 for more details.
 */
static
int rrip_add_PL(Ecma119Image *t, Ecma119Node *n, struct susp_info *susp)
{
    uint8_t *PL;

    if (n->type != ECMA119_DIR || n->info.dir->real_parent == NULL) {
        /* should never occur */
        return ISO_ASSERT_FAILURE;
    }

    PL = malloc(12);
    if (PL == NULL) {
        return ISO_OUT_OF_MEM;
    }

    PL[0] = 'P';
    PL[1] = 'L';
    PL[2] = 12;
    PL[3] = 1;

    /* write the location of the real parent, already computed */
    iso_bb(&PL[4],
       n->info.dir->real_parent->info.dir->block - t->eff_partition_offset, 4);
    return susp_append(t, susp, PL);
}

/**
 * Add a RE System Use Entry to the given tree node. The purpose of the 
 * this System Use Entry is to indicate to an RRIP-compliant receiving
 * system that the Directory Record in which an "RE" System Use Entry is 
 * recorded has been relocated from another position in the original 
 * Directory Hierarchy.
 * 
 * See RRIP, 4.1.5.3 for more details.
 */
static
int rrip_add_RE(Ecma119Image *t, Ecma119Node *n, struct susp_info *susp)
{
    uint8_t *RE = malloc(4);
    if (RE == NULL) {
        return ISO_OUT_OF_MEM;
    }

    RE[0] = 'R';
    RE[1] = 'E';
    RE[2] = 4;
    RE[3] = 1;
    return susp_append(t, susp, RE);
}

/**
 * Add a PN System Use Entry to the given tree node. 
 * The PN System Use Entry is used to store the device number, and it's
 * mandatory if the tree node corresponds to a character or block device.
 * 
 * See RRIP, 4.1.2 for more details.
 */
static
int rrip_add_PN(Ecma119Image *t, Ecma119Node *n, struct susp_info *susp)
{
    IsoSpecial *node;
    uint8_t *PN;
    int high_shift= 0;

    node = (IsoSpecial*)n->node;
    if (node->node.type != LIBISO_SPECIAL) {
        /* should never occur */
        return ISO_ASSERT_FAILURE;
    }

    PN = malloc(20);
    if (PN == NULL) {
        return ISO_OUT_OF_MEM;
    }

    PN[0] = 'P';
    PN[1] = 'N';
    PN[2] = 20;
    PN[3] = 1;

    /* (dev_t >> 32) causes compiler warnings on FreeBSD.
       RRIP 1.10 4.1.2 prescribes PN "Dev_t High" to be 0 on 32 bit dev_t.
    */
    if (sizeof(node->dev) > 4) {
        high_shift = 32;
        iso_bb(&PN[4], (uint32_t) (node->dev >> high_shift), 4);
    } else
        iso_bb(&PN[4], 0, 4);
    iso_bb(&PN[12], (uint32_t) (node->dev & 0xffffffff), 4);
    return susp_append(t, susp, PN);
}

/**
 * Add to the given tree node a CL System Use Entry, that is used to record 
 * the new location of a directory which has been relocated.
 * 
 * See RRIP, 4.1.5.1 for more details.
 */
static
int rrip_add_CL(Ecma119Image *t, Ecma119Node *n, struct susp_info *susp)
{
    uint8_t *CL;
    if (n->type != ECMA119_PLACEHOLDER) {
        /* should never occur */
        return ISO_ASSERT_FAILURE;
    }
    CL = malloc(12);
    if (CL == NULL) {
        return ISO_OUT_OF_MEM;
    }

    CL[0] = 'C';
    CL[1] = 'L';
    CL[2] = 12;
    CL[3] = 1;
    iso_bb(&CL[4], n->info.real_me->info.dir->block - t->eff_partition_offset,
           4);
    return susp_append(t, susp, CL);
}

/**
 * Convert a RR filename to the requested charset. On any conversion error, 
 * the original name will be used.
 * @param flag   bit0= do not issue error messages
 */
int iso_get_rr_name(IsoWriteOpts *opts, char *input_charset,
                    char *output_charset, int imgid,
                    char *str, char **name, int flag)
{
    int ret;

    if (!strcmp(input_charset, output_charset)) {
        /* no conversion needed */
        ret = iso_clone_mem(str, name, 0);
        return ret;
    }

    ret = strconv(str, input_charset, output_charset, name);
    if (ret < 0) {
        /* TODO we should check for possible cancellation */
        if (!(flag & 1))
            iso_msg_submit(imgid, ISO_FILENAME_WRONG_CHARSET, ret,
                   "Charset conversion error. Cannot convert %s from %s to %s",
                   str, input_charset, output_charset);
        *name = NULL;
        return ISO_FILENAME_WRONG_CHARSET;
    }

    return ISO_SUCCESS;
}

static
char *get_rr_fname(Ecma119Image *t, char *str)
{
    int ret;
    char *name = NULL;

    ret = iso_get_rr_name(t->opts, t->input_charset, t->output_charset,
                          t->image->id, str, &name, 0);
    if (ret < 0)
        return NULL;
    return name;
}

/**
 * Add a NM System Use Entry to the given tree node. The purpose of this
 * System Use Entry is to store the content of an Alternate Name to support 
 * POSIX-style or other names. 
 * 
 * See RRIP, 4.1.4 for more details.
 * 
 * @param size
 *     Length of the name to be included into the NM
 * @param flags
 * @param ce
 *     Whether to add or not to CE
 */
static
int rrip_add_NM(Ecma119Image *t, struct susp_info *susp, char *name, int size,
                int flags, int ce)
{
    uint8_t *NM;

    if (size > 250)
        return ISO_ASSERT_FAILURE;

    NM = malloc(size + 5);
    if (NM == NULL) {
        return ISO_OUT_OF_MEM;
    }

    NM[0] = 'N';
    NM[1] = 'M';
    NM[2] = size + 5;
    NM[3] = 1;
    NM[4] = flags;
    if (size) {
        memcpy(&NM[5], name, size);
    }
    if (ce) {
        return susp_append_ce(t, susp, NM);
    } else {
        return susp_append(t, susp, NM);
    }
}

/**
 * Add a new SL component (RRIP, 4.1.3.1) to a list of components.
 * 
 * @param n
 *     Number of components. It will be updated.
 * @param compos
 *     Pointer to the list of components.
 * @param s
 *     The component content
 * @param size
 *     Size of the component content
 * @param fl
 *     Flags
 * @return
 *     1 on success, < 0 on error
 */
static
int rrip_SL_append_comp(size_t *n, uint8_t ***comps, char *s, int size, char fl)
{
    uint8_t *comp = malloc(size + 2);
    if (comp == NULL) {
        return ISO_OUT_OF_MEM;
    }

    (*n)++;
    comp[0] = fl;
    comp[1] = size;
    *comps = realloc(*comps, (*n) * sizeof(void*));
    if (*comps == NULL) {
        free(comp);
        return ISO_OUT_OF_MEM;
    }
    (*comps)[(*n) - 1] = comp;

    if (size) {
        memcpy(&comp[2], s, size);
    }
    return ISO_SUCCESS;
}


#ifdef Libisofs_with_rrip_rR

/**
 * Add a RR System Use Entry to the given tree node. This is an obsolete 
 * entry from before RRIP-1.10. Nevertheless mkisofs produces it. There
 * is the suspicion that some operating systems could take it as indication
 * for Rock Ridge.
 *
 * The meaning of the payload byte is documented e.g. in
 *  /usr/src/linux/fs/isofs/rock.h 
 * It announces the presence of entries PX, PN, SL, NM, CL, PL, RE, TF
 * by payload byte bits 0 to 7.
 */
static
int rrip_add_RR(Ecma119Image *t, Ecma119Node *n, struct susp_info *susp)
{
    uint8_t *RR;
    RR = malloc(5);
    if (RR == NULL) {
        return ISO_OUT_OF_MEM;
    }

    RR[0] = 'R';
    RR[1] = 'R';
    RR[2] = 5;
    RR[3] = 1;

    /* <<< ts B20307 : Not all directories have NM, many files have more entries */
    RR[4] = 0x89; /* TF, NM , PX */

    /* >>> ts B20307 : find out whether n carries 
           PX, PN, SL, NM, CL, PL, RE, TF and mark by bit0 to bit7 in RR[4]
    */

    return susp_append(t, susp, RR);
}

#endif /* Libisofs_with_rrip_rR */


/**
 * Add a SL System Use Entry to the given tree node. This is used to store 
 * the content of a symbolic link, and is mandatory if the tree node
 * indicates a symbolic link (RRIP, 4.1.3).
 * 
 * @param comp
 *     Components of the SL System Use Entry. If they don't fit in a single
 *     SL, more than one SL will be added.
 * @param n
 *     Number of components in comp
 * @param ce
 *     Whether to add to a continuation area or system use field.
 */
static
int rrip_add_SL(Ecma119Image *t, struct susp_info *susp, uint8_t **comp,
                size_t n, int ce)
{
    int ret;
    size_t i, j;
    int total_comp_len = 0;
    size_t pos, written = 0;

    uint8_t *SL;

    for (i = 0; i < n; i++) {

        total_comp_len += comp[i][1] + 2;
        if (total_comp_len > 250) {
            /* we need a new SL entry */
            total_comp_len -= comp[i][1] + 2;
            SL = malloc(total_comp_len + 5);
            if (SL == NULL) {
                return ISO_OUT_OF_MEM;
            }

            SL[0] = 'S';
            SL[1] = 'L';
            SL[2] = total_comp_len + 5;
            SL[3] = 1;
            SL[4] = 1; /* CONTINUE */
            pos = 5;
            for (j = written; j < i; j++) {
                memcpy(&SL[pos], comp[j], comp[j][1] + 2);
                pos += comp[j][1] + 2;
            }

            /*
             * In this case we are sure we're writing to CE. Check for
             * debug purposes
             */
            if (ce == 0) {
                free(SL);
                return ISO_ASSERT_FAILURE;
            }
            ret = susp_append_ce(t, susp, SL);
            if (ret < 0) {
                free(SL);
                return ret;
            }
            SL = NULL; /* now owned by susp */
            written = i;
            total_comp_len = comp[i][1] + 2;
        }
    }

    SL = malloc(total_comp_len + 5);
    if (SL == NULL) {
        return ISO_OUT_OF_MEM;
    }

    SL[0] = 'S';
    SL[1] = 'L';
    SL[2] = total_comp_len + 5;
    SL[3] = 1;
    SL[4] = 0;
    pos = 5;

    for (j = written; j < n; j++) {
        memcpy(&SL[pos], comp[j], comp[j][1] + 2);
        pos += comp[j][1] + 2;
    }
    if (ce) {
        ret = susp_append_ce(t, susp, SL);
    } else {
        ret = susp_append(t, susp, SL);
    }
    return ret;
}


/* @param flag bit1= care about crossing block boundaries */
static
int susp_calc_add_to_ce(Ecma119Image *t, size_t *ce, size_t base_ce, int add,
                        int flag)
{
    if (flag & 2) {
        /* Account for inserted CE before size exceeds block size */
        if ((*ce + base_ce + add + ISO_CE_ENTRY_SIZE - 1) / BLOCK_SIZE !=
            (*ce + base_ce) / BLOCK_SIZE) {
            /* Insert CE and padding */
            t->curr_ce_entries++;
            *ce += ISO_CE_ENTRY_SIZE;
            if ((*ce + base_ce) % BLOCK_SIZE)
                *ce += BLOCK_SIZE - ((*ce + base_ce) % BLOCK_SIZE);
        }
    }
    *ce += add;
    return ISO_SUCCESS;
}


/*
   @param flag bit0= only account sizes in sua_free resp. ce_len.
                     Parameter susp may be NULL in this case
               bit1= account for crossing block boundaries
                     (implied by bit0 == 0)
   @param ce_len counts the freshly added CA size of the current node
   @param ce_mem tells the CA size of previous nodes in the same directory
*/
static
int aaip_add_AL(Ecma119Image *t, struct susp_info *susp,
                uint8_t **data, size_t num_data,
                size_t *sua_free, size_t *ce_len, size_t ce_mem, int flag)
{
    int ret, done = 0, len, es_extra = 0;
    uint8_t *aapt, *cpt;
    size_t count = 0;

    if (!(flag & 1))
        flag |= 2;
    if (!t->opts->aaip_susp_1_10)
        es_extra = 5;
    if (*sua_free < num_data + es_extra || *ce_len > 0) {
        if (es_extra > 0)
            susp_calc_add_to_ce(t, ce_len, ce_mem, es_extra, flag & 2);
        done = 0;
        for (aapt = *data; !done; aapt += aapt[2]) {
            done = !(aapt[4] & 1);
            len = aapt[2];
            susp_calc_add_to_ce(t, ce_len, ce_mem, len, flag & 2);
            count += len;
        }
    } else {
        *sua_free -= num_data + es_extra;
    }
    if (flag & 1)
        return ISO_SUCCESS;

    /* If AAIP enabled and announced by ER : Write ES field to announce AAIP */
    if (t->opts->aaip && !t->opts->aaip_susp_1_10) {
        ret = susp_add_ES(t, susp, (*ce_len > 0), 1);
        if (ret < 0)
            return ret;
    }
    aapt = *data;
    if (!(aapt[4] & 1)) {
        /* Single field can be handed over directly */
        if (*ce_len > 0) {
            ret = susp_append_ce(t, susp, aapt);
        } else {
            ret = susp_append(t, susp, aapt);
        }
        *data = NULL;
        return ISO_SUCCESS;
    }

    /* Multiple fields have to be handed over as single field copies */
    done = 0;
    for (aapt = *data; !done; aapt += aapt[2]) {
        done = !(aapt[4] & 1);
        len = aapt[2];
        cpt = calloc(aapt[2], 1);
        if (cpt == NULL)
            return ISO_OUT_OF_MEM;
        memcpy(cpt, aapt, len);
        if (*ce_len > 0) {
            ret = susp_append_ce(t, susp, cpt);
        } else {
            ret = susp_append(t, susp, cpt);
        }
        if (ret < 0)
            return ret;
    }
    free(*data);
    *data = NULL;
    return ISO_SUCCESS;
}


/**
 * Add a SUSP "ER" System Use Entry to identify the Rock Ridge specification.
 * 
 * The "ER" System Use Entry is used to uniquely identify a specification
 * compliant with SUSP. This method adds to the given tree node "." entry
 * the "ER" corresponding to the RR protocol.
 * 
 * See SUSP, 5.5 and RRIP, 4.3 for more details.
 */
static
int rrip_add_ER(Ecma119Image *t, struct susp_info *susp)
{
    unsigned char *ER;

    if (!t->opts->rrip_version_1_10) {
        /*
        According to RRIP 1.12 this is the future form:
        4.3 "Specification of the ER System Use Entry Values for RRIP"
        talks of "IEEE_P1282" in each of the three strings and finally states
        "Note: Upon adoption as an IEEE standard, these lengths will each
         decrease by 1."
        So "IEEE_P1282" would be the new form, "RRIP_1991A" is the old form.
        */

        ER = malloc(182);
        if (ER == NULL) {
            return ISO_OUT_OF_MEM;
        }
    
        ER[0] = 'E';
        ER[1] = 'R';
        ER[2] = 182;
        ER[3] = 1;
        ER[4] = 9;
        ER[5] = 72;
        ER[6] = 93;
        ER[7] = 1;
        memcpy(&ER[8], "IEEE_1282", 9);
        memcpy(&ER[17], "THE IEEE 1282 PROTOCOL PROVIDES SUPPORT FOR POSIX "
            "FILE SYSTEM SEMANTICS.", 72);
        memcpy(&ER[89], "PLEASE CONTACT THE IEEE STANDARDS DEPARTMENT, "
            "PISCATAWAY, NJ, USA FOR THE 1282 SPECIFICATION.", 93);
    } else {
        /*
        RRIP 1.09 and 1.10: 
        4.3 Specification of the ER System Use Field Values for RRIP
        The Extension Version number for the version of the RRIP defined herein
        shall be 1. The content of the Extension Identifier field shall be
        "RRIP_1991A". The Identifier Length shall be 10. The recommended
        content of the Extension Descriptor shall be "THE ROCK RIDGE
        INTERCHANGE PROTOCOL PROVIDES SUPPORT FOR POSIX FILE SYSTEM SEMANTICS."
        The corresponding Description Length is 84.
        The recommended content of the Extension Source shall be "PLEASE
        CONTACT DISC PUBLISHER FOR SPECIFICATION SOURCE.  SEE PUBLISHER
        IDENTIFIER IN PRIMARY VOLUME DESCRIPTOR FOR CONTACT INFORMATION."
        The corresponding Source Length is 135.
        */

        ER = malloc(237);
        if (ER == NULL) {
            return ISO_OUT_OF_MEM;
        }

        ER[0] = 'E';
        ER[1] = 'R';
        ER[2] = 237;
        ER[3] = 1;
        ER[4] = 10;
        ER[5] = 84;
        ER[6] = 135;
        ER[7] = 1;

        memcpy(&ER[8], "RRIP_1991A", 10);
        memcpy(&ER[18], "THE ROCK RIDGE INTERCHANGE PROTOCOL PROVIDES SUPPORT FOR POSIX FILE SYSTEM SEMANTICS", 84);
        memcpy(&ER[102], "PLEASE CONTACT DISC PUBLISHER FOR SPECIFICATION SOURCE.  SEE PUBLISHER IDENTIFIER IN PRIMARY VOLUME DESCRIPTOR FOR CONTACT INFORMATION.", 135);
    }

    /** This always goes to continuation area */
    return susp_append_ce(t, susp, ER);
}


static
int aaip_add_ER(Ecma119Image *t, struct susp_info *susp, int flag)
{
    unsigned char *ER;

    ER = malloc(160);
    if (ER == NULL) {
        return ISO_OUT_OF_MEM;
    }
    
    ER[0] = 'E';
    ER[1] = 'R';
    ER[2] = 160;
    ER[3] = 1;
    ER[4] = 9;
    ER[5] = 81;
    ER[6] = 62;
    ER[7] = 1;
    memcpy(ER + 8, "AAIP_0200", 9);
    memcpy(ER + 17,
           "AL PROVIDES VIA AAIP 2.0 SUPPORT FOR ARBITRARY FILE ATTRIBUTES"
           " IN ISO 9660 IMAGES", 81);
    memcpy(ER + 98,
           "PLEASE CONTACT THE LIBBURNIA PROJECT VIA LIBBURNIA-PROJECT.ORG",
           62);

    /** This always goes to continuation area */
    return susp_append_ce(t, susp, ER);
}


/**
 * Create the byte representation of a CE entry.
 * (SUSP, 5.1).
 */
static
int susp_make_CE(Ecma119Image *t, uint8_t **CE,
                 uint32_t block_offset, uint32_t byte_offset, uint32_t size)
{
    uint8_t *data;

    *CE = NULL;
    data = calloc(1, 28);
    if (data == NULL)
        return ISO_OUT_OF_MEM;
    *CE = data;

    data[0] = 'C';
    data[1] = 'E';
    data[2] = 28;
    data[3] = 1;

    iso_bb(&data[4], block_offset - t->eff_partition_offset, 4);
    iso_bb(&data[12], byte_offset, 4);
    iso_bb(&data[20], size, 4);

    return ISO_SUCCESS;
}


/**
 * Add a CE System Use Entry to the given tree node. A "CE" is used to add
 * a continuation area, where additional System Use Entry can be written.
 * (SUSP, 5.1).
 */
static
int susp_add_CE(Ecma119Image *t, size_t ce_len, struct susp_info *susp)
{
    uint32_t block_offset, byte_offset;
    uint8_t *CE;
    int ret;

    /* Linux fs/isofs wants byte_offset + ce_len <= BLOCK_SIZE
     * Here the byte_offset is reduced to the minimum.
     */
    block_offset = susp->ce_block + susp->ce_len / BLOCK_SIZE;
    byte_offset = susp->ce_len % BLOCK_SIZE;
    ret = susp_make_CE(t, &CE, block_offset, byte_offset, (uint32_t) ce_len);
    if (ret < 0)
        return ret;
    return susp_append(t, susp, CE);
}

/**
 * Add a SP System Use Entry. The SP provide an identifier that the SUSP is 
 * used within the volume. The SP shall be recorded in the "." entry of the 
 * root directory. See SUSP, 5.3 for more details.
 */
static
int susp_add_SP(Ecma119Image *t, struct susp_info *susp)
{
    unsigned char *SP = malloc(7);
    if (SP == NULL) {
        return ISO_OUT_OF_MEM;
    }

    SP[0] = 'S';
    SP[1] = 'P';
    SP[2] = (char)7;
    SP[3] = (char)1;
    SP[4] = 0xbe;
    SP[5] = 0xef;
    SP[6] = 0;
    return susp_append(t, susp, SP);
}


/**
 * SUSP 1.12: [...] shall specify as an 8-bit number the Extension
 * Sequence Number of the extension specification utilized in the entries
 * immediately following this System Use Entry. The Extension Sequence
 * Numbers of multiple extension specifications on a volume shall correspond to
 * the order in which their "ER" System Use Entries are recorded [...]
 */
static
int susp_add_ES(Ecma119Image *t, struct susp_info *susp, int to_ce, int seqno)
{
    unsigned char *ES = malloc(5);

    if (ES == NULL) {
        return ISO_OUT_OF_MEM;
    }
    ES[0] = 'E';
    ES[1] = 'S';
    ES[2] = (unsigned char) 5;
    ES[3] = (unsigned char) 1;
    ES[4] = (unsigned char) seqno;
    if (to_ce) {
        return susp_append_ce(t, susp, ES);
    } else {
        return susp_append(t, susp, ES);
    }
}


/**
 * see doc/zisofs_format.txt : "ZF System Use Entry Format"
 * see doc/zisofs2_format.txt : "ZF System Use Entry Format", "Z2 ..."
 */
static
int zisofs_add_ZF(Ecma119Image *t, struct susp_info *susp, int to_ce,
                  uint8_t algo[2], int header_size_div4, int block_size_log2,
                  uint64_t uncompressed_size, int flag)
{
    unsigned char *ZF = malloc(16);

    /* Intimate friendship with this variable in filters/zisofs.c */
    extern int iso_zisofs2_enable_susp_z2;

    if (ZF == NULL) {
        return ISO_OUT_OF_MEM;
    }
    ZF[0] = 'Z';
    ZF[1] = 'F';
    ZF[2] = (unsigned char) 16;
    if (algo[0] == 'p' && algo[1] == 'z') {
        ZF[3] = (unsigned char) 1;
    } else {
        ZF[3] = (unsigned char) 2;
        if (iso_zisofs2_enable_susp_z2)
            ZF[1] = '2';
    }
    ZF[4] = (unsigned char) algo[0];
    ZF[5] = (unsigned char) algo[1];
    ZF[6] = (unsigned char) header_size_div4;
    ZF[7] = (unsigned char) block_size_log2;
    if (algo[0] == 'p' && algo[1] == 'z') {
        if (uncompressed_size > (uint64_t) 0xffffffff)
            return ISO_ZISOFS_TOO_LARGE;
        iso_bb(&ZF[8], (uint32_t) uncompressed_size, 4);
    } else {
        iso_lsb64(&ZF[8], uncompressed_size);
    }
    if (to_ce) {
        return susp_append_ce(t, susp, ZF);
    } else {
        return susp_append(t, susp, ZF);
    }
}


/* @param flag bit0= Do not add data but only count sua_free and ce_len
               bit1= account for crossing block boundaries
                     (implied by bit0 == 0)
*/
static
int add_zf_field(Ecma119Image *t, Ecma119Node *n, struct susp_info *info,
                 size_t *sua_free, size_t *ce_len, size_t base_ce, int flag)
{
    int ret, will_copy = 1, stream_type = 0, do_zf = 0;
    int header_size_div4 = 0, block_size_log2 = 0;
    uint64_t uncompressed_size = 0;
    IsoStream *stream = NULL, *input_stream, *last_stream, *first_stream;
    IsoStream *first_filter = NULL;
    IsoFile *file;
    void *xipt;
    struct zisofs_zf_info *zf;
    uint8_t algo[2];

    /* Intimate friendship with this function in filters/zisofs.c */
    int ziso_is_zisofs_stream(IsoStream *stream, int *stream_type,
                              uint8_t zisofs_algo[2],
                              int *header_size_div4, int *block_size_log2,
                              uint64_t *uncompressed_size, int flag);

    if (!(flag & 1))
        flag |= 2;

    if (iso_node_get_type(n->node) != LIBISO_FILE)
        return 2;
    file = (IsoFile *) n->node;

    /* Inspect: last_stream < ... < first_filter < first_stream */
    /* The content is in zisofs format if:
       It gets copied and
           the last stream is a ziso stream,
           or it had a ZF entry and is unfiltered
           or it has a zf xinfo record (because its last stream delivered a
                                        zisofs file header when inquired)
       or it stays uncopied and
           the first filter is an osiz stream,
           or it had a ZF entry
           or it has a zf xinfo record (because its first stream delivered a 
                                        zisofs file header when inquired)
    */

    if (t->opts->appendable && file->from_old_session) 
        will_copy = 0;

    first_filter = first_stream = last_stream = iso_file_get_stream(file);
    while (1) {
        input_stream = iso_stream_get_input_stream(first_stream, 0);
        if (input_stream == NULL)
    break;
        first_filter = first_stream;
        first_stream = input_stream;
    }
    if (will_copy) {
        stream = last_stream;
    } else {
        /* (the eventual osiz filter on the image stream) */
        stream = first_filter;
    }

    /* Determine stream type : 1=ziso , -1=osiz , 0=other */
    algo[0] = algo[1] = 0;
    ret = ziso_is_zisofs_stream(stream, &stream_type, algo, &header_size_div4,
                                &block_size_log2, &uncompressed_size, 0);
    if (ret < 0)
        return ret;

    if (stream_type == 1 && will_copy) {
        do_zf = 1;
    } else if (stream_type == -1 && !will_copy) {
        do_zf = 1;
    } else if(first_stream == last_stream || !will_copy) {
        /* Try whether the image side stream remembers a ZF field */
        ret = iso_stream_get_src_zf(first_stream, algo, &header_size_div4,
                                    &block_size_log2, &uncompressed_size, 0);
        if (ret == 1 && header_size_div4 > 0)
            do_zf = 1;
    }
    if (!do_zf) {
        /* Look for an xinfo mark of a zisofs header */
        ret = iso_node_get_xinfo((IsoNode *) file, zisofs_zf_xinfo_func,
                                 &xipt);
        if (ret == 1) {
            zf = xipt;
            header_size_div4 = zf->header_size_div4;
            block_size_log2 = zf->block_size_log2;
            uncompressed_size = zf->uncompressed_size;
            algo[0] = zf->zisofs_algo[0];
            algo[1] = zf->zisofs_algo[1];
            if (header_size_div4 > 0)
                do_zf = 1;
        }
    }

    if (!do_zf)
        return 2;

    /* Account for field size */
    if (*sua_free < 16 || *ce_len > 0) {
        susp_calc_add_to_ce(t, ce_len, base_ce, 16, flag & 2);
    } else {
        *sua_free -= 16;
    }
    if (flag & 1)
        return 1;

    /* write ZF field */
    ret = zisofs_add_ZF(t, info, (*ce_len > 0), algo, header_size_div4,
                       block_size_log2, uncompressed_size, 0);
    if (ret < 0)
        return ret;
    return 1;
}

/* API */
int aaip_xinfo_func(void *data, int flag)
{
    if (flag & 1) {
        free(data);
    }
    return 1;
}

/* API */
int aaip_xinfo_cloner(void *old_data, void **new_data, int flag)
{
    size_t aa_size;

    *new_data = NULL;
    if (old_data == NULL)
        return 0;
    aa_size = aaip_count_bytes((unsigned char *) old_data, 0);
    if (aa_size <= 0)
        return ISO_AAIP_BAD_AASTRING;
    *new_data = calloc(1, aa_size);
    if (*new_data == NULL)
        return ISO_OUT_OF_MEM;
    memcpy(*new_data, old_data, aa_size);
    return (int) aa_size;
}

/**
 * Compute SUA length and eventual Continuation Area length of field NM and
 * eventually fields SL and AL. Because CA usage makes necessary the use of
 * a CE entry of 28 bytes in SUA, this computation fails if not the 28 bytes
 * are taken into account at start. In this case the caller should retry with
 * bit0 set. 
 * If the resulting *ce added to base_ce is in a different block than base_ce,
 * then computation with bit0 fails and the caller should finally try bit1.
 * 
 * @param flag      bit0= assume CA usage (else return 0 on SUA overflow)
 *                  bit1= let CA start at block start (else return 0 if
 *                        *ce crosses a block boundary)
 * @return          1= ok, computation of *su_size and *ce is valid
 *                  0= not ok, CA usage is necessary but bit0 was not set
 *                             or *ce crosses boundary and bit1 was not set
 *                     (*su_size and *ce stay unaltered in this case)
 *                 <0= error:
 *                 -1= not enough SUA space for 28 bytes of CE entry
 *                 -2= out of memory
 *                 (int) ISO_TOO_MANY_CE
 */
static
int susp_calc_nm_sl_al(Ecma119Image *t, Ecma119Node *n, size_t space,
                       size_t *su_size, size_t *ce, size_t base_ce, int flag)
{
    char *name;
    size_t namelen, su_mem, ce_mem;
    void *xipt;
    size_t num_aapt = 0, sua_free = 0;
    int ret;
    uint8_t *aapt;
    uint32_t curr_ce_entries_mem;

#ifdef Libisofs_ce_calc_debug_extrA

    if (n->node->name != NULL)
        fprintf(stderr, "libburn_DEBUG: susp_calc_nm_sl_al : %u %.f %s \n",
                        (unsigned int) t->curr_ce_entries,
                        (double) base_ce, n->node->name);

#endif /* Libisofs_ce_calc_debug_extrA */

    su_mem = *su_size;
    ce_mem = *ce;
    curr_ce_entries_mem = t->curr_ce_entries;
    if (*ce > 0 && !(flag & 1))
        goto unannounced_ca;

    if (flag & 2)
        flag |= 1;

    namelen = 0;
    name = get_rr_fname(t, n->node->name);
    if (name != NULL) {
        namelen = strlen(name);
        free(name);
    }

    if (flag & 1) {
        /* Account for 28 bytes of CE field */
        if (*su_size + 28 > space)
            return -1;
        *su_size += 28;
        t->curr_ce_entries++;
    }

    /* NM entry */
    if (*su_size + 5 + namelen <= space) {
        /* ok, it fits in System Use Area */
        *su_size += 5 + namelen;
    } else {
        /* the NM will be divided in a CA */
        if (!(flag & 1))
            goto unannounced_ca;
        namelen = namelen - (space - *su_size - 5);

        /* >>> SUPER_LONG_RR: Need to handle CA part lengths > 250 
               (This cannot happen with name lengths <= 256, as a part
                of the name will always fit into the directory entry.)
        */;

        susp_calc_add_to_ce(t, ce, base_ce, 5 + namelen, flag & 2);
        *su_size = space;
    }
    if (n->type == ECMA119_SYMLINK) {
        /* 
         * for symlinks, we also need to write the SL
         */
        char *dest, *cur, *prev;
        size_t sl_len = 5;
        int cew = (*ce != 0); /* are we writing to CA ? */

        dest = get_rr_fname(t, ((IsoSymlink*)n->node)->dest);
        if (dest == NULL)
            return -2;
        prev = dest;
        cur = strchr(prev, '/');
        while (1) {
            size_t clen;
            if (cur) {
                clen = cur - prev;
            } else {
                /* last component */
                clen = strlen(prev);
            }

            if (clen == 1 && prev[0] == '.') {
                clen = 0;
            } else if (clen == 2 && prev[0] == '.' && prev[1] == '.') {
                clen = 0;
            }

            /* flags and len for each component record (RRIP, 4.1.3.1) */
            clen += 2;

            if (!cew) {
                /* we are still writing to the SUA */
                if (*su_size + sl_len + clen > space) {
                    /* 
                     * ok, we need a Continuation Area anyway
                     * TODO this can be handled better, but for now SL
                     * will be completely moved into the CA
                     */
                    if (!(flag & 1)) {
                        free(dest);
                        goto unannounced_ca;
                    }
                    cew = 1;
                } else {
                    sl_len += clen;
                }
            }
            if (cew) {
                if (sl_len + clen > 255) {
                    /* we need an additional SL entry */
                    if (clen > 250) {
                        /* 
                         * case 1, component too large to fit in a 
                         * single SL entry. Thus, the component need
                         * to be divided anyway.
                         * Note than clen can be up to 255 + 2 = 257.
                         * 
                         * First, we check how many bytes fit in current
                         * SL field 
                         */
                        ssize_t fit = 255 - sl_len - 2;
                        if ((ssize_t) (clen - 250) <= fit) {
                            /* 
                             * the component can be divided between this
                             * and another SL entry
                             */
                            /* Will fill up old SL and write it */
                            susp_calc_add_to_ce(t, ce, base_ce, 255, flag & 2);
                            sl_len = 5 + (clen - fit); /* Start new SL */
                        } else {
                            /*
                             * the component will need a 2rd SL entry in
                             * any case, so we prefer to don't write 
                             * anything in this SL
                             */
                            /* Will write non-full old SL */
                            susp_calc_add_to_ce(t, ce, base_ce, sl_len,
                                                flag & 2);
                            /* Will write another full SL */
                            susp_calc_add_to_ce(t, ce, base_ce, 255, flag & 2);
                            sl_len = 5 + (clen - 250) + 2; /* Start new SL */
                        }
                    } else {
                        /* case 2, create a new SL entry */
                        /* Will write non-full old SL */
                        susp_calc_add_to_ce(t, ce, base_ce, sl_len, flag & 2);
                        sl_len = 5 + clen; /* Start new SL */
                    }
                } else {
                    sl_len += clen;
                }
            }

            if (!cur || cur[1] == '\0') {
                /* cur[1] can be \0 if dest ends with '/' */
                break;
            }
            prev = cur + 1;
            cur = strchr(prev, '/');
        }

        free(dest);

        /* and finally write the pending SL field */
        if (!cew) {
            /* the whole SL fits into the SUA */
            *su_size += sl_len;
        } else {
            susp_calc_add_to_ce(t, ce, base_ce, sl_len, flag & 2);
        }

    }

    /* Find out whether ZF is to be added and account for its bytes */
    sua_free = space - *su_size;
    add_zf_field(t, n, NULL, &sua_free, ce, base_ce, 1 | (flag & 2));
    *su_size = space - sua_free;
    if (*ce > 0 && !(flag & 1))
        goto unannounced_ca;

    /* obtain num_aapt from node */
    xipt = NULL;
    num_aapt = 0;
    if (t->opts->aaip) {
        ret = iso_node_get_xinfo(n->node, aaip_xinfo_func, &xipt);
        if (ret == 1) {
            num_aapt = aaip_count_bytes((unsigned char *) xipt, 0);
        }
    }
    /* let the expert decide where to add num_aapt */
    if (num_aapt > 0) {
        sua_free = space - *su_size;
        aapt = (uint8_t *) xipt;
        aaip_add_AL(t, NULL, &aapt, num_aapt, &sua_free, ce, base_ce,
                    1 | (flag & 2));
        *su_size = space - sua_free;
        if (*ce > 0 && !(flag & 1))
            goto unannounced_ca;
    }

#ifdef Libisofs_ce_calc_debug_filetraP

    if (n->node->name != NULL)
        if (strcmp(n->node->name, "...insert.leaf.name.here...") == 0)
            fprintf(stderr,
                    "libburn_DEBUG: filename breakpoint susp_calc_nm_sl_al\n");

#endif /* Libisofs_ce_calc_debug_filetraP */

    if (*ce > 0 && !(flag & 2)) {
        if (base_ce / BLOCK_SIZE !=
            (base_ce + *ce + ISO_CE_ENTRY_SIZE - 1) / BLOCK_SIZE) {

#ifdef Libisofs_ce_calc_debuG

            fprintf(stderr,
    "\nlibburn_DEBUG: Crossed block boundary: %.f (%lu) -> %.f (%lu) : %s\n\n",
                    (double) base_ce, (unsigned long) (base_ce / BLOCK_SIZE),
                    (double) (base_ce + *ce - 1),
                    (unsigned long)
                        ((base_ce + *ce + ISO_CE_ENTRY_SIZE - 1) / BLOCK_SIZE),
                     n->node->name);

#endif /* Libisofs_ce_calc_debuG */

            /* Crossed a block boundary */
            *su_size = su_mem;
            *ce = ce_mem;
            t->curr_ce_entries = curr_ce_entries_mem;
            return 0;
        }
    }

    return 1;

unannounced_ca:;
    *su_size = su_mem;
    *ce = ce_mem;
    t->curr_ce_entries = curr_ce_entries_mem;
    return 0;
}


/* @param flag bit0= Do not add data but only count sua_free and ce_len
                     param info may be NULL in this case
               bit1= account for crossing block boundaries
                     (implied by bit0 == 0)
*/
static
int add_aa_string(Ecma119Image *t, Ecma119Node *n, struct susp_info *info,
                  size_t *sua_free, size_t *ce_len, size_t base_ce, int flag)
{
    int ret;
    uint8_t *aapt;
    void *xipt;
    size_t num_aapt= 0;

    if (!t->opts->aaip)
        return 1;

    ret = iso_node_get_xinfo(n->node, aaip_xinfo_func, &xipt);
    if (ret == 1) {
        num_aapt = aaip_count_bytes((unsigned char *) xipt, 0);
        if (num_aapt > 0) {
            if (flag & 1) {
                aapt = (unsigned char *) xipt;
                ret = aaip_add_AL(t, NULL, &aapt, num_aapt, sua_free, ce_len,
                                  base_ce, flag & (1 | 2));
            } else {
                aapt = malloc(num_aapt);
                if (aapt == NULL)
                    return ISO_OUT_OF_MEM;
                memcpy(aapt, xipt, num_aapt);
                ret = aaip_add_AL(t, info, &aapt, num_aapt, sua_free, ce_len,
                                  base_ce, 0);
                /* aapt is NULL now and the memory is owned by t */
            }
            if (ret < 0) 
                return ret;
        }
    }
    return 1;
}


static
void iso_msg_too_many_ce(Ecma119Image *t, Ecma119Node *n, int err)
{
    if (n->node->name != NULL) {
        iso_msg_submit(t->image->id, err, 0,
                       "Too many CE entries for file with name: %s",
                       n->node->name);
    } else {
        iso_msg_submit(t->image->id, err, 0,
                       "Too many CE entries for a single file",
                       n->node->name);
    }
}


/**
 * Compute the length needed for write all RR and SUSP entries for a given
 * node.
 * 
 * @param type
 *      0 normal entry, 1 "." entry for that node (it is a dir), 2 ".."
 *      for that node (i.e., it will refer to the parent)
 * @param used_up
 *      Already occupied space in the directory record.
 * @param ce
 *      Will be filled with the space needed in a CE
 * @param base_ce
 *      Predicted fill of continuation area by previous nodes of same dir
 * @return
 *      The size needed for the RR entries in the System Use Area
 */
ssize_t rrip_calc_len(Ecma119Image *t, Ecma119Node *n, int type, size_t used_up,
                      size_t *ce, size_t base_ce)
{
    size_t su_size, space;
    int ret, retry = 0;
    size_t aaip_sua_free= 0, aaip_len= 0;

try_again:

    /* Directory record length must be even (ECMA-119, 9.1.13). Maximum is 254.
    */
    space = 254 - used_up - (used_up % 2);
    if (type < 0 || type > 2 || space < ISO_ROCKRIDGE_IN_DIR_REC) {
        iso_msg_submit(t->image->id, ISO_ASSERT_FAILURE, 0,
          "Unknown node type %d or short RR space %d < %d in directory record",
          type, (int) space, ISO_ROCKRIDGE_IN_DIR_REC);
        return ISO_ASSERT_FAILURE;
    }

    *ce = 0;
    su_size = 0;
    t->curr_ce_entries = 0;

    /* If AAIP enabled and announced by ER : account for 5 bytes of ES */;
    if (t->opts->aaip && !t->opts->aaip_susp_1_10)
        su_size += 5;

#ifdef Libisofs_with_rrip_rR
    /* obsolete RR field (once in RRIP-1.09) */
    su_size += 5;
#endif

    /* PX and TF, we are sure they always fit in SUA */
    if (t->opts->rrip_1_10_px_ino || !t->opts->rrip_version_1_10) {
        su_size += 44 + 26;
    } else {
        su_size += 36 + 26;
    }

    if (n->type == ECMA119_DIR) {
        if (n->info.dir->real_parent != NULL) {
            /* it is a reallocated entry */
            if (type == 2) {
                /* we need to add a PL entry */
                su_size += 12;
            } else if (type == 0) {
                /* we need to add a RE entry */
                su_size += 4;
            }
        } else if(ecma119_is_dedicated_reloc_dir(t, n) &&
                  (t->opts->rr_reloc_flags & 1)) {
            /* The dedicated relocation directory shall be marked by RE */
            su_size += 4;
        }
    } else if (n->type == ECMA119_SPECIAL) {
        if (S_ISBLK(n->node->mode) || S_ISCHR(n->node->mode)) {
            /* block or char device, we need a PN entry */
            su_size += 20;
        }
    } else if (n->type == ECMA119_PLACEHOLDER) {
        /* we need the CL entry */
        su_size += 12;
    }

    if (type == 0) {

        /* Try without CE */
        ret = susp_calc_nm_sl_al(t, n, space, &su_size, ce, base_ce, 0);
        if (ret == 0) /* Retry with CE but no block crossing */
            ret = susp_calc_nm_sl_al(t, n, space, &su_size, ce, base_ce, 1);
        if (ret == 0) /* Retry with aligned CE and block hopping */
            ret = susp_calc_nm_sl_al(t, n, space, &su_size, ce, base_ce,
                                     1 | 2);
        if (ret == -2)
           return ISO_OUT_OF_MEM;
        /* -1 should not occur. By tradition it would not cause return */
        if (ret < -2) {
            if (n->node->name != NULL)
                iso_msg_submit(t->image->id, ret, 0,
                               "SUSP planning failed for file with name: %s",
                               n->node->name);
            return ret;
        }

    } else {

        /* "." or ".." entry */

        if (!t->opts->rrip_version_1_10)
            su_size += 5; /* NM field */

        if (type == 1 && n->parent == NULL) {
            /* 
             * "." for root directory 
             * we need to write SP and ER entries. The first fits in SUA,
             * ER needs a Continuation Area, thus we also need a CE entry
             */
            su_size += 7 + 28; /* SP + CE */
            t->curr_ce_entries++;
            /* ER of RRIP */
            if (t->opts->rrip_version_1_10) {
                *ce = 237;
            } else {
                *ce = 182;
            }
            if (t->opts->aaip && !t->opts->aaip_susp_1_10) {
                *ce += 160; /* ER of AAIP */
            }
            /* Compute length of AAIP string of root node.
               Will write all AIIP to CA, which already starts at
               block boundary. So no need for three tries.
             */
            aaip_sua_free= 0;
            ret = add_aa_string(t, n, NULL, &aaip_sua_free, &aaip_len, base_ce,
                                1 | 2);
            if (ret < 0)
               return ret;
            *ce += aaip_len;
        }
    }

    if (t->curr_ce_entries > t->opts->max_ce_entries) {
        /* If permitted by API setting: Remove non-isofs-non-ACL fattr */
        retry++;
        if (retry == 1) {
            if ((t->opts->max_ce_drop_attr & 15) >= 1) {
                ret = iso_node_remove_fattr(n->node, 0);
                if (ret > 0) {
                    iso_msg_too_many_ce(t, n, ISO_CE_REMOVING_ATTR);
                    iso_msg_submit(t->image->id, ISO_CE_REMOVING_ATTR, 0,
                                   "Removed non-isofs attributes");
                    goto try_again;
                }
            }
        } else if (retry == 2) {
            if ((t->opts->max_ce_drop_attr & 15) >= 2) {
                ret = iso_node_remove_fattr(n->node, 1);
                if (ret > 0) {
                    iso_msg_submit(t->image->id, ISO_CE_REMOVING_ATTR, 0,
                                   "Removed ACL");
                    goto try_again;
                }
            }
        }
        iso_msg_too_many_ce(t, n, ISO_TOO_MANY_CE);
        return (ssize_t) (int) ISO_TOO_MANY_CE;
    } else if (t->curr_ce_entries >= 32) {
        if (n->node->name != NULL)
            iso_msg_submit(t->image->id, ISO_TOO_MANY_CE_FOR_LINUX, 0,
                           "SUSP planning risky for file with name: %s",
                           n->node->name);
        iso_msg_submit(t->image->id, ISO_TOO_MANY_CE_FOR_LINUX, 0,
                  "Too many CE entries for single file when mounted by Linux");
    }

    /*
     * The System Use field inside the directory record must be padded if
     * it is an odd number (ECMA-119, 9.1.13)
     */
    su_size += (su_size % 2);
    return su_size;
}

/**
 * Free all info in a struct susp_info.
 */
static
void susp_info_free(struct susp_info* susp)
{
    size_t i;

    for (i = 0; i < susp->n_susp_fields; ++i) {
        free(susp->susp_fields[i]);
    }
    free(susp->susp_fields);

    for (i = 0; i < susp->n_ce_susp_fields; ++i) {
        free(susp->ce_susp_fields[i]);
    }
    free(susp->ce_susp_fields);
}


/**
 * Fill a struct susp_info with the RR/SUSP entries needed for a given
 * node.
 * 
 * @param type
 *      0 normal entry, 1 "." entry for that node (it is a dir), 2 ".."
 *      for that node (i.e., it will refer to the parent)
 * @param used_up
 *      Already occupied space in the directory record.
 * @param info
 *      Pointer to the struct susp_info where the entries will be stored.
 *      If some entries need to go to a Continuation Area, they will be added
 *      to the existing ce_susp_fields, and ce_len will be incremented
 *      properly. Please ensure ce_block is initialized properly.
 * @return
 *      1 success, < 0 error
 */
int rrip_get_susp_fields(Ecma119Image *t, Ecma119Node *n, int type,
                         size_t used_up, struct susp_info *info)
{
    int ret;
    size_t i;
    Ecma119Node *node;
    char *name = NULL;
    char *dest = NULL;
    size_t aaip_er_len= 0;
    size_t rrip_er_len= 182;
    size_t su_size_pd, ce_len_pd; /* predicted sizes of SUA and CA */
    int ce_is_predicted = 0;
    size_t aaip_sua_free= 0, aaip_len= 0, ce_mem;
    int space;

    if (t == NULL || n == NULL || info == NULL) {
        return ISO_NULL_POINTER;
    }

    /* Directory record length must be even (ECMA-119, 9.1.13). Maximum is 254.
    */
    space = 254 - used_up - (used_up % 2);
    if (type < 0 || type > 2 || space < ISO_ROCKRIDGE_IN_DIR_REC) {
        iso_msg_submit(t->image->id, ISO_ASSERT_FAILURE, 0,
          "Unknown node type %d or short RR space %d < %d in directory record",
          type, space, ISO_ROCKRIDGE_IN_DIR_REC);
        return ISO_ASSERT_FAILURE;
    }

    /* Mark start index of node's continuation area for later update */
    info->current_ce_start = info->n_ce_susp_fields;
    ce_mem = info->ce_len;

#ifdef Libisofs_ce_calc_debug_filetraP

    if (n->node->name != NULL)
        if (strcmp(n->node->name, "...put.leafname.here...") == 0)
            fprintf(stderr, "libburn_DEBUG: filename breakpoint\n");

#endif /* Libisofs_ce_calc_debug_filetraP */

    if (type == 2 && n->parent != NULL) {
        node = n->parent;
    } else {
        node = n;
    }

    /* 
     * SP must be the first entry for the "." record of the root directory
     * (SUSP, 5.3)
     */
    if (type == 1 && n->parent == NULL) {
        ret = susp_add_SP(t, info);
        if (ret < 0) {
            goto add_susp_cleanup;
        }
    }

    /* If AAIP enabled and announced by ER : Announce RRIP by ES */
    if (t->opts->aaip && !t->opts->aaip_susp_1_10) {
        ret = susp_add_ES(t, info, 0, 0);
        if (ret < 0)
            goto add_susp_cleanup;
    }

#ifdef Libisofs_with_rrip_rR
    ret = rrip_add_RR(t, node, info);
    if (ret < 0) {
        goto add_susp_cleanup;
    }
#endif /* Libisofs_with_rrip_rR */

    /* PX and TF, we are sure they always fit in SUA */
    ret = rrip_add_PX(t, node, info);
    if (ret < 0) {
        goto add_susp_cleanup;
    }
    ret = rrip_add_TF(t, node, info);
    if (ret < 0) {
        goto add_susp_cleanup;
    }

    if (n->type == ECMA119_DIR) {
        if (n->info.dir->real_parent != NULL) {
            /* it is a reallocated entry */
            if (type == 2) {
                /* 
                 * we need to add a PL entry
                 * Note that we pass "n" as parameter, not "node" 
                 */
                ret = rrip_add_PL(t, n, info);
                if (ret < 0) {
                    goto add_susp_cleanup;
                }
            } else if (type == 0) {
                /* we need to add a RE entry */
                ret = rrip_add_RE(t, node, info);
                if (ret < 0) {
                    goto add_susp_cleanup;
                }
            }
        } else if(ecma119_is_dedicated_reloc_dir(t, n) &&
                  (t->opts->rr_reloc_flags & 1)) {
            /* The dedicated relocation directory shall be marked by RE */
            ret = rrip_add_RE(t, node, info);
            if (ret < 0)
                goto add_susp_cleanup;
        }
    } else if (n->type == ECMA119_SPECIAL) {
        if (S_ISBLK(n->node->mode) || S_ISCHR(n->node->mode)) {
            /* block or char device, we need a PN entry */
            ret = rrip_add_PN(t, node, info);
            if (ret < 0) {
                goto add_susp_cleanup;
            }
        }
    } else if (n->type == ECMA119_PLACEHOLDER) {
        /* we need the CL entry */
        ret = rrip_add_CL(t, node, info);
        if (ret < 0) {
            goto add_susp_cleanup;
        }
    }

    if (info->suf_len + 28 > space) {
        iso_msg_submit(t->image->id, ISO_ASSERT_FAILURE, 0,
         "Directory Record overflow. name='%s' , suf_len=%d > space=%d - 28\n", 
         node->iso_name, (int) info->suf_len, space);
        return ISO_ASSERT_FAILURE;
    }

    if (type == 0) {
        size_t sua_free; /* free space in the SUA */
        int nm_type = 0; /* 0 whole entry in SUA, 1 part in CE */
        size_t ce_len = 0; /* len of the CE */
        size_t namelen;

        /* this two are only defined for symlinks */
        uint8_t **comps= NULL; /* components of the SL field */
        size_t n_comp = 0; /* number of components */

        namelen = 0;
        name = get_rr_fname(t, n->node->name);
        if (name == NULL)
            name = strdup("");
        if (name == NULL) {
            ret = ISO_OUT_OF_MEM;
            goto add_susp_cleanup;
        }
        namelen = strlen(name);
        sua_free = space - info->suf_len;

        /* Try whether NM, SL, AL will fit into SUA */
        su_size_pd = info->suf_len;
        ce_len_pd = ce_len;
        ret = susp_calc_nm_sl_al(t, n, (size_t) space,
                                 &su_size_pd, &ce_len_pd, info->ce_len, 0);
        if (ret == 0) { /* Have to use CA. 28 bytes of CE are necessary */
            ret = susp_calc_nm_sl_al(t, n, (size_t) space,
                                     &su_size_pd, &ce_len_pd, info->ce_len, 1);
            if (ret == 0) /* Retry with aligned CE and block hopping */
                ret = susp_calc_nm_sl_al(t, n, (size_t) space,
                                         &su_size_pd, &ce_len_pd, info->ce_len,
                                         1 | 2);
            sua_free -= 28;
            ce_is_predicted = 1;
        }
        if (ret == -2) {
           ret = ISO_OUT_OF_MEM;
           goto add_susp_cleanup;
        }
        /* -1 should not occur. By tradition it would not cause return */
        if (ret < -2)
            goto add_susp_cleanup;

        /* NM entry */
        if (5 + namelen <= sua_free) {
            /* ok, it fits in System Use Area */
            sua_free -= (5 + namelen);
            nm_type = 0;
        } else {
            /* the NM will be divided in a CE */
            nm_type = 1;
            namelen = namelen - (sua_free - 5);
            ce_len = 5 + namelen;
            sua_free = 0;
        }
        if (n->type == ECMA119_SYMLINK) {
            /* 
             * for symlinks, we also need to write the SL
             */
            char *cur, *prev;
            size_t sl_len = 5;
            int cew = (nm_type == 1); /* are we writing to CE? */

            dest = get_rr_fname(t, ((IsoSymlink*)n->node)->dest);
            if (dest == NULL)
                dest = strdup("");
            if (dest == NULL) {
                ret = ISO_OUT_OF_MEM;
                goto add_susp_cleanup;
            }

            prev = dest;
            cur = strchr(prev, '/');
            while (1) {
                size_t clen;
                char cflag = 0; /* component flag (RRIP, 4.1.3.1) */
                if (cur) {
                    clen = cur - prev;
                } else {
                    /* last component */
                    clen = strlen(prev);
                }

                if (clen == 0) {
                    /* this refers to the roor directory, '/' */
                    cflag = 1 << 3;
                }
                if (clen == 1 && prev[0] == '.') {
                    clen = 0;
                    cflag = 1 << 1;
                } else if (clen == 2 && prev[0] == '.' && prev[1] == '.') {
                    clen = 0;
                    cflag = 1 << 2;
                }

                /* flags and len for each component record (RRIP, 4.1.3.1) */
                clen += 2;

                if (!cew) {
                    /* we are still writing to the SUA */
                    if (sl_len + clen > sua_free) {
                        /* 
                         * ok, we need a Continuation Area anyway
                         * TODO this can be handled better, but for now SL
                         * will be completely moved into the CA
                         */

                        /* sua_free, ce_len, nm_type already account for CE */
                        cew = 1;

                    } else {
                        /* add the component */
                        ret = rrip_SL_append_comp(&n_comp, &comps, prev, 
                                                  clen - 2, cflag);
                        if (ret < 0) {
                            goto add_susp_cleanup;
                        }
                        sl_len += clen;
                    }
                }
                if (cew) {
                    if (sl_len + clen > 255) {
                        /* we need an addition SL entry */
                        if (clen > 250) {
                            /* 
                             * case 1, component too large to fit in a 
                             * single SL entry. Thus, the component need
                             * to be divided anyway.
                             * Note than clen can be up to 255 + 2 = 257.
                             * 
                             * First, we check how many bytes fit in current
                             * SL field 
                             */
                            ssize_t fit = 255 - sl_len - 2;
                            if ((ssize_t) (clen - 250) <= fit) {
                                /* 
                                 * the component can be divided between this
                                 * and another SL entry
                                 */
                                ret = rrip_SL_append_comp(&n_comp, &comps,
                                                          prev, fit, 0x01);
                                if (ret < 0) {
                                    goto add_susp_cleanup;
                                }
                                /* 
                                 * and another component, that will go in 
                                 * other SL entry
                                 */
                                ret = rrip_SL_append_comp(&n_comp, &comps, prev
                                        + fit, clen - fit - 2, 0);
                                if (ret < 0) {
                                    goto add_susp_cleanup;
                                }
                                ce_len += 255; /* this SL, full */
                                sl_len = 5 + (clen - fit);
                            } else {
                                /*
                                 * the component will need a 2rd SL entry in
                                 * any case, so we prefer to don't write 
                                 * anything in this SL
                                 */
                                ret = rrip_SL_append_comp(&n_comp, &comps,
                                                          prev, 248, 0x01);
                                if (ret < 0) {
                                    goto add_susp_cleanup;
                                }
                                ret = rrip_SL_append_comp(&n_comp, &comps, prev
                                        + 248, strlen(prev + 248), 0x00);
                                if (ret < 0) {
                                    goto add_susp_cleanup;
                                }
                                ce_len += sl_len + 255;
                                sl_len = 5 + (clen - 250) + 2;
                            }
                        } else {
                            /* case 2, create a new SL entry */
                            ret = rrip_SL_append_comp(&n_comp, &comps, prev,
                                                      clen - 2, cflag);
                            if (ret < 0) {
                                goto add_susp_cleanup;
                            }
                            ce_len += sl_len;
                            sl_len = 5 + clen;
                        }
                    } else {
                        /* the component fit in the SL entry */
                        ret = rrip_SL_append_comp(&n_comp, &comps, prev, 
                                                  clen - 2, cflag);
                        if (ret < 0) {
                            goto add_susp_cleanup;
                        }
                        sl_len += clen;
                    }
                }

                if (!cur || cur[1] == '\0') {
                    /* cur[1] can be \0 if dest ends with '/' */
                    break;
                }
                prev = cur + 1;
                cur = strchr(prev, '/');
            }

            if (cew) {
                ce_len += sl_len;
            }
        }

        /*
         * We we reach here:
         * - We know if NM fill in the SUA (nm_type == 0)
         * - If SL needs an to be written in CE (ce_len > 0)
         * - The components for SL entry (or entries)
         */

        if (nm_type == 0) {
            /* the full NM fills in SUA */
            ret = rrip_add_NM(t, info, name, strlen(name), 0, 0);
            if (ret < 0) {
                goto add_susp_cleanup;
            }
        } else {
            /* 
             * Write the NM part that fits in SUA...  Note that CE
             * entry and NM in the continuation area is added below 
             */

            namelen = space - info->suf_len - 28 * (!!ce_is_predicted) - 5;
            ret = rrip_add_NM(t, info, name, namelen, 1, 0);
            if (ret < 0) {
                goto add_susp_cleanup;
            }
        }

        if (ce_is_predicted) {
            /* Add the CE entry */
            ret = susp_add_CE(t, ce_len_pd, info);
            if (ret < 0) {
                goto add_susp_cleanup;
            }
        }

        if (nm_type == 1) {
            /* 
             * ..and the part that goes to continuation area.
             */

            /* >>> SUPER_LONG_RR : Need a loop to handle CA lengths > 250
                   (This cannot happen with name lengths <= 256, as a part
                    of the name will always fit into the directory entry.)
             */;

            ret = rrip_add_NM(t, info, name + namelen, strlen(name + namelen),
                              0, 1);
            if (ret < 0) {
                goto add_susp_cleanup;
            }
        }

        if (n->type == ECMA119_SYMLINK) {

            /* add the SL entry (or entries) */
            ret = rrip_add_SL(t, info, comps, n_comp, (ce_len > 0));

            /* free the components */
            for (i = 0; i < n_comp; i++) {
                free(comps[i]);
            }
            free(comps);

            if (ret < 0) {
                goto add_susp_cleanup;
            }
        }

        /* Eventually write zisofs ZF field */
        ret = add_zf_field(t, n, info, &sua_free, &ce_len, ce_mem, 0);
        if (ret < 0)
            goto add_susp_cleanup;

        /* Eventually obtain AAIP field string from node
           and write it to directory entry or CE area.
        */
        ret = add_aa_string(t, n, info, &sua_free, &ce_len, ce_mem, 0);
        if (ret < 0)
            goto add_susp_cleanup;


    } else {

        /* "." or ".." entry */

        /* write the NM entry */
        if (t->opts->rrip_version_1_10) {
            /* RRIP-1.10:
               "NM" System Use Fields recorded for the ISO 9660 directory
                records with names (00) and (01), used to designate the
                current and parent directories, respectively, should be
                ignored. Instead, the receiving system should convert these
                names to the appropriate receiving system-dependent
                designations for the current and parent directories.
            */
            /* mkisofs obviously writes no NM for '.' and '..' .
               Program isoinfo shows empty names with records as of RRIP-1.12
            */
            /* no op */;
        } else {
            /* RRIP-1.12:
               If the ISO 9660 Directory Record File Identifier is (00), then
               the CURRENT bit of the "NM" Flags field [...], if present, shall
               be set to ONE. If the ISO 9660 Directory Record File Identifier
               is (01), then the PARENT bit of the "NM" Flags field [...],
               if present, shall be set to ONE.
               [...]
               "BP 3 - Length (LEN_NM)" shall specify as an 8-bit number the
               length in bytes [...]. If bit position 1, 2, or 5 of the "NM"
               Flags is set to ONE, the value of this field shall be 5 and no
               Name Content shall be recorded.
               [The CURRENT bit has position 1. The PARENT bit has position 2.]
            */
            ret = rrip_add_NM(t, info, NULL, 0, 1 << type, 0);
            if (ret < 0)
                goto add_susp_cleanup;
        }

        if (type == 1 && n->parent == NULL) {
            /* 
             * "." for root directory 
             * we need to write SP and ER entries. The first fits in SUA,
             * ER needs a Continuation Area, thus we also need a CE entry.
             * Note that SP entry was already added above
             */

            if (t->opts->rrip_version_1_10) {
                rrip_er_len = 237;
            } else {
                rrip_er_len = 182;
            }
            if (t->opts->aaip && !t->opts->aaip_susp_1_10) {
                aaip_er_len = 160;
            }

            /* Compute length of AAIP string of root node */
            aaip_sua_free= 0;
            /* (just to give t->curr_ce_entries a defined state) */
            t->curr_ce_entries = 0;
            ret = add_aa_string(t, n, NULL, &aaip_sua_free, &aaip_len, ce_mem,
                                1 | 2);
            if (ret < 0)
                goto add_susp_cleanup;

            /* Allocate the necessary CE space */
            ret = susp_add_CE(t, rrip_er_len + aaip_er_len + aaip_len, info);
            if (ret < 0) {
                goto add_susp_cleanup;
            }
            ret = rrip_add_ER(t, info);
            if (ret < 0) {
                goto add_susp_cleanup;
            }
            if (t->opts->aaip && !t->opts->aaip_susp_1_10) {
                ret = aaip_add_ER(t, info, 0);
                if (ret < 0) {
                    goto add_susp_cleanup;
                }
            }
            /* Write AAIP string of root node */
            aaip_sua_free= aaip_len= 0;
            ret = add_aa_string(t, n, info, &aaip_sua_free, &aaip_len, ce_mem,
                                0);
            if (ret < 0)
                goto add_susp_cleanup;

        }
    }


    /*
     * The System Use field inside the directory record must be padded if
     * it is an odd number (ECMA-119, 9.1.13)
     */
    info->suf_len += (info->suf_len % 2);

    free(name);
    free(dest);
    return ISO_SUCCESS;

    add_susp_cleanup: ;
    if (name != NULL)
        free(name);
    if (dest != NULL)
        free(dest);
    susp_info_free(info);
    return ret;
}


/* Update the sizes of CE fields at end of info->susp_fields and in
   single node range of info->ce_susp_fields.
*/
static
int susp_update_CE_sizes(Ecma119Image *t, struct susp_info *info, int flag)
{
    size_t i, curr_pos;
    uint8_t *curr_ce;
    uint32_t size;

    if (info->n_susp_fields == 0 ||
        info->n_ce_susp_fields - info->current_ce_start == 0)
        return ISO_SUCCESS;

    for (i = 0; i < info->n_susp_fields; i++)
        if (info->susp_fields[i][0] == 'C')
            if(info->susp_fields[i][1] == 'E')
    break;
    if (i >= info->n_susp_fields) {
        iso_msg_submit(t->image->id, ISO_ASSERT_FAILURE, 0,
                       "System Use Area field contains no CE, but there are fields in Continuation Area");
        return ISO_ASSERT_FAILURE;
    }
    curr_ce = info->susp_fields[i];
    curr_pos = 0;
    for (i = info->current_ce_start; i < info->n_ce_susp_fields; i++) {
       if (info->ce_susp_fields[i][0] == 0) {
           /* ignore pseudo SUSP PAD */
    continue;
       }
       if (info->ce_susp_fields[i][0] == 'C' &&
           info->ce_susp_fields[i][1] == 'E') {
          size = (curr_pos + info->ce_susp_fields[i][2]) % BLOCK_SIZE;
          if (size == 0)
              size = BLOCK_SIZE;
          iso_bb(curr_ce + 20, size, 4);
          curr_ce = info->ce_susp_fields[i];
          /* Start a new CE Area */
          curr_pos = 0;
    continue;
       }
       curr_pos = (curr_pos + info->ce_susp_fields[i][2]) % 2048;
    }
    if (curr_pos > 0) {
        size = curr_pos;
        if (size > BLOCK_SIZE) {
            /* Should never happen */
            iso_msg_submit(t->image->id, ISO_WRONG_RR_WARN, 0,
                      "Encountered and truncated oversized Continuation Area");
            size = BLOCK_SIZE;
        }
        iso_bb(curr_ce + 20, size, 4);
    }
    return ISO_SUCCESS;
}


/**
 * Write the given SUSP fields into buf. Note that Continuation Area
 * fields are not written.
 * If info does not contain any SUSP entry this function just return. 
 * After written, the info susp_fields array will be freed, and the counters
 * updated properly.
 */
void rrip_write_susp_fields(Ecma119Image *t, struct susp_info *info,
                            uint8_t *buf)
{
    size_t i;
    size_t pos = 0;
    int ret;

    if (info->n_susp_fields == 0) {
        return;
    }

    ret = susp_update_CE_sizes(t, info, 0);
    if (ret < 0)
        return;

    for (i = 0; i < info->n_susp_fields; i++) {
        memcpy(buf + pos, info->susp_fields[i], info->susp_fields[i][2]);
        pos += info->susp_fields[i][2];
    }

    /* free susp_fields */
    for (i = 0; i < info->n_susp_fields; ++i) {
        free(info->susp_fields[i]);
    }
    free(info->susp_fields);
    info->susp_fields = NULL;
    info->n_susp_fields = 0;
    info->suf_len = 0;
}

/**
 * Write the Continuation Area entries for the given struct susp_info, using
 * the iso_write() function.
 * After written, the ce_susp_fields array will be freed.
 */
int rrip_write_ce_fields(Ecma119Image *t, struct susp_info *info)
{
    size_t i;
    uint8_t *padding = NULL;
    int ret= ISO_SUCCESS;
    uint64_t written = 0, pad_size;

    if (info->n_ce_susp_fields == 0) {
        goto ex;
    }
    LIBISO_ALLOC_MEM(padding, uint8_t, BLOCK_SIZE);

    for (i = 0; i < info->n_ce_susp_fields; i++) {
        if (info->ce_susp_fields[i][0] == 0) {
            /* Pseudo field: pad up to next block boundary */
            pad_size = BLOCK_SIZE - (written % BLOCK_SIZE);
            if (pad_size == BLOCK_SIZE)
    continue;
            memset(padding, 0, pad_size);
            ret = iso_write(t, padding, pad_size);
            if (ret < 0)
                goto write_ce_field_cleanup;
            written += pad_size;
    continue;
        }
        ret = iso_write(t, info->ce_susp_fields[i], 
                        info->ce_susp_fields[i][2]);
        if (ret < 0) {
            goto write_ce_field_cleanup;
        }
        written += info->ce_susp_fields[i][2];
    }

    /* pad continuation area until block size */
    i = BLOCK_SIZE - (info->ce_len % BLOCK_SIZE);
    if (i > 0 && i < BLOCK_SIZE) {
        memset(padding, 0, i);
        ret = iso_write(t, padding, i);
        if (ret < 0)
            goto write_ce_field_cleanup;
        written += i;
    }

    write_ce_field_cleanup: ;
    /* free ce_susp_fields */
    for (i = 0; i < info->n_ce_susp_fields; ++i) {
        free(info->ce_susp_fields[i]);
    }
    free(info->ce_susp_fields);
    info->ce_susp_fields = NULL;
    info->n_ce_susp_fields = 0;
    info->alloc_ce_susp_fields = 0;
    info->ce_len = 0;
ex:;
    LIBISO_FREE_MEM(padding);
    return ret;
}

