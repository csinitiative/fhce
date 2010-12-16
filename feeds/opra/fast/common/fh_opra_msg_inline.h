/*
 * Copyright (C) 2008, 2009, 2010 The Collaborative Software Foundation.
 *
 * This file is part of FeedHandlers (FH).
 *
 * FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FH_OPRA_MSG_INLINE_H__
#define __FH_OPRA_MSG_INLINE_H__

#include "fh_opra_option.h"

/*
 * opra_root_copy
 *
 * Copy the root symbol from the message symbol by stripping trailing spaces.
 */
static inline void opra_root_copy(fh_opra_opt_key_t *k, uint8_t *sym, uint32_t symsize)
{
    char    *ptr = k->k_symbol;
    uint32_t i;

    for (i  = 0; i < symsize; i++) {
        char c = sym[i];

        if (c == ' ') {
            *ptr++ = 0;
            break;
        }

        *ptr++ = c;
    }

    /*
     * Make sure that the option root gets padded with 0
     */
    for (++i; i < sizeof(k->k_symbol); i++) {
        *ptr++ = 0;
    }
}

/*
 * opra_opt_lookup
 *
 * Looks up an option based on the option key.
 */
static inline fh_opra_opt_t *opra_opt_lookup(uint8_t year, uint8_t month, uint8_t day,
                                             char putcall, uint32_t decimal, uint16_t fraction,
                                             char exchid, uint8_t *sym, uint32_t symsize)
{
    fh_opra_opt_t     *opt = NULL;
    fh_opra_opt_key_t  k;
    FH_STATUS          rc;

    /*
     * Prepare the option hash key, as the tuple of root symbol,
     * strike price code, the expiration code, and the participant ID.
     */
    opra_root_copy(&k, sym, symsize);

    k.k_year     = year;
    k.k_month    = month;
    k.k_day      = day;
    k.k_putcall  = putcall;
    k.k_decimal  = decimal;
    k.k_fraction = fraction;
    k.k_exchid   = exchid;

    /*
     * Get the option from the Option H-Table, and create it if it doesn't
     * already exist.
     */
    rc = fh_opra_opt_lookup(&k, &opt);
    if (rc != FH_OK) {
        rc = fh_opra_opt_add(&k, &opt);
        if (rc != FH_OK) {
            return NULL;
        }
    }

    return opt;
}

/*
 * iseprice
 *
 * Mormalise to ise format
 */
static inline int iseprice(int price, char code)
{
    int divc = 0;

    divc = 'D' - code;

    if (divc > 0) {
        while (divc > 0) {
            price *= 10;
            divc--;
        }
    }
    else {
        while (divc < 0) {
            price /= 10;
            divc++;
        }
    }

    return price;
}

/*
 * indexvalue
 *
 * Convert index value.
 */
static inline int indexvalue(uint8_t *str, int len)
{
    /*
     * Maximum code is: 'A' (no decimal after period) + maximum length if
     * period is in the first character, thus (len-1)
     */
    char code  = 'A' + len;
    int  price = 0;
    int  i;

    for (i=0; i<len; i++) {
        char d = str[i];

        if (d != '.') {
            price = price * 10 + (d - '0');
            code--;
        }
    }

    return iseprice(price, code);
}

#endif /* __FH_OPRA_MSG_INLINE_H__ */
