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

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* FH common includes */
#include "fh_plugin.h"
#include "fh_errors.h"
#include "fh_alerts.h"

/* FH shared module includes */
#include "fh_shr_cfg_lh.h"
#include "fh_shr_lh.h"
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_lkp_order.h"

/* FH BATS headers */
#include "fh_bats_pitch20_msg.h"

/* macro to do a verbose plugin include */
#define BATS_PLUGIN_REGISTER(x, h)                                                      \
do {                                                                                    \
    printf("Loading BATS hook: %s (%d) hook=%s...", #x, x, #h);                         \
    if (fh_plugin_register((x), (fh_plugin_hook_t)(h)) != FH_OK) {                      \
        fprintf(stderr, "BATS_PLUGIN: Unable to register %s (%d) hook\n", #x, x);       \
        exit(1);                                                                        \
    }                                                                                   \
    printf("done\n");                                                                   \
} while (0)


/*
 * Log fields and structure of a message header
 */
void bats_log_msg_header(fh_bats_msg_header *header)
{
    FH_LOG(CSI, VSTATE, ("fh_bats_msg_header => {  Message Type => 0x%x , seq_no => %lld,"
                         "timestamp => %lld }",
                         header->msg_type, header->seq_no,header->timestamp));
}

/*
 * Function to turn ISE prices into double precision floats (for display)
 */
static inline double dbl_price(uint64_t price)
{
    double result = 0.0;

    /* extract the whole dollar part of the price */
    result += price / 10000;

    /* extract the decimal part of the price */
    result += (double)(price % 10000) / 10000;

    return result;
}


/*
 * Log fields belonging to a symbol table entry
 */
void bats_log_sym_table(fh_shr_lkp_sym_t *entry)
{
    if (entry != NULL) {
        FH_LOG(CSI, VSTATE, ("fh_shr_lkp_sym_t => { symbol => %s }", entry->symbol));
    }
    else {
        FH_LOG(CSI, VSTATE, ("fh_shr_lkp_sym_t => NULL"));
    }
}

/*
 * Log fields belonging to an order table entry
 */
void bats_log_ord_table(fh_shr_lkp_ord_t *entry)
{
    if (entry != NULL) {
        FH_LOG(CSI, VSTATE, ("fh_shr_lkp_ord_t => { order_no => %lu, order_no_str => \"%.12s\", "
                             "price => $%.4f, "
                             "shares => %u, buy_sell_ind => '%c', stock => \"%.6s\" }",
                             entry->order_no, entry->order_no_str,
                             dbl_price(entry->price), entry->shares,
                             entry->buy_sell_ind, entry->stock));
        if (entry->sym_entry != NULL) {
            FH_LOG(CSI, VSTATE, ("    sym_entry => { symbol => %s }", entry->sym_entry->symbol));
        }
        else {
            FH_LOG(CSI, VSTATE, ("    sym_entry => NULL"));
        }
    }
    else {
        FH_LOG(CSI, VSTATE, ("fh_shr_lkp_ord_t => NULL"));
    }
}

/**
 *  @brief Function that is called just after the feed handler configuration has been loaded
 *
 *  @param rc return code to be "returned"
 *  @param config pointer to the configuration that has been loaded by the feed handler
 */
void bats_cfg_load(FH_STATUS *rc, fh_shr_cfg_lh_proc_t *config)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_CFG_LOAD called: configuration at 0x%x", config));
    *rc = FH_OK;
}

/**
 *  @brief Function that is called just after the line handler has been initialized
 *
 *  @param rc return code to be "returned"
 *  @param config pointer to the feed handler's process data structure
 */
void bats_lh_init(FH_STATUS *rc, fh_shr_lh_proc_t *process)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_LH_INIT called: process structure at 0x%x", process));
    *rc = FH_OK;
}


/**
 *  @brief Function that is called for any alert condition that occurs in the feed handler
 *
 *  @param rc return code to be "returned"
 *  @param alert the alert that has been generated
 */
void bats_alert(FH_STATUS *rc, FH_ALERT alert, fh_shr_lh_conn_t * conn)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ALERT:%s(%s)", conn->line->config->name, conn->tag));
    FH_LOG(CSI, VSTATE, ("alert => %d", alert));

    *rc = FH_OK;
}

/**
 *  @brief Send a message (mostly for plugins that plan to publish)
 *
 *  @param rc return code to be "returned"
 *  @param data a pointer to the data being sent
 *  @param length the length of the data being sent
 */
void bats_msg_send(FH_STATUS *rc, void *data, uint32_t length)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_MSG_SEND called: data of length %u at 0x%x", length, data));
    *rc = FH_OK;
}

/**
 *  @brief Called before (potentially) long periods of inactivity to give the plugin a chance to
 *         to flush any message buffers
 *
 *  @param rc return code to be "returned"
 */
void bats_msg_flush(FH_STATUS *rc)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_MSG_FLUSH called"));
    *rc = FH_OK;
}

/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_add_order_long(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                         fh_bats_add_order_long_t *message,
                         void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_ADD_ORDER_LONG:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_add_order_long_t => { order_id => %lld ,"
                         "side => '%c' , shares => %ld, "
                         "stock => \"%.6s\" , price => $%.4f "
                         "add_flags => %d  }", message->order_id,message->side,
                         message->shares, message->stock, dbl_price(message->price),
                         (uint8_t)message->add_flags));


    bats_log_sym_table(message->sym_entry);
    bats_log_ord_table(message->ord_entry);

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_add_order_short(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_bats_add_order_short_t *message,
                          void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_ADD_ORDER_SHORT:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_add_order_short_t => { order_id => %lld ,"
                         "side => '%c' , shares => %ld, "
                         "stock => \"%.6s\" , price => $%.4f "
                         "add_flags => %d  }", message->order_id,message->side,
                         message->shares, message->stock, message->price,
                         message->add_flags));

    bats_log_sym_table(message->sym_entry);
    bats_log_ord_table(message->ord_entry);

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_order_execute(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_bats_order_execute_t *message,
                        void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_ORDER_EXE: %s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_order_execute_t => { order_id => %lld ,"
                         "shares => %ld, execution_id => %lld, }",
                         message->order_id,
                         message->shares, message->exec_id));


    bats_log_ord_table(message->ord_entry);


    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;


}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_order_execute_price(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                              fh_bats_order_execute_price_t *message,
                              void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_EXEC_PRICE:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_order_execute_price_t => { order_id => %lld ,"
                         "shares => %ld, remaining shares => %ld "
                         "execute_id => %lld, price => $%.4f} ",
                         message->order_id, message->shares,
                         message->rem_shares, dbl_price(message->price)));


    bats_log_ord_table(message->ord_entry);


    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_reduce_size_long(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                           fh_bats_reduce_size_long_t *message,
                           void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_REDUCE_SIZE_LONG:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_reduce_size_long_t => { order_id => %lld ,"
                         "shares => %d }",
                         message->order_id, message->shares));


    bats_log_ord_table(message->ord_entry);


    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_reduce_size_short(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                            fh_bats_reduce_size_short_t *message,
                            void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_REDUCE_SIZE_SHORT:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_reduce_size_short_t => { order_id => %lld ,"
                         "shares => %d }",
                         message->order_id, message->shares));


    bats_log_ord_table(message->ord_entry);



    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_modify_order_long(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                            fh_bats_modify_long_t *message,
                            void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_MODIFY_ORDER_LONG:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_modify_long_t => { order_id => %lld ,"
                         "shares => %ld,  price => $%.4f} "
                         "modify_flags => %d }",
                         message->order_id, message->shares,
                         dbl_price(message->price), message->modify_flags));


    bats_log_ord_table(message->ord_entry);


    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_modify_order_short(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                             fh_bats_modify_short_t *message,
                             void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_MODIFY_ORDER_SHORT:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_modify_short_t => { order_id => %lld ,"
                         "shares => %d,  price => $%.4f "
                         "modify_flags => %d }",
                         message->order_id, message->shares,
                         dbl_price(message->price), message->modify_flags));


    bats_log_ord_table(message->ord_entry);


    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}

/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_order_delete(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_bats_delete_order_t *message,
                       void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_ORDER_DELETE:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_delete_order_t => { order_id => %lld }",
                         message->order_id));


     /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_trade_long(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_bats_trade_long_t *message,
                     void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_TRADE_LONG:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_trade_long_t => { order_id => %lld ,"
                         "side => '%c' , shares => %ld, "
                         "stock => \"%.6s\" , price => $%.4f "
                         "execution_id => %lld  }", message->order_id,message->side,
                         message->shares, message->stock, dbl_price(message->price),
                         message->execution_id));


    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_trade_short(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_bats_trade_short_t *message,
                      void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_ADD_ORDER_LONG:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_add_order_long_t => { order_id => %lld ,"
                         "side => '%c' , shares => %ld, "
                         "stock => \"%.6s\" , price => $%.4f "
                         "execution_id => %lld  }", message->order_id,message->side,
                          message->shares, message->stock, dbl_price(message->price),
                         message->execution_id));

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}


/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_trade_break(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_bats_trade_break_t *message,
                      void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_TRADE_BREAK:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_bats_trade_break_t => { execution_id => %lld  }",
                         message->execution_id));


    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}

/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void bats_end_of_session(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                         fh_bats_end_of_session_t *message,
                         void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_BATS_MSG_END_OF_SESSION:%s(%s)", conn->line->config->name, conn->tag));
    bats_log_msg_header(&message->hdr);

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;

}



/**
 *  @brief Startup function
 *  The "__attribute__ ((constructor))" syntax will cause this function to be run whenever the
 *  shared object, compiled from this .c file, is loaded.  It is in this function that all
 *  registration of hook functions must be done.
 */
void __attribute__ ((constructor)) bats_plugin_init(void)
{
    BATS_PLUGIN_REGISTER(FH_PLUGIN_CFG_LOAD,                    bats_cfg_load);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_LH_INIT,                     bats_lh_init);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_ALERT,                       bats_alert);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_MSG_SEND,                    bats_msg_send);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_MSG_FLUSH,                   bats_msg_flush);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_ADD_ORDER_LONG,     bats_add_order_long);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_ADD_ORDER_SHORT,    bats_add_order_short);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_ORDER_EXE,          bats_order_execute);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_ORDER_EXE_PRICE,    bats_order_execute_price);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_REDUCE_SIZE_LONG,   bats_reduce_size_long);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_REDUCE_SIZE_SHORT,  bats_reduce_size_short);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_MODIFY_ORDER_LONG,  bats_modify_order_long);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_MODIFY_ORDER_SHORT, bats_modify_order_short);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_ORDER_DELETE,       bats_order_delete);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_TRADE_LONG,         bats_trade_long);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_TRADE_SHORT,        bats_trade_short);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_TRADE_BREAK,        bats_trade_break);
    BATS_PLUGIN_REGISTER(FH_PLUGIN_BATS_MSG_END_OF_SESSION,     bats_end_of_session);

}
