#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "heap.h"
#include "module.h"
#include "parser.h"
#include "prolog.h"
#include "query.h"

bool fn_iso_findall_3(query *q)
{
	GET_FIRST_ARG(xp1,any);
	GET_NEXT_ARG(xp2,callable);
	GET_NEXT_ARG(xp3,list_or_nil_or_var);

	// This checks for a valid list (it allows for partial but acyclic lists)...

	if (!q->retry) {
		bool is_partial = false;

		if (is_iso_list(xp3) && !check_list(q, xp3, xp3_ctx, &is_partial, NULL) && !is_partial)
			return throw_error(q, xp3, xp3_ctx, "type_error", "list");

#if 0
		cell *p0 = deep_clone_to_heap(q, q->st.curr_cell, q->st.curr_frame);
		check_heap_error(p0);
#else
		cell *p0 = deep_copy_to_heap(q, q->st.curr_cell, q->st.curr_frame, false);
		check_heap_error(p0);
		unify(q, q->st.curr_cell, q->st.curr_frame, p0, q->st.curr_frame);
#endif
		GET_FIRST_ARG0(p1,any,p0);
		GET_NEXT_ARG(p2,any);

		grab_queuen(q);
		assert(q->st.qnbr < MAX_QUEUES);
		cell *tmp = clone_to_heap(q, true, p2, 2+p1->nbr_cells+2);
		check_heap_error(tmp);
		pl_idx_t nbr_cells = 1 + p2->nbr_cells;
		make_struct(tmp+nbr_cells++, g_sys_queue_s, fn_sys_queuen_2, 2, 1+p1->nbr_cells);
		make_int(tmp+nbr_cells++, q->st.qnbr);
		nbr_cells += safe_copy_cells(tmp+nbr_cells, p1, p1->nbr_cells);
		make_struct(tmp+nbr_cells++, g_fail_s, fn_iso_fail_0, 0, 0);
		make_call(q, tmp+nbr_cells);
		check_heap_error(push_barrier(q));
		q->st.curr_cell = tmp;
		return true;
	}

	if (!queuen_used(q)) {
		drop_queuen(q);
		return unify(q, xp3, xp3_ctx, make_nil(), q->st.curr_frame);
	}

	// Retry takes the queue

	pl_idx_t nbr_cells = queuen_used(q);
	cell *solns = take_queuen(q);
	drop_queuen(q);

	// Now grab matching solutions with fresh variables...

	try_me(q, MAX_ARITY);
	check_heap_error(init_tmp_heap(q), free(solns));

	for (cell *c = solns; nbr_cells; nbr_cells -= c->nbr_cells, c += c->nbr_cells) {
		cell *tmp = alloc_on_tmp(q, 1);
		check_heap_error(tmp, free(solns));
		make_struct(tmp, g_dot_s, NULL, 2, c->nbr_cells);
		tmp = deep_copy_to_tmp(q, c, q->st.fp, false);
		check_heap_error(tmp, free(solns));
	}

	cell *l = end_list(q);
	check_heap_error(l);
	fix_list(l);
	free(solns);
	return unify(q, xp3, xp3_ctx, l, q->st.curr_frame);
}
