/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 * 
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FH_PLUGIN_H__
#define __FH_PLUGIN_H__

/* system headers */
#include <stdbool.h>

/* FH common headers */
#include "fh_errors.h"


/* general hook functions */
#define FH_PLUGIN_LOG_MSG                       (0)
                                                
/* OPRA hook functions */                       
#define FH_PLUGIN_MSG_SEND                      (1)
#define FH_PLUGIN_MSG_FLUSH                     (2)
#define FH_PLUGIN_OPRA_MIN                      (3)
#define FH_PLUGIN_OPRA_MSG_INIT                 (3)
#define FH_PLUGIN_OPRA_CTRL_PACK                (4)
#define FH_PLUGIN_OPRA_OI_PACK                  (5)
#define FH_PLUGIN_OPRA_UV_LS_PACK               (6)
#define FH_PLUGIN_OPRA_UV_BO_PACK               (7)
#define FH_PLUGIN_OPRA_LS_PACK                  (8)
#define FH_PLUGIN_OPRA_EOD_PACK                 (9)
#define FH_PLUGIN_OPRA_QUOTE_PACK               (10)
#define FH_PLUGIN_OPRA_QUOTE_BO_PACK            (11)
#define FH_PLUGIN_OPRA_QUOTE_BB_PACK            (12)
#define FH_PLUGIN_OPRA_QUOTE_BBO_PACK           (13)
#define FH_PLUGIN_OPRA_OPT_ADD                  (14)
#define FH_PLUGIN_OPRA_CFG_LOAD                 (15)
                                                
/* management layer hook functions */           
#define FH_PLUGIN_MGMT_INIT                     (16)
#define FH_PLUGIN_FHMGR_CFG_LOAD                (17)
#define FH_PLUGIN_FHMGR_STATS_REPORT            (18)
                                                
/* OPRA alert related hook functions */         
#define FH_PLUGIN_OPRA_FTLINE_EVENT             (19)
#define FH_PLUGIN_MGMT_INIT_COMPLETE            (20)
                                                
/* Arca hook functions */                       
#define FH_PLUGIN_ARCA_MIN                      (21)
#define FH_PLUGIN_ARCA_MSG_INIT                 (21)
#define FH_PLUGIN_ARCA_INIT                     (22)
#define FH_PLUGIN_ARCA_SHUTDOWN                 (23)
#define FH_PLUGIN_SYMBOL_CLEAR                  (24)
#define FH_PLUGIN_MESSAGE_UNAVAILABLE           (25)
#define FH_PLUGIN_SEQUENCE_RESET                (26)
#define FH_PLUGIN_BOOK_REFRESH                  (27)
#define FH_PLUGIN_IMBALANCE_REFRESH             (28)
#define FH_PLUGIN_SYMBOL_MAPPING                (29)
#define FH_PLUGIN_FIRM_MAPPING                  (30)
#define FH_PLUGIN_ADD_ORDER                     (31)
#define FH_PLUGIN_MOD_ORDER                     (32)
#define FH_PLUGIN_DEL_ORDER                     (33)
#define FH_PLUGIN_IMBALANCE                     (34)
#define FH_PLUGIN_PACKET_LOSS                   (35)
#define FH_PLUGIN_FEED_ALERT                    (36)

/* Arca reference (order, symbol, firm, etc.) hook functions */
#define FH_PLUGIN_REFERENCE_MIN                 (37)
#define FH_PLUGIN_BUILD_SYM_TABLE               (37)
#define FH_PLUGIN_FREE_SYM_TABLE                (38)
#define FH_PLUGIN_BUILD_FIRM_TABLE              (39)
#define FH_PLUGIN_FREE_FIRM_TABLE               (40)
#define FH_PLUGIN_BUILD_ORDER_TABLE             (41)
#define FH_PLUGIN_FREE_ORDER_TABLE              (42)
#define FH_PLUGIN_CONFIG_TABLE                  (43)
#define FH_PLUGIN_ADD_SYMBOL                    (44)
#define FH_PLUGIN_ADD_FIRM                      (45)
#define FH_PLUGIN_ADD_ORDER_REF                 (46)
#define FH_PLUGIN_MOD_ORDER_REF                 (47)
#define FH_PLUGIN_DEL_ORDER_REF                 (48)
#define FH_PLUGIN_DEL_SYMBOL                    (49)
#define FH_PLUGIN_SYMBOL_LOOKUP                 (50)
#define FH_PLUGIN_FIRM_LOOKUP                   (51)
#define FH_PLUGIN_ORDER_LOOKUP                  (52)
#define FH_PLUGIN_GET_SYMBOL                    (53)
#define FH_PLUGIN_GET_VOLUME                    (54)
#define FH_PLUGIN_GET_PRICE                     (55)
#define FH_PLUGIN_GET_SIDE                      (56)
#define FH_PLUGIN_ADD_SYMBOL_BY_INDEX           (57)
#define FH_PLUGIN_LOOKUP_SYMBOL_BY_INDEX        (58)
                                                
/* ITCH hook functions */                       
#define FH_PLUGIN_ITCH_MSG_SYSTEM               (59)
#define FH_PLUGIN_ITCH_MSG_STOCK_DIR            (60)
#define FH_PLUGIN_ITCH_MSG_STOCK_TRADE_ACT      (61)
#define FH_PLUGIN_ITCH_MSG_MARKET_PART_POS      (62)
#define FH_PLUGIN_ITCH_MSG_ORDER_ADD            (63)
#define FH_PLUGIN_ITCH_MSG_ORDER_ADD_ATTR       (64)
#define FH_PLUGIN_ITCH_MSG_ORDER_EXE            (65)
#define FH_PLUGIN_ITCH_MSG_ORDER_EXE_PRICE      (66)
#define FH_PLUGIN_ITCH_MSG_ORDER_CANCEL         (67)
#define FH_PLUGIN_ITCH_MSG_ORDER_DELETE         (68)
#define FH_PLUGIN_ITCH_MSG_ORDER_REPLACE        (69)
#define FH_PLUGIN_ITCH_MSG_TRADE                (70)
#define FH_PLUGIN_ITCH_MSG_TRADE_CROSS          (71)
#define FH_PLUGIN_ITCH_MSG_TRADE_BROKEN         (72)
#define FH_PLUGIN_ITCH_MSG_NOII                 (73)

/* more general hook functions (will be renumbered at some point) */
#define FH_PLUGIN_CFG_LOAD                      (74)
#define FH_PLUGIN_LH_INIT                       (75)
#define FH_PLUGIN_PERIODIC_STATS                (76)
#define FH_PLUGIN_ALERT                         (77)
                                                
/* arca trade plugins */                        
#define FH_PLUGIN_ARCA_TRADE                    (78)
#define FH_PLUGIN_ARCA_TRADE_CANCEL             (79)
#define FH_PLUGIN_ARCA_TRADE_CORRECTION         (80)
#define FH_PLUGIN_GET_CONTEXT_POINTER           (81)
#define FH_PLUGIN_SET_CONTEXT_POINTER           (82)

/* Direct Edge plugins */
#define FH_PLUGIN_DIR_EDGE_MSG_SYSTEM_EVENT     (83)
#define FH_PLUGIN_DIR_EDGE_MSG_ADD_ORDER        (84)
#define FH_PLUGIN_DIR_EDGE_MSG_ORDER_EXECUTED   (85)
#define FH_PLUGIN_DIR_EDGE_MSG_ORDER_CANCELED   (86)
#define FH_PLUGIN_DIR_EDGE_MSG_TRADE            (87)
#define FH_PLUGIN_DIR_EDGE_MSG_BROKEN_TRADE     (88)
#define FH_PLUGIN_DIR_EDGE_MSG_SECURITY_STATUS  (89) 

/* BATS plugins       */
#define FH_PLUGIN_BATS_MSG_ADD_ORDER_LONG       (90)
#define FH_PLUGIN_BATS_MSG_ADD_ORDER_SHORT      (91)
#define FH_PLUGIN_BATS_MSG_ORDER_EXE            (92)
#define FH_PLUGIN_BATS_MSG_ORDER_EXE_PRICE      (93)
#define FH_PLUGIN_BATS_MSG_REDUCE_SIZE_LONG     (94)
#define FH_PLUGIN_BATS_MSG_REDUCE_SIZE_SHORT    (95)
#define FH_PLUGIN_BATS_MSG_MODIFY_ORDER_LONG    (96)
#define FH_PLUGIN_BATS_MSG_MODIFY_ORDER_SHORT   (97)
#define FH_PLUGIN_BATS_MSG_ORDER_DELETE         (98)
#define FH_PLUGIN_BATS_MSG_TRADE_LONG           (100)
#define FH_PLUGIN_BATS_MSG_TRADE_SHORT          (101)
#define FH_PLUGIN_BATS_MSG_TRADE_BREAK          (102)
#define FH_PLUGIN_BATS_MSG_END_OF_SESSION       (103)

/* CME plugin hooks */
#define FH_PLUGIN_CME_MSG                       (104)

/* maximum allowed hook function number */
#define FH_PLUGIN_MAX                           (104)

/* Type specification for hook function pointers */
typedef void (*fh_plugin_hook_t)(FH_STATUS *, ...);


/** 
 *  @brief Register a function for a plugin hook
 *
 *  @param hook the hook being registered
 *  @param hook_func the function to register
 *  @return status code indicating success or failure
 */
FH_STATUS fh_plugin_register(int, fh_plugin_hook_t);

/** 
 *  @brief Load any plugins that are in the specified directory
 *
 *  @param plugin_dir_name name of the plugin directory being loaded
 */
void fh_plugin_load(const char *plugin_dir_name);

/**
 *  @brief Returns the hook function for the given hook
 *
 *  @param hook the hook function to fetch
 *  @return hook function
 */
fh_plugin_hook_t fh_plugin_get_hook(int hook);

/**
 *  @brief Set the flag that determines whether two plugins can register for the same hook
 *
 *  @param flag value to set
 */
void fh_plugin_allow_override(int flag);

#endif /* __FH_PLUGIN_H__ */
