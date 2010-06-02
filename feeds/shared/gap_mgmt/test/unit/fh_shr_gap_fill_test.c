/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

/* system headers */
#include <time.h>

/* shared FH component headers */
#include "fh_shr_gap_fill.h"

/* FH unit test framework headers */
#include "fh_test_assert.h"

void test_new_function_returns_nonnull()
{
    FH_TEST_ASSERT_NOTNULL(fh_shr_gap_fill_new(100, 15));
}

void test_new_function_sets_initial_values()
{
    fh_shr_gap_fill_list_t *list;

    list = fh_shr_gap_fill_new(100, 15);
    FH_TEST_ASSERT_EQUAL(list->count, 0);
    FH_TEST_ASSERT_EQUAL(list->max, 100);
    FH_TEST_ASSERT_EQUAL(list->timeout, 15);
    FH_TEST_ASSERT_NULL(list->head);
    FH_TEST_ASSERT_NULL(list->tail);
}

void test_new_function_allocates_memory_pool()
{
    fh_shr_gap_fill_list_t *list;

    list = fh_shr_gap_fill_new(100, 15);
    FH_TEST_ASSERT_NOTNULL(list->data);
}

void test_pushing_new_entry_increases_count()
{
    fh_shr_gap_fill_list_t *list;

    list = fh_shr_gap_fill_new(100, 15);
    FH_TEST_ASSERT_STATEQUAL(fh_shr_gap_fill_push(list, 1, 1), FH_OK);
    FH_TEST_ASSERT_EQUAL(list->count, 1);
}

void test_pushing_too_many_entries_returns_number_lost()
{
    fh_shr_gap_fill_list_t *list;

    list = fh_shr_gap_fill_new(1, 15);
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_push(list, 10, 10), 0);
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_push(list, 20, 10), 10);
    FH_TEST_ASSERT_EQUAL(list->count, 1);
}

void test_find_of_first_and_last_sequence_numbers_return()
{
    fh_shr_gap_fill_list_t *list;

    list = fh_shr_gap_fill_new(1, 15);
    FH_TEST_ASSERT_STATEQUAL(fh_shr_gap_fill_push(list, 10, 10), FH_OK);
    FH_TEST_ASSERT_NOTNULL(fh_shr_gap_fill_find(list, 10));
    FH_TEST_ASSERT_NOTNULL(fh_shr_gap_fill_find(list, 19));
}

void test_node_contains_correct_list_pointer()
{
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *node;

    list = fh_shr_gap_fill_new(1, 15);
    fh_shr_gap_fill_push(list, 10, 10);
    node = fh_shr_gap_fill_find(list, 10);

    FH_TEST_ASSERT_PTREQUAL(list, node->list);
}

void test_find_of_first_minus_one_and_last_plus_one_returns_null()
{
    fh_shr_gap_fill_list_t *list;

    list = fh_shr_gap_fill_new(1, 15);
    FH_TEST_ASSERT_STATEQUAL(fh_shr_gap_fill_push(list, 10, 10), FH_OK);
    FH_TEST_ASSERT_NULL(fh_shr_gap_fill_find(list, 9));
    FH_TEST_ASSERT_NULL(fh_shr_gap_fill_find(list, 20));
}

void test_removal_of_first_and_last_seqence_numbers_does_not_create_new_node()
{
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *node;
    time_t                  start, end;

    list = fh_shr_gap_fill_new(2, 15);
    start = time(NULL);
    fh_shr_gap_fill_push(list, 10, 10);
    end = time(NULL);
    node = fh_shr_gap_fill_find(list, 10);

    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&node, 10), 0);
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&node, 19), 0);
    FH_TEST_ASSERT_EQUAL(list->count, 1);
    FH_TEST_ASSERT_LEQUAL(node->seq_no, 11);
    FH_TEST_ASSERT_LEQUAL(node->size, 8);
    FH_TEST_ASSERT_TRUE(node->timestamp >= start);
    FH_TEST_ASSERT_TRUE(node->timestamp <= end);
}

/************************ TWO NODE TESTS ************************/

void test_two_nodes_with_full_list_and_head_delete() {
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *orig_node, *new_node;
    time_t                  orig_time;

    /* create a new list of three gaps, find the middle gap, and store some original pointers */
    list = fh_shr_gap_fill_new(2, 15);
    fh_shr_gap_fill_push(list, 10, 10);
    fh_shr_gap_fill_push(list, 50, 10);
    orig_node = fh_shr_gap_fill_find(list, 15);
    new_node = orig_node;
    orig_time = orig_node->timestamp;

    /* assert that the head node is different (has been removed) and all values are correct */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&new_node, 15), 5);
    FH_TEST_ASSERT_PTREQUAL(orig_node, new_node);
    FH_TEST_ASSERT_EQUAL(list->count, 2);
    FH_TEST_ASSERT_LEQUAL(orig_node->seq_no, 16);
    FH_TEST_ASSERT_EQUAL(orig_node->size, 4);
    FH_TEST_ASSERT_EQUAL(orig_node->timestamp, orig_time);
}

void test_two_nodes_with_full_list_and_tail_delete() {
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *orig_node, *new_node, *orig_tail = NULL;
    time_t                  orig_time;

    /* create a new list of three gaps, find the middle gap, and store some original pointers */
    list = fh_shr_gap_fill_new(2, 15);
    fh_shr_gap_fill_push(list, 10, 10);
    fh_shr_gap_fill_push(list, 50, 10);
    orig_node = fh_shr_gap_fill_find(list, 55);
    new_node = orig_node;
    orig_time = orig_node->timestamp;

    /* assert that the head node is different (has been removed) and all values are correct */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&new_node, 55), 10);
    FH_TEST_ASSERT_PTRUNEQUAL(orig_tail, list->tail);
    FH_TEST_ASSERT_EQUAL(list->count, 2);
    FH_TEST_ASSERT_LEQUAL(orig_node->seq_no, 50);
    FH_TEST_ASSERT_EQUAL(orig_node->size, 5);
    FH_TEST_ASSERT_EQUAL(orig_node->timestamp, orig_time);
    FH_TEST_ASSERT_LEQUAL(new_node->seq_no, 56);
    FH_TEST_ASSERT_EQUAL(new_node->size, 4);
    FH_TEST_ASSERT_EQUAL(new_node->timestamp, orig_time);
}


/************************ THREE NODE TESTS ************************/

void test_three_nodes_with_full_list_and_middle_delete() {
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *orig_node, *new_node, *orig_head;
    time_t                  orig_time;

    /* create a new list of three gaps, find the middle gap, and store some original pointers */
    list = fh_shr_gap_fill_new(3, 15);
    fh_shr_gap_fill_push(list, 10, 10);
    fh_shr_gap_fill_push(list, 50, 10);
    fh_shr_gap_fill_push(list, 100, 10);
    orig_node = fh_shr_gap_fill_find(list, 55);
    new_node = orig_node;
    orig_head = list->head;
    orig_time = orig_node->timestamp;

    /* assert that the head node is different (has been removed) and all values are correct */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&new_node, 55), 10);
    FH_TEST_ASSERT_PTRUNEQUAL(list->head, orig_head);
    FH_TEST_ASSERT_EQUAL(list->count, 3);
    FH_TEST_ASSERT_LEQUAL(orig_node->seq_no, 50);
    FH_TEST_ASSERT_EQUAL(orig_node->size, 5);
    FH_TEST_ASSERT_EQUAL(orig_node->timestamp, orig_time);
    FH_TEST_ASSERT_LEQUAL(new_node->seq_no, 56);
    FH_TEST_ASSERT_EQUAL(new_node->size, 4);
    FH_TEST_ASSERT_EQUAL(new_node->timestamp, orig_time);

    /* assert that all pointers are correct */
    FH_TEST_ASSERT_PTREQUAL(orig_node->next, new_node);
    FH_TEST_ASSERT_PTREQUAL(new_node->prev, orig_node);
    FH_TEST_ASSERT_PTREQUAL(list->head, orig_node);
    FH_TEST_ASSERT_PTREQUAL(list->tail, new_node->next);
    FH_TEST_ASSERT_NULL(list->head->prev);
    FH_TEST_ASSERT_NULL(list->tail->next);
}

void test_three_nodes_with_full_list_and_head_delete() {
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *orig_node, *new_node;
    time_t                  orig_time;

    /* create a new list of three gaps, find the middle gap, and store some original pointers */
    list = fh_shr_gap_fill_new(3, 15);
    fh_shr_gap_fill_push(list, 10, 10);
    fh_shr_gap_fill_push(list, 50, 10);
    fh_shr_gap_fill_push(list, 100, 10);
    orig_node = fh_shr_gap_fill_find(list, 15);
    new_node = orig_node;
    orig_time = orig_node->timestamp;

    /* assert that the head node is different (has been removed) and all values are correct */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&new_node, 15), 5);
    FH_TEST_ASSERT_PTREQUAL(orig_node, new_node);
    FH_TEST_ASSERT_EQUAL(list->count, 3);
    FH_TEST_ASSERT_LEQUAL(orig_node->seq_no, 16);
    FH_TEST_ASSERT_EQUAL(orig_node->size, 4);
    FH_TEST_ASSERT_EQUAL(orig_node->timestamp, orig_time);
}

void test_three_nodes_with_full_list_and_tail_delete() {
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *orig_node, *new_node, *orig_tail = NULL;
    time_t                  orig_time;

    /* create a new list of three gaps, find the middle gap, and store some original pointers */
    list = fh_shr_gap_fill_new(3, 15);
    fh_shr_gap_fill_push(list, 10, 10);
    fh_shr_gap_fill_push(list, 50, 10);
    fh_shr_gap_fill_push(list, 100, 10);
    orig_node = fh_shr_gap_fill_find(list, 105);
    new_node = orig_node;
    orig_time = orig_node->timestamp;

    /* assert that the head node is different (has been removed) and all values are correct */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&new_node, 105), 10);
    FH_TEST_ASSERT_PTRUNEQUAL(orig_tail, list->tail);
    FH_TEST_ASSERT_EQUAL(list->count, 3);
    FH_TEST_ASSERT_LEQUAL(orig_node->seq_no, 100);
    FH_TEST_ASSERT_EQUAL(orig_node->size, 5);
    FH_TEST_ASSERT_EQUAL(orig_node->timestamp, orig_time);
    FH_TEST_ASSERT_LEQUAL(new_node->seq_no, 106);
    FH_TEST_ASSERT_EQUAL(new_node->size, 4);
    FH_TEST_ASSERT_EQUAL(new_node->timestamp, orig_time);
}

void test_three_nodes_with_nonfull_list_and_middle_delete() {
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *orig_node, *new_node, *orig_head, *orig_tail;
    time_t                  orig_time;

    /* create a new list of three gaps, find the middle gap, and store some original pointers */
    list = fh_shr_gap_fill_new(4, 15);
    fh_shr_gap_fill_push(list, 100, 100);
    fh_shr_gap_fill_push(list, 200, 100);
    fh_shr_gap_fill_push(list, 300, 100);
    orig_node = fh_shr_gap_fill_find(list, 250);
    new_node = orig_node;
    orig_head = list->head;
    orig_tail = list->tail;
    orig_time = orig_node->timestamp;

    /* assert that the head and tail haven't changed and the count has increased */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&new_node, 250), 0);
    FH_TEST_ASSERT_PTREQUAL(list->head, orig_head);
    FH_TEST_ASSERT_PTREQUAL(list->tail, orig_tail);
    FH_TEST_ASSERT_EQUAL(list->count, 4);

    /* check relevant node values */
    FH_TEST_ASSERT_LEQUAL(orig_node->seq_no, 200);
    FH_TEST_ASSERT_EQUAL(orig_node->size, 50);
    FH_TEST_ASSERT_EQUAL(orig_node->timestamp, orig_time);
    FH_TEST_ASSERT_LEQUAL(new_node->seq_no, 251);
    FH_TEST_ASSERT_EQUAL(new_node->size, 49);
    FH_TEST_ASSERT_EQUAL(new_node->timestamp, orig_time);
}


/************************ FIVE NODE TESTS ************************/

void test_five_nodes_with_full_list_and_middle_delete() {
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *orig_node, *new_node, *orig_head, *orig_tail;
    time_t                  orig_time;

    /* create a new list of three gaps, find the middle gap, and store some original pointers */
    list = fh_shr_gap_fill_new(5, 15);
    fh_shr_gap_fill_push(list, 100, 100);
    fh_shr_gap_fill_push(list, 200, 100);
    fh_shr_gap_fill_push(list, 300, 100);
    fh_shr_gap_fill_push(list, 400, 100);
    fh_shr_gap_fill_push(list, 500, 100);
    orig_node = fh_shr_gap_fill_find(list, 350);
    new_node = orig_node;
    orig_head = list->head;
    orig_tail = list->tail;
    orig_time = orig_node->timestamp;

    /* assert that the tail node count and count haven't changed while the head has */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&new_node, 350), 100);
    FH_TEST_ASSERT_PTRUNEQUAL(list->head, orig_head);
    FH_TEST_ASSERT_PTREQUAL(list->tail, orig_tail);
    FH_TEST_ASSERT_EQUAL(list->count, 5);

    /* check relevant node values */
    FH_TEST_ASSERT_LEQUAL(orig_node->seq_no, 300);
    FH_TEST_ASSERT_EQUAL(orig_node->size, 50);
    FH_TEST_ASSERT_EQUAL(orig_node->timestamp, orig_time);
    FH_TEST_ASSERT_LEQUAL(new_node->seq_no, 351);
    FH_TEST_ASSERT_EQUAL(new_node->size, 49);
    FH_TEST_ASSERT_EQUAL(new_node->timestamp, orig_time);
}

/************************ FINAL SEQUENCE NUMBER REMOVAL TESTS ************************/

void test_final_seqno_removal_of_only_node()
{
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *node;

    /* create a new list of one gap of size 1 and then delete that 1 */
    list = fh_shr_gap_fill_new(1, 15);
    fh_shr_gap_fill_push(list, 1, 1);
    node = fh_shr_gap_fill_find(list, 1);

    /* delete that sequence number and assert that everything is gone */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&node, 1), 0);
    FH_TEST_ASSERT_NULL(node);
    FH_TEST_ASSERT_NULL(list->head);
    FH_TEST_ASSERT_NULL(list->tail);
    FH_TEST_ASSERT_EQUAL(list->count, 0);
}

void test_final_seqno_removal_of_head_node()
{
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *node;

    /* create a new list of one gap of size 1 and then delete that 1 */
    list = fh_shr_gap_fill_new(3, 15);
    fh_shr_gap_fill_push(list, 1, 1);
    fh_shr_gap_fill_push(list, 2, 1);
    fh_shr_gap_fill_push(list, 3, 1);
    node = fh_shr_gap_fill_find(list, 1);

    /* delete that sequence number and assert that everything is gone */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&node, 1), 0);
    FH_TEST_ASSERT_NULL(node);
    FH_TEST_ASSERT_PTREQUAL(list->head, list->tail->prev);
    FH_TEST_ASSERT_EQUAL(list->count, 2);
}

void test_final_seqno_removal_of_tail_node()
{
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *node;

    /* create a new list of one gap of size 1 and then delete that 1 */
    list = fh_shr_gap_fill_new(3, 15);
    fh_shr_gap_fill_push(list, 1, 1);
    fh_shr_gap_fill_push(list, 2, 1);
    fh_shr_gap_fill_push(list, 3, 1);
    node = fh_shr_gap_fill_find(list, 3);

    /* delete that sequence number and assert that everything is gone */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&node, 3), 0);
    FH_TEST_ASSERT_NULL(node);
    FH_TEST_ASSERT_PTREQUAL(list->head->next, list->tail);
    FH_TEST_ASSERT_EQUAL(list->count, 2);
}

void test_final_seqno_removal_of_middle_node()
{
    fh_shr_gap_fill_list_t *list;
    fh_shr_gap_fill_node_t *node, *orig_head, *orig_tail;

    /* create a new list of one gap of size 1 and then delete that 1 */
    list = fh_shr_gap_fill_new(3, 15);
    fh_shr_gap_fill_push(list, 1, 1);
    fh_shr_gap_fill_push(list, 2, 1);
    fh_shr_gap_fill_push(list, 3, 1);
    node = fh_shr_gap_fill_find(list, 2);
    orig_head = list->head;
    orig_tail = list->tail;

    /* delete that sequence number and assert that everything is gone */
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&node, 2), 0);
    FH_TEST_ASSERT_NULL(node);
    FH_TEST_ASSERT_PTREQUAL(list->head, orig_head);
    FH_TEST_ASSERT_PTREQUAL(list->tail, orig_tail);
    FH_TEST_ASSERT_PTREQUAL(list->head->next, list->tail);
    FH_TEST_ASSERT_EQUAL(list->count, 2);
}

void test_proper_gap_flushing_operation_with_expired_entry()
{
    fh_shr_gap_fill_list_t *list = fh_shr_gap_fill_new(1, 1);

    fh_shr_gap_fill_push(list, 1, 1);
    sleep(2);

    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_flush(list), 1);
    FH_TEST_ASSERT_EQUAL(list->count, 0);
}

void test_proper_gap_flushing_operation_without_expired_entry()
{
    fh_shr_gap_fill_list_t *list = fh_shr_gap_fill_new(1, 10);

    fh_shr_gap_fill_push(list, 1, 1);
    sleep(1);

    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_flush(list), 0);
    FH_TEST_ASSERT_EQUAL(list->count, 1);
}

void test_proper_gap_flushing_operation_with_some_entries()
{
    fh_shr_gap_fill_list_t *list = fh_shr_gap_fill_new(3, 1);

    fh_shr_gap_fill_push(list, 1, 1);
    fh_shr_gap_fill_push(list, 2, 1);
    sleep(2);
    fh_shr_gap_fill_push(list, 3, 1);

    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_flush(list), 2);
    FH_TEST_ASSERT_EQUAL(list->count, 1);
}

void test_fully_filling_gap_with_previous_middle_insertion()
{
    fh_shr_gap_fill_list_t *list = fh_shr_gap_fill_new(3, 1);
    fh_shr_gap_fill_node_t *node;

    fh_shr_gap_fill_push(list, 1, 3);

    node = fh_shr_gap_fill_find(list, 2);
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&node, 2), 0);
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&node, 1), 0);
    FH_TEST_ASSERT_EQUAL(fh_shr_gap_fill_del(&node, 3), 0);
}
