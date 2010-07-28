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

#ifndef __FH_SHR_GAP_FILL_H__
#define __FH_SHR_GAP_FILL_H__

/* system headers */
#include <stdint.h>
#include <time.h>

/* common FH headers */
#include "fh_errors.h"
#include "fh_log.h"
#include "fh_mpool.h"

/* convenience typedefs */
typedef struct fh_shr_gap_fill_node fh_shr_gap_fill_node_t;
typedef struct fh_shr_gap_fill_list fh_shr_gap_fill_list_t;

/**
 *  @brief One node in a list of gaps, used to track and fill gaps in a message sequence
 */
struct fh_shr_gap_fill_node {
    uint64_t                 seq_no;    /**< starting sequence number of this gap */
    uint32_t                 size;      /**< the size of this gap */
    time_t                   timestamp; /**< time when this gap was created */
    fh_shr_gap_fill_node_t  *next;      /**< pointer to the next gap in the list */
    fh_shr_gap_fill_node_t  *prev;      /**< pointer to the previous gap in the list */
    fh_shr_gap_fill_list_t  *list;      /**< pointer to the list this node is in */
};

/**
 *  @brief One node in a list of gaps, used to track and fill gaps in a message sequence
 */
struct fh_shr_gap_fill_list {
    uint32_t                 count;     /**< number of gaps stored in this list */
    uint32_t                 max;       /**< maximum number of gaps this list can store */
    uint32_t                 timeout;   /**< time to wait for gaps to be filled before giving up */
    fh_shr_gap_fill_node_t  *head;      /**< the head of the gap list */
    fh_shr_gap_fill_node_t  *tail;      /**< the tail of the gap list */
    fh_mpool_t              *data;      /**< memory pool from which to pull new entries */
};

/**
 *  @brief Allocate a new gap list structure of the specified size and return a pointer to it
 *
 *  @param max the maximum number of gap entries this list can hold
 *  @param timeout the time (in seconds) after which gaps will be removed
 *  @return newly allocated list (or NULL if there is a problem)
 */
fh_shr_gap_fill_list_t *fh_shr_gap_fill_new(uint32_t max, uint32_t timeout);

/**********************************************************************************/
/************** implementations in the header so they can be inlined **************/
/**********************************************************************************/

/**
 *  @brief Insert a new gap into the list
 *
 *  @param list the list we are inserting into
 *  @param seq_no the starting sequence number of the new gap
 *  @param size the size of the new gap
 *  @return -1 if any error occurs, 0 if the push is successful, the number of lost sequence
 *          numbers if a record has to be removed to make room
 */
static inline int fh_shr_gap_fill_push(fh_shr_gap_fill_list_t *list, uint64_t seq_no, uint32_t size)
{
    fh_shr_gap_fill_node_t  *node;
    int                      rc = 0;

    /* if the list is full, use the head node as the new node and return the number lost */
    if (list->count + 1 > list->max) {
        node = list->head;
        list->head= list->head->next;
        rc = node->size;
    }
    else {
        /* fetch a new node from the memory pool */
        node = (fh_shr_gap_fill_node_t *)fh_mpool_get(list->data);
        if (node == NULL) {
            FH_LOG(CSI, ERR, ("unable to fetch a new gap list entry from memory pool"));
            return -1;
        }

        /* increment the list count */
        list->count++;
    }

    /* set up the new node's values */
    node->next      = NULL;
    node->prev      = list->tail;
    node->seq_no    = seq_no;
    node->size      = size;
    node->timestamp = time(NULL);
    node->list      = list;

    /* insert the new node at the tail of the list */
    if (list->tail == NULL) {
        list->tail = node;
        list->head = node;
    }
    else {
        list->tail->next = node;
        list->tail = node;
    }

    /* if we get here, success! */
    return rc;
}

/**
 *  @brief Locate the gap record that contains the specified sequence number
 *
 *  @param list the gap list to look in
 *  @param seq_no the sequence number being looked for
 *  @return pointer to the gap record that contains the sequence number (or NULL if not found)
 */
static inline fh_shr_gap_fill_node_t *fh_shr_gap_fill_find(fh_shr_gap_fill_list_t *list,
                                                           uint64_t seq_no)
{
    fh_shr_gap_fill_node_t *curr = list->head;

    /* look through the list for the right gap */
    while (curr != NULL) {
        if (seq_no >= curr->seq_no && seq_no < curr->seq_no + curr->size) {
            return curr;
        }
        curr = curr->next;
    }

    /* if we get all the way here, not found */
    return NULL;
}

/**
 *  @brief Remove the specified sequence number from the specified entry
 *  It is possible that this function will create a new gap entry if the specified sequence
 *  number falls in the middle of this gap's range.  In this case the node parameter will be
 *  updated with a pointer to the newly created node (since it will contain the next sequence
 *  number)
 *
 *  @param node a pointer to a pointer containing the node to look in
 *  @param seq_no the sequence number being deleted
 *  @return -1 if the sequence number was not found, 0 if it was found and no nodes were deleted
 *          in the process, and the number of lost sequence numbers if nodes were deleted
 */
static inline int fh_shr_gap_fill_del(fh_shr_gap_fill_node_t **node, uint64_t seq_no)
{
    fh_shr_gap_fill_node_t  *new_node;
    fh_shr_gap_fill_node_t  *curr = *node;
    fh_shr_gap_fill_list_t  *list = curr->list;
    int                      rc   = 0;

    /* is the sequence number to delete the first in the range? */
    if (curr->seq_no == seq_no) {
        curr->seq_no++;
        curr->size--;
    }
    /* or is it the last sequence number? */
    else if (curr->seq_no + curr->size - 1 == seq_no) {
        curr->size--;
    }
    /* or is it in the middle? */
    else if (seq_no > curr->seq_no && seq_no < curr->seq_no + curr->size - 1) {
        /* first, determine if a node will have to be deleted */
        if (list->count >= list->max) {
            /* if this node is the list head, just update it with the new values */
            if (curr == list->head) {
                rc           = seq_no - curr->seq_no;
                curr->size  -= rc + 1;
                curr->seq_no = seq_no + 1;
            }
            /* otherwise, we have to repurpose the head node as the new node */
            else {
                /* save a reference to the old head */
                new_node = list->head;

                /* move the list's head pointer up one spot */
                list->head = list->head->next;
                list->head->prev = NULL;

                /* insert the "new" node where it belongs */
                new_node->next = curr->next;
                new_node->prev = curr;
                curr->next = new_node;
                if (curr == list->tail) {
                    list->tail = new_node;
                }
                else {
                    new_node->next->prev = new_node;
                }

                /* set the values of rc, curr, and new_node */
                rc               = curr->size;
                new_node->seq_no = seq_no + 1;
                new_node->size   = curr->size - (seq_no - curr->seq_no) - 1;
                curr->size      -= new_node->size + 1;

                /* finally, set the node double-pointer so the caller has new_node */
                *node = new_node;
            }
        }
        /* we can get a new node from the memory pool */
        else {
            /* fetch a new node from the memory pool */
            new_node = (fh_shr_gap_fill_node_t *)fh_mpool_get(list->data);
            if (new_node == NULL) {
                FH_LOG(CSI, ERR, ("unable to fetch a new gap list entry from memory pool"));
                return -1;
            }

            /* update pointers */
            new_node->next = curr->next;
            new_node->prev = curr;
            new_node->list = list;
            curr->next = new_node;

            /* update the tail if the current node was the tail */
            if (curr == list->tail) {
                list->tail = new_node;
            }
            else {
                new_node->next->prev = new_node;
            }

            /* increase the gap count */
            list->count++;

            /* set the values of curr and new_node */
            new_node->seq_no = seq_no + 1;
            new_node->size = curr->size - (seq_no - curr->seq_no) - 1;
            new_node->timestamp = curr->timestamp;
            curr->size -= (new_node->size + 1);

            /* finally, set the node double-pointer so the caller has new_node */
            *node = new_node;
        }
    }

    /* before returning we need to check whether the current gap has been completely filled */
    if (curr->size == 0) {
        /* head and tail are the same (list of 1) */
        if (list->head == list->tail) {
            list->head = NULL;
            list->tail = NULL;
        }
        /* the current node is the head */
        else if (curr == list->head) {
            list->head = list->head->next;
            list->head->prev = NULL;
        }
        /* the current node is the tail */
        else if (curr == list->tail) {
            list->tail = list->tail->prev;
            list->tail->next = NULL;
        }
        /* the current node is in the middle of the list */
        else {
            curr->prev->next = curr->next;
            curr->next->prev = curr->prev;
        }

        /* set the "returned" node to NULL, put it back in the mpool, and decrement gap count */
        *node = NULL;
        fh_mpool_put(list->data, curr);
        list->count--;
    }

    /* return a code indicating whether a node had to be removed to make room */
    return rc;
}

static inline int fh_shr_gap_fill_flush(fh_shr_gap_fill_list_t *list)
{
    int     lost   = 0;
    time_t  limit  = time(NULL) - list->timeout;

    /* go through every list node */
    while (list->head != NULL) {
        /* if we reach a point in the list where timestamp is >= limit the rest will be as well */
        if (list->head->timestamp >= limit) {
            break;
        }

        /* add the head node's sequence numbers to the lost count */
        lost += list->head->size;

        /* remove this entry */
        fh_mpool_put(list->data, list->head);
        list->head = list->head->next;
        if (list->head == NULL) {
            list->tail = NULL;
        }
        else {
            list->head->prev = NULL;
        }
        list->count--;
    }

    /* return the count of lost sequence numbers */
    return lost;
}

#endif /* __FH_SHR_GAP_FILL_H__ */
