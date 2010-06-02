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

#ifndef __FH_TEST_MSG_H__
#define __FH_TEST_MSG_H__

// FH test headers
#include "fh_test_sym.h"

#define FH_TEST_MSG_MAX 1024

// enumeration of the various allowable message types
typedef enum fh_test_msg_type {
    SUCCESS = 0,
    FAILURE,
    ERROR
} fh_test_msg_type_t;

// structure to hold a single failure, error, etc. message
typedef struct fh_test_msg {
    fh_test_msg_type_t  type;   
    char                message[FH_TEST_MSG_MAX];
} fh_test_msg_t;

/*! \brief Get a string representation of a message type
 *
 *  \param type the type being converted to string
 *  \return string version of the type
 */
const char *fh_test_msg_typestr(fh_test_msg_type_t type);

/*! \brief Get an upper case string representation of a message type
 *
 *  \param type the type being converted to string
 *  \return string version of the type
 */
const char *fh_test_msg_uctypestr(fh_test_msg_type_t type);


/*! \brief Add an error, failure, etc. message to the message list
 *
 *  \param type the type of message being added (ERROR, FAILURE, etc.)
 *  \param message message (in addition to basic info) to be include in the message
 *  \param test the test structure where the error occured
 */
void fh_test_msg_add(fh_test_msg_type_t type, const char *message, fh_test_sym_test_t *test);

/*! \brief Print a listing of all messages
 */
void fh_test_msg_print();

#endif  /* __FH_TEST_MSG_H__ */
