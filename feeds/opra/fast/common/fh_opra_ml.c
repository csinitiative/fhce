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

/*
 * System includes
 */
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

/*
 * FH Common includes
 */
#include "fh_log.h"
#include "fh_plugin.h"
#include "fh_msg.h"

/*
 * OPRA includes
 */
#include "fh_opra_cfg.h"
#include "fh_opra_ml.h"

#define FH_OPRA_DROP_ALL (0)

static fh_plugin_hook_t ml_init     = NULL;
static fh_plugin_hook_t ml_flush    = NULL;
static fh_plugin_hook_t ml_send     = NULL;
static fh_plugin_hook_t ml_opt_add  = NULL;

static fh_msg_sess_t sess = { .sess_init = 0 };

/*
 * fh_opra_ml_init
 *
 * Initialize the messaging layer. Load the hooks if needed.
 */
FH_STATUS fh_opra_ml_init()
{
    FH_STATUS rc;

    FH_ASSERT(sess.sess_init == 0 && ml_init == NULL);

    /*
     * Load the messaging layer hooks if registered
     */
    ml_flush = fh_plugin_get_hook(FH_PLUGIN_MSG_FLUSH);
    if (ml_flush) {
        FH_LOG(MGMT, STATE, ("FH Plugin loaded: OPRA message layer flush"));
    }

    ml_send = fh_plugin_get_hook(FH_PLUGIN_MSG_SEND);
    if (ml_send) {
        FH_LOG(MGMT, STATE, ("FH Plugin loaded: OPRA message layer send"));
    }

    ml_init = fh_plugin_get_hook(FH_PLUGIN_OPRA_MSG_INIT);
    if (ml_init) {
        FH_LOG(MGMT, STATE, ("FH Plugin loaded: OPRA message layer init"));
    }

    ml_opt_add = fh_plugin_get_hook(FH_PLUGIN_OPRA_OPT_ADD);
    if (ml_opt_add) {
        FH_LOG(MGMT, STATE, ("FH Plugin loaded: OPRA message layer option creation"));
    }

    if (ml_init) {
        /*
         * If the messaging layer is coming from a plugin, let's make sure that
         * all the hooks have been defined and successfully loaded
         */
        FH_ASSERT(ml_send);
        FH_ASSERT(ml_flush);
        FH_ASSERT(ml_opt_add);

        ml_init(&rc, &opra_cfg);

        return rc;
    }

    /*
     * Initialize the default messaging layer
     */
    rc = fh_msg_init(&sess);
    if (rc != FH_OK) {
        return rc;
    }

    return FH_OK;
}

/*
 * fh_opra_ml_flush
 *
 * Flush the pending data in the messaging layer.
 */
FH_STATUS fh_opra_ml_flush()
{
    FH_STATUS rc;

    if (ml_flush) {
        ml_flush(&rc);
        return rc;
    }

    return fh_msg_flush(&sess);
}

/*
 * fh_opra_ml_send
 *
 * Send some data to the messaging layer.
 */
FH_STATUS fh_opra_ml_send(void *msg, int length)
{
    FH_STATUS rc = FH_OK;

#if !FH_OPRA_DROP_ALL
    if (ml_send) {
        ml_send(&rc, msg, length);
        return rc;
    }

    rc = fh_msg_send(&sess, msg, length);
#endif
    return rc;
}

/*
 * fh_opra_ml_opt_add
 *
 * Register an option to the messaging layer.
 */
FH_STATUS fh_opra_ml_opt_add(fh_opra_opt_t *opt)
{
    FH_STATUS rc;

    if (ml_opt_add) {
        ml_opt_add(&rc, opt);
        return rc;
    }

    opt->opt_priv = NULL;

    return FH_OK;
}


