#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "heap.h"
#include "module.h"
#include "parser.h"
#include "prolog.h"
#include "query.h"

#include "bif_atts.h"

static const char *do_attribute(query *q, cell *c, unsigned arity)
{
	module *m = q->pl->modules;

	while (m) {
		if ((arity == m->arity) && !CMP_STRING_TO_CSTR(q, c, m->name))
			return m->orig->name;

		m = m->next;
	}

	return q->st.m->name;
}

bool bif_attribute_3(query *q)
{
	GET_FIRST_ARG(p1,atom_or_var);
	GET_NEXT_ARG(p2,atom);
	GET_NEXT_ARG(p3,integer);
	const char *m_name = do_attribute(q, p2, get_smalluint(p3));
	cell tmp;
	make_atom(&tmp, new_atom(q->pl, m_name));
	return unify(q, p1, p1_ctx, &tmp, q->st.curr_frame);
}

bool bif_put_atts_2(query *q)
{
	GET_FIRST_ARG(p1,var);
	GET_NEXT_ARG(p2,callable);
	const frame *f = GET_FRAME(p1_ctx);
	slot *e = GET_SLOT(f, p1->var_nbr);
	bool is_minus = p2->val_off == g_minus_s;

	if (!e->c.attrs && is_minus)
		return true;

	cell *attr = p2;

	if ((p2->val_off == g_minus_s) || (p2->val_off == g_plus_s))
		attr++;

	//if (e->c.attrs || !is_nil(attr))
		add_trail(q, p1_ctx, p1->var_nbr, e->c.attrs, e->c.attrs_ctx);

	if (is_nil(attr)) {
		e->c.flags = 0;
		e->c.attrs = NULL;
		e->c.attrs_ctx = 0;
		return true;
	}

	unsigned a_arity = attr->arity;
	const char *m_name = do_attribute(q, attr, a_arity);
	init_tmp_heap(q);

	if (!is_minus) {
		cell *tmp = alloc_on_tmp(q, 1+1);
		check_heap_error(tmp);
		make_atom(tmp, g_dot_s);
		tmp->arity = 2;
		tmp->nbr_cells += 1+attr->nbr_cells;
		make_atom(tmp+1, new_atom(q->pl, m_name));
		tmp[1].arity = 1;
		cell *tmp2 = clone_to_tmp(q, attr, p2_ctx);
		check_heap_error(tmp2);
		cell *tmp3 = get_tmp_heap(q, 1);
		tmp3->nbr_cells += tmp2->nbr_cells;
	}

	if (e->c.attrs) {
		cell *l = e->c.attrs;
		pl_idx l_ctx = e->c.attrs_ctx;
		LIST_HANDLER(l);

		while (is_iso_list(l)) {
			cell *h = LIST_HEAD(l);

			if (CMP_STRING_TO_CSTR(q, h, m_name)
				|| CMP_STRING_TO_STRING(q, h+1, attr)
				|| ((h+1)->arity != a_arity)) {
				append_list(q, h);
			}

			l = LIST_TAIL(l);
		}
	}

	cell *l = end_list(q);
	check_heap_error(l);

	if (is_nil(l)) {
		e->c.flags = 0;
		e->c.attrs = NULL;
		e->c.attrs_ctx = 0;
		return true;
	}

	e->c.attrs = l;
	e->c.attrs_ctx = q->st.curr_frame;
	return true;
}

bool bif_get_atts_2(query *q)
{
	GET_FIRST_ARG(p1,var);
	GET_NEXT_ARG(p2,callable_or_var);
	const frame *f = GET_FRAME(p1_ctx);
	bool is_minus = !is_var(p2) && p2->val_off == g_minus_s;
	slot *e = GET_SLOT(f, p1->var_nbr);

	if (!e->c.attrs)
		return is_minus ? true : false;

	if (is_var(p2)) {
		cell *l = e->c.attrs;
		pl_idx l_ctx = e->c.attrs_ctx;
		init_tmp_heap(q);
		LIST_HANDLER(l);

		while (is_iso_list(l)) {
			cell *h = LIST_HEAD(l);
			//DUMP_TERM("$att", h, l_ctx, 1);

			if (!is_nil(h+1))
				append_list(q, h+1);

			l = LIST_TAIL(l);
		}

		l = end_list(q);
		check_heap_error(l);

		if (is_nil(l))
			return false;

		return unify(q, p2, p2_ctx, l, q->st.curr_frame);
	}

	cell *attr = p2;

	if ((p2->val_off == g_minus_s) || (p2->val_off == g_plus_s))
		attr++;

	unsigned a_arity = attr->arity;
	const char *m_name = do_attribute(q, attr, a_arity);
	cell *l = e->c.attrs;
	pl_idx l_ctx = e->c.attrs_ctx;
	LIST_HANDLER(l);

	while (is_iso_list(l)) {
		cell *h = LIST_HEAD(l);

		if (!CMP_STRING_TO_CSTR(q, h, m_name)
			&& !CMP_STRING_TO_STRING(q, h+1, attr)
			&& ((h+1)->arity == a_arity)) {
			if (is_minus)
				return false;

			return unify(q, attr, p2_ctx, h+1, l_ctx);
		}

		l = LIST_TAIL(l);
	}

	return is_minus ? true : false;
}

bool bif_sys_list_attributed_1(query *q)
{
	GET_FIRST_ARG(p1,var);
	parser *p = q->p;
	const frame *f = GET_FIRST_FRAME();
	check_heap_error(init_tmp_heap(q));

	for (unsigned i = 0; i < p->nbr_vars; i++) {
		//if (!strcmp(p->vartab.var_name[i], "_"))
		//	continue;

		slot *e = GET_SLOT(f, i);
		cell *c = &e->c;
		cell *v = deref(q, c, q->st.curr_frame);
		pl_idx v_ctx = q->latest_ctx;

#if 0
		if (is_interned(v)) {
			collect_vars(q, v, v_ctx);

			for (unsigned i = 0, done = 0; i < q->tab_idx; i++) {
				frame *vf = GET_FRAME(q->pl->tabs[i].ctx);
				slot *ve = GET_SLOT(vf, q->pl->tabs[i].var_nbr);
				cell *v = &ve->c;

				if (!is_empty(v) || !v->attrs)
					continue;

				cell tmp;
				make_var(&tmp, new_atom(q->pl, p->vartab.var_name[i]), i);
				append_list(q, &tmp);
			}
		}
#endif

		if (!c->attrs)
			continue;

		cell tmp;
		make_var(&tmp, new_atom(q->pl, p->vartab.var_name[i]), i);
		append_list(q, &tmp);
	}

	cell *l = end_list(q);
	check_heap_error(l);
	return unify(q, p1, p1_ctx, l, 0);
}

bool bif_sys_attributed_var_1(query *q)
{
	GET_FIRST_ARG(p1,var);
	const frame *f = GET_FRAME(p1_ctx);
	slot *e = GET_SLOT(f, p1->var_nbr);

	if (!e->c.attrs)
		return false;

	cell *l = e->c.attrs;
	pl_idx l_ctx = e->c.attrs_ctx;
	init_tmp_heap(q);
	LIST_HANDLER(l);

	while (is_iso_list(l)) {
		cell *h = LIST_HEAD(l);
		//DUMP_TERM("$att", h, l_ctx, 1);

		if (!is_nil(h+1))
			append_list(q, h+1);

		l = LIST_TAIL(l);
	}

	l = end_list(q);
	check_heap_error(l);

	if (is_nil(l))
		return false;

	return true;
}

bool bif_sys_unattributed_var_1(query *q)
{
	return !bif_sys_attributed_var_1(q);
}

typedef struct {
	blob b;
	pl_idx lo_tp, hi_tp;
	slot e[];
} bind_state;

static void check_occurs(unsigned var_nbr, pl_idx var_ctx, cell *c)
{
	if (is_indirect(c))
		c = c->val_ptr;

	for (unsigned nbr_cells = c->nbr_cells; nbr_cells--; c++) {
		if (!is_ref(c))
			continue;

		if (var_nbr != c->var_nbr)
			continue;

		if (var_ctx != c->var_ctx)
			continue;

		c->flags |= FLAG_VAR_CYCLIC;
	}
}

bool bif_sys_undo_trail_2(query *q)
{
	GET_FIRST_ARG(p1,var);
	GET_NEXT_ARG(p2,var);
	q->run_hook = false;

	if (q->undo_hi_tp <= q->undo_lo_tp)
		return unify(q, p1, p1_ctx, make_nil(), q->st.curr_frame);

	pl_idx slots = q->undo_hi_tp - q->undo_lo_tp;
	bind_state *save = malloc(sizeof(bind_state)+(sizeof(slot)*slots));
	check_error(save);
	save->b.ptr = save->b.ptr2 = NULL;
	save->lo_tp = q->undo_lo_tp;
	save->hi_tp = q->undo_hi_tp;
	init_tmp_heap(q);

	// Unbind & save our vars

	for (pl_idx i = q->undo_lo_tp, j = 0; i < q->undo_hi_tp; i++, j++) {
		const trail *tr = q->trails + i;
		const frame *f = GET_FRAME(tr->var_ctx);
		slot *e = GET_SLOT(f, tr->var_nbr);
		save->e[j] = *e;
		check_occurs(tr->var_nbr, tr->var_ctx, &e->c);
		//printf("*** unbind [%u:%u] hi_tp=%u, tag=%u, tr->var_ctx=%u, tr->var_nbr=%u\n", j, i, q->undo_hi_tp, e->c.tag, tr->var_ctx, tr->var_nbr);
		cell lhs, rhs;
		make_ref(&lhs, g_anon_s, tr->var_nbr, tr->var_ctx);
		rhs = e->c;
		//DUMP_TERM("$undo1 rhs", &e->c, e->c.var_ctx, 0);
		cell tmp[3];
		make_struct(tmp, g_minus_s, NULL, 2, 2);
		SET_OP(&tmp[0], OP_YFX);
		tmp[1] = lhs;
		tmp[2] = rhs;
		append_list(q, tmp);
		init_cell(&e->c);
		e->c.attrs = tr->attrs;
		e->c.attrs_ctx = tr->attrs_ctx;
		//if (tr->attrs) DUMP_TERM("$undo2 trail", tr->attrs, tr->attrs_ctx, 0);
	}

	cell *tmp = end_list(q);
	check_heap_error(tmp, free(save));
	//DUMP_TERM("undolist tmp", tmp, q->st.curr_frame, 0);

	if (!unify(q, p1, p1_ctx, tmp, q->st.curr_frame))
		return false;

	cell tmp2 = {0};
	tmp2.tag = TAG_BLOB;
	tmp2.flags = FLAG_MANAGED;
	tmp2.nbr_cells = 1;
	tmp2.val_blob = &save->b;
	tmp2.val_blob->refcnt = 0;
	return unify(q, p2, p2_ctx, &tmp2, q->st.curr_frame);
}

bool bif_sys_redo_trail_1(query * q)
{
	GET_FIRST_ARG(p1,any);

	if (!is_blob(p1))
		return true;

	const bind_state *save = (bind_state*)p1->val_blob;

	for (pl_idx i = save->lo_tp, j = 0; i < save->hi_tp; i++, j++) {
		if (is_empty(&save->e[j].c))
			continue;

		const trail *tr = q->trails + i;
		const frame *f = GET_FRAME(tr->var_ctx);
		slot *e = GET_SLOT(f, tr->var_nbr);
		*e = save->e[j];
		//printf("*** rebind [%u:%u] hi_tp=%u, tag=%u, ctx=%u, var=%u\n", j, i, q->undo_hi_tp, e->c.tag, tr->var_ctx, tr->var_nbr);
		//DUMP_TERM("$redo1 rhs", &e->c, e->c.var_ctx, 0);
	}

	return true;
}

bool do_post_unification_hook(query *q, bool is_builtin)
{
	q->run_hook = false;
	q->undo_lo_tp = q->before_hook_tp;
	q->undo_hi_tp = q->st.tp;
	q->before_hook_tp = 0;

	cell *tmp = alloc_on_heap(q, 3);
	check_heap_error(tmp);
	// Needed for follow() to work
	tmp[0].tag = TAG_INTERNED;
	tmp[0].arity = 0;
	tmp[0].nbr_cells = 1;
	tmp[0].flags = FLAG_BUILTIN;
	tmp[0].val_off = g_true_s;

	static void *s_fn_ptr1 = NULL;

	if (!s_fn_ptr1)
		s_fn_ptr1 = get_fn_ptr(bif_iso_true_0);

	tmp[0].bif_ptr = s_fn_ptr1;

	tmp[1].tag = TAG_INTERNED;
	tmp[1].nbr_cells = 1;
	tmp[1].arity = 0;
	tmp[1].flags = 0;
	tmp[1].val_off = g_post_unify_hook_s;

	static void *s_fn_ptr2 = NULL;

	if (!s_fn_ptr2)
		s_fn_ptr2 = search_predicate(q->st.m, tmp+1, NULL);

	tmp[1].match = s_fn_ptr2;

	if (!tmp[1].match)
		return throw_error(q, tmp+1, q->st.curr_frame, "existence_error", "procedure");

	if (is_builtin)
		make_call(q, tmp+2);
	else
		make_call_redo(q, tmp+2);

	q->st.curr_cell = tmp;
	return true;
}

