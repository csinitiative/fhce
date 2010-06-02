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

// System headers
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

// FH test headers
#include "fh_test_msg.h"

// global data
static fh_test_msg_t *messages  = NULL;
static long           msg_count = 0;

/*! \brief Get a string representation of a message type
 *
 *  \param type the type being converted to string
 *  \return string version of the type
 */
const char *fh_test_msg_typestr(fh_test_msg_type_t type)
{
    switch (type) {
    case SUCCESS:
        return "Success";

    case ERROR:
        return "Error";
        
    case FAILURE:
        return "Failure";
    }
    
    return "Unknown problem";
}

/*! \brief Get an upper case string representation of a message type
 *
 *  \param type the type being converted to string
 *  \return string version of the type
 */
const char *fh_test_msg_uctypestr(fh_test_msg_type_t type)
{
    switch (type) {
    case SUCCESS:
        return "SUCCESS";

    case ERROR:
        return "ERROR";
        
    case FAILURE:
        return "FAILURE";
    }
    
    return "UNKNOWN";
}

/*! \brief Add an error, failure, etc. message to the message list
 *
 *  \param type the type of message being added (ERROR, FAILURE, etc.)
 *  \param message message (in addition to basic info) to be include in the message
 *  \param test the test structure where the error occured
 */
void fh_test_msg_add(fh_test_msg_type_t type, const char *message, fh_test_sym_test_t *test)
{
    fh_test_msg_t *msg;
    
    // allocate space for the new message
    messages = (fh_test_msg_t *)realloc(messages, ++msg_count * sizeof(fh_test_msg_t));
    if (messages == NULL) {
        fprintf(stderr, "\nERROR: allocating message memory: %s (%d)\n", strerror(errno), errno);
        exit(1);
    }
    
    // copy all relevant data into the newly allocated message
    msg = &messages[msg_count - 1];
    msg->type = type;
    sprintf(msg->message, "%s in %s (%s:%ld)", fh_test_msg_typestr(type), test->name,
            test->source_file, test->first_line);
    if (message != NULL) {
        snprintf(&msg->message[strlen(msg->message)], FH_TEST_MSG_MAX, "\n%s", message);
    }
}

/*! \brief Print a listing of all messages
 */
void fh_test_msg_print()
{
    long i;
    
    for (i = 0; i < msg_count; i++) {
        printf("\n\n%ld) %s", i + 1, messages[i].message);
    }
}
