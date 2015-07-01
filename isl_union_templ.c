/*
 * Copyright 2010      INRIA Saclay
 * Copyright 2013      Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 * and Ecole Normale Superieure, 45 rue d'Ulm, 75230 Paris, France
 */

#include <isl_hash_private.h>
#include <isl_union_macro.h>

/* A union of expressions defined over different domain spaces.
 * "space" describes the parameters.
 * The entries of "table" are keyed on the domain space of the entry.
 */
struct UNION {
	int ref;
#ifdef HAS_TYPE
	enum isl_fold type;
#endif
	isl_space *space;

	struct isl_hash_table	table;
};

__isl_give UNION *FN(UNION,cow)(__isl_take UNION *u);

isl_ctx *FN(UNION,get_ctx)(__isl_keep UNION *u)
{
	return u ? u->space->ctx : NULL;
}

__isl_give isl_space *FN(UNION,get_space)(__isl_keep UNION *u)
{
	if (!u)
		return NULL;
	return isl_space_copy(u->space);
}

/* Return the number of parameters of "u", where "type"
 * is required to be set to isl_dim_param.
 */
unsigned FN(UNION,dim)(__isl_keep UNION *u, enum isl_dim_type type)
{
	if (!u)
		return 0;

	if (type != isl_dim_param)
		isl_die(FN(UNION,get_ctx)(u), isl_error_invalid,
			"can only reference parameters", return 0);

	return isl_space_dim(u->space, type);
}

/* Return the position of the parameter with the given name
 * in "u".
 * Return -1 if no such dimension can be found.
 */
int FN(UNION,find_dim_by_name)(__isl_keep UNION *u, enum isl_dim_type type,
	const char *name)
{
	if (!u)
		return -1;
	return isl_space_find_dim_by_name(u->space, type, name);
}

#ifdef HAS_TYPE
static __isl_give UNION *FN(UNION,alloc)(__isl_take isl_space *dim,
	enum isl_fold type, int size)
#else
static __isl_give UNION *FN(UNION,alloc)(__isl_take isl_space *dim, int size)
#endif
{
	UNION *u;

	dim = isl_space_params(dim);
	if (!dim)
		return NULL;

	u = isl_calloc_type(dim->ctx, UNION);
	if (!u)
		goto error;

	u->ref = 1;
#ifdef HAS_TYPE
	u->type = type;
#endif
	u->space = dim;
	if (isl_hash_table_init(dim->ctx, &u->table, size) < 0)
		return FN(UNION,free)(u);

	return u;
error:
	isl_space_free(dim);
	return NULL;
}

#ifdef HAS_TYPE
__isl_give UNION *FN(UNION,ZERO)(__isl_take isl_space *dim, enum isl_fold type)
{
	return FN(UNION,alloc)(dim, type, 16);
}
#else
__isl_give UNION *FN(UNION,ZERO)(__isl_take isl_space *dim)
{
	return FN(UNION,alloc)(dim, 16);
}
#endif

__isl_give UNION *FN(UNION,copy)(__isl_keep UNION *u)
{
	if (!u)
		return NULL;

	u->ref++;
	return u;
}

/* Return the number of base expressions in "u".
 */
int FN(FN(UNION,n),PARTS)(__isl_keep UNION *u)
{
	return u ? u->table.n : 0;
}

S(UNION,foreach_data)
{
	isl_stat (*fn)(__isl_take PART *part, void *user);
	void *user;
};

static isl_stat FN(UNION,call_on_copy)(void **entry, void *user)
{
	PART *part = *entry;
	S(UNION,foreach_data) *data = (S(UNION,foreach_data) *)user;

	return data->fn(FN(PART,copy)(part), data->user);
}

isl_stat FN(FN(UNION,foreach),PARTS)(__isl_keep UNION *u,
	isl_stat (*fn)(__isl_take PART *part, void *user), void *user)
{
	S(UNION,foreach_data) data = { fn, user };

	if (!u)
		return isl_stat_error;

	return isl_hash_table_foreach(u->space->ctx, &u->table,
				      &FN(UNION,call_on_copy), &data);
}

/* Is the domain space of "entry" equal to the domain of "space"?
 */
static int FN(UNION,has_same_domain_space)(const void *entry, const void *val)
{
	PART *part = (PART *)entry;
	isl_space *space = (isl_space *) val;

	if (isl_space_is_set(space))
		return isl_space_is_set(part->dim);

	return isl_space_tuple_is_equal(part->dim, isl_dim_in,
					space, isl_dim_in);
}

/* Return the entry, if any, in "u" that lives in "space".
 * If "reserve" is set, then an entry is created if it does not exist yet.
 * Return NULL on error and isl_hash_table_entry_none if no entry was found.
 * Note that when "reserve" is set, the function will never return
 * isl_hash_table_entry_none.
 *
 * First look for the entry (if any) with the same domain space.
 * If it exists, then check if the range space also matches.
 */
static struct isl_hash_table_entry *FN(UNION,find_part_entry)(
	__isl_keep UNION *u, __isl_keep isl_space *space, int reserve)
{
	isl_ctx *ctx;
	uint32_t hash;
	struct isl_hash_table_entry *entry;
	isl_bool equal;
	PART *part;

	if (!u || !space)
		return NULL;

	ctx = FN(UNION,get_ctx)(u);
	hash = isl_space_get_domain_hash(space);
	entry = isl_hash_table_find(ctx, &u->table, hash,
			&FN(UNION,has_same_domain_space), space, reserve);
	if (!entry)
		return reserve ? NULL : isl_hash_table_entry_none;
	if (reserve && !entry->data)
		return entry;
	part = entry->data;
	equal = isl_space_tuple_is_equal(part->dim, isl_dim_out,
					    space, isl_dim_out);
	if (equal < 0)
		return NULL;
	if (equal)
		return entry;
	if (!reserve)
		return isl_hash_table_entry_none;
	isl_die(FN(UNION,get_ctx)(u), isl_error_invalid,
		"union expression can only contain a single "
		"expression over a given domain", return NULL);
}

/* Remove "part_entry" from the hash table of "u".
 */
static __isl_give UNION *FN(UNION,remove_part_entry)(__isl_take UNION *u,
	struct isl_hash_table_entry *part_entry)
{
	isl_ctx *ctx;

	if (!u || !part_entry)
		return FN(UNION,free)(u);

	ctx = FN(UNION,get_ctx)(u);
	isl_hash_table_remove(ctx, &u->table, part_entry);
	FN(PART,free)(part_entry->data);

	return u;
}

/* Extract the element of "u" living in "space" (ignoring parameters).
 *
 * Return the ZERO element if "u" does not contain any element
 * living in "space".
 */
__isl_give PART *FN(FN(UNION,extract),PARTS)(__isl_keep UNION *u,
	__isl_take isl_space *space)
{
	struct isl_hash_table_entry *entry;

	if (!u || !space)
		goto error;
	if (!isl_space_match(u->space, isl_dim_param, space, isl_dim_param)) {
		space = isl_space_drop_dims(space, isl_dim_param,
					0, isl_space_dim(space, isl_dim_param));
		space = isl_space_align_params(space,
					FN(UNION,get_space)(u));
		if (!space)
			goto error;
	}

	entry = FN(UNION,find_part_entry)(u, space, 0);
	if (!entry)
		goto error;
	if (entry == isl_hash_table_entry_none)
#ifdef HAS_TYPE
		return FN(PART,ZERO)(space, u->type);
#else
		return FN(PART,ZERO)(space);
#endif
	isl_space_free(space);
	return FN(PART,copy)(entry->data);
error:
	isl_space_free(space);
	return NULL;
}

/* Add "part" to "u".
 * If "disjoint" is set, then "u" is not allowed to already have
 * a part that is defined on the same space as "part".
 * Otherwise, compute the union sum of "part" and the part in "u"
 * defined on the same space.
 */
static __isl_give UNION *FN(UNION,add_part_generic)(__isl_take UNION *u,
	__isl_take PART *part, int disjoint)
{
	int empty;
	struct isl_hash_table_entry *entry;

	if (!part)
		goto error;

	empty = FN(PART,IS_ZERO)(part);
	if (empty < 0)
		goto error;
	if (empty) {
		FN(PART,free)(part);
		return u;
	}

	u = FN(UNION,align_params)(u, FN(PART,get_space)(part));
	part = FN(PART,align_params)(part, FN(UNION,get_space)(u));

	u = FN(UNION,cow)(u);

	if (!u)
		goto error;

	entry = FN(UNION,find_part_entry)(u, part->dim, 1);
	if (!entry)
		goto error;

	if (!entry->data)
		entry->data = part;
	else {
		if (disjoint)
			isl_die(FN(UNION,get_ctx)(u), isl_error_invalid,
				"additional part should live on separate "
				"space", goto error);
		entry->data = FN(PART,union_add_)(entry->data,
						FN(PART,copy)(part));
		if (!entry->data)
			goto error;
		empty = FN(PART,IS_ZERO)(part);
		if (empty < 0)
			goto error;
		if (empty)
			u = FN(UNION,remove_part_entry)(u, entry);
		FN(PART,free)(part);
	}

	return u;
error:
	FN(PART,free)(part);
	FN(UNION,free)(u);
	return NULL;
}

/* Add "part" to "u", where "u" is assumed not to already have
 * a part that is defined on the same space as "part".
 */
__isl_give UNION *FN(FN(UNION,add),PARTS)(__isl_take UNION *u,
	__isl_take PART *part)
{
	return FN(UNION,add_part_generic)(u, part, 1);
}

#ifdef HAS_TYPE
/* Allocate a UNION with the same type and the same size as "u" and
 * with space "space".
 */
static __isl_give UNION *FN(UNION,alloc_same_size_on_space)(__isl_keep UNION *u,
	__isl_take isl_space *space)
{
	if (!u)
		space = isl_space_free(space);
	return FN(UNION,alloc)(space, u->type, u->table.n);
}
#else
/* Allocate a UNION with the same size as "u" and with space "space".
 */
static __isl_give UNION *FN(UNION,alloc_same_size_on_space)(__isl_keep UNION *u,
	__isl_take isl_space *space)
{
	if (!u)
		space = isl_space_free(space);
	return FN(UNION,alloc)(space, u->table.n);
}
#endif

/* Allocate a UNION with the same space, the same type (if any) and
 * the same size as "u".
 */
static __isl_give UNION *FN(UNION,alloc_same_size)(__isl_keep UNION *u)
{
	return FN(UNION,alloc_same_size_on_space)(u, FN(UNION,get_space)(u));
}

/* Call "fn" on each part entry of "u".
 */
static isl_stat FN(UNION,foreach_inplace)(__isl_keep UNION *u,
	isl_stat (*fn)(void **part, void *user), void *user)
{
	isl_ctx *ctx;

	if (!u)
		return isl_stat_error;
	ctx = FN(UNION,get_ctx)(u);
	return isl_hash_table_foreach(ctx, &u->table, fn, user);
}

static isl_stat FN(UNION,add_part)(__isl_take PART *part, void *user)
{
	UNION **u = (UNION **)user;

	*u = FN(FN(UNION,add),PARTS)(*u, part);

	return isl_stat_ok;
}

__isl_give UNION *FN(UNION,dup)(__isl_keep UNION *u)
{
	UNION *dup;

	if (!u)
		return NULL;

	dup = FN(UNION,alloc_same_size)(u);
	if (FN(FN(UNION,foreach),PARTS)(u, &FN(UNION,add_part), &dup) < 0)
		goto error;
	return dup;
error:
	FN(UNION,free)(dup);
	return NULL;
}

__isl_give UNION *FN(UNION,cow)(__isl_take UNION *u)
{
	if (!u)
		return NULL;

	if (u->ref == 1)
		return u;
	u->ref--;
	return FN(UNION,dup)(u);
}

static isl_stat FN(UNION,free_u_entry)(void **entry, void *user)
{
	PART *part = *entry;
	FN(PART,free)(part);
	return isl_stat_ok;
}

__isl_null UNION *FN(UNION,free)(__isl_take UNION *u)
{
	if (!u)
		return NULL;

	if (--u->ref > 0)
		return NULL;

	isl_hash_table_foreach(u->space->ctx, &u->table,
				&FN(UNION,free_u_entry), NULL);
	isl_hash_table_clear(&u->table);
	isl_space_free(u->space);
	free(u);
	return NULL;
}

S(UNION,align) {
	isl_reordering *exp;
	UNION *res;
};

static isl_stat FN(UNION,align_entry)(__isl_take PART *part, void *user)
{
	isl_reordering *exp;
	S(UNION,align) *data = user;

	exp = isl_reordering_extend_space(isl_reordering_copy(data->exp),
				    FN(PART,get_domain_space)(part));

	data->res = FN(FN(UNION,add),PARTS)(data->res,
					    FN(PART,realign_domain)(part, exp));

	return isl_stat_ok;
}

/* Reorder the parameters of "u" according to the given reordering.
 */
static __isl_give UNION *FN(UNION,realign_domain)(__isl_take UNION *u,
	__isl_take isl_reordering *r)
{
	S(UNION,align) data = { NULL, NULL };
	isl_space *space;

	if (!u || !r)
		goto error;

	space = isl_space_copy(r->dim);
	data.res = FN(UNION,alloc_same_size_on_space)(u, space);
	data.exp = r;
	if (FN(FN(UNION,foreach),PARTS)(u, &FN(UNION,align_entry), &data) < 0)
		data.res = FN(UNION,free)(data.res);

	isl_reordering_free(data.exp);
	FN(UNION,free)(u);
	return data.res;
error:
	FN(UNION,free)(u);
	isl_reordering_free(r);
	return NULL;
}

/* Align the parameters of "u" to those of "model".
 */
__isl_give UNION *FN(UNION,align_params)(__isl_take UNION *u,
	__isl_take isl_space *model)
{
	isl_reordering *r;

	if (!u || !model)
		goto error;

	if (isl_space_match(u->space, isl_dim_param, model, isl_dim_param)) {
		isl_space_free(model);
		return u;
	}

	model = isl_space_params(model);
	r = isl_parameter_alignment_reordering(u->space, model);
	isl_space_free(model);

	return FN(UNION,realign_domain)(u, r);
error:
	isl_space_free(model);
	FN(UNION,free)(u);
	return NULL;
}

/* Add "part" to *u, taking the union sum if "u" already has
 * a part defined on the same space as "part".
 */
static isl_stat FN(UNION,union_add_part)(__isl_take PART *part, void *user)
{
	UNION **u = (UNION **)user;

	*u = FN(UNION,add_part_generic)(*u, part, 0);

	return isl_stat_ok;
}

/* Compute the sum of "u1" and "u2" on the union of their domains,
 * with the actual sum on the shared domain and
 * the defined expression on the symmetric difference of the domains.
 *
 * This is an internal function that is exposed under different
 * names depending on whether the base expressions have a zero default
 * value.
 * If they do, then this function is called "add".
 * Otherwise, it is called "union_add".
 */
static __isl_give UNION *FN(UNION,union_add_)(__isl_take UNION *u1,
	__isl_take UNION *u2)
{
	u1 = FN(UNION,align_params)(u1, FN(UNION,get_space)(u2));
	u2 = FN(UNION,align_params)(u2, FN(UNION,get_space)(u1));

	u1 = FN(UNION,cow)(u1);

	if (!u1 || !u2)
		goto error;

	if (FN(FN(UNION,foreach),PARTS)(u2, &FN(UNION,union_add_part), &u1) < 0)
		goto error;

	FN(UNION,free)(u2);

	return u1;
error:
	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	return NULL;
}

__isl_give UNION *FN(FN(UNION,from),PARTS)(__isl_take PART *part)
{
	isl_space *dim;
	UNION *u;

	if (!part)
		return NULL;

	dim = FN(PART,get_space)(part);
	dim = isl_space_drop_dims(dim, isl_dim_in, 0, isl_space_dim(dim, isl_dim_in));
	dim = isl_space_drop_dims(dim, isl_dim_out, 0, isl_space_dim(dim, isl_dim_out));
#ifdef HAS_TYPE
	u = FN(UNION,ZERO)(dim, part->type);
#else
	u = FN(UNION,ZERO)(dim);
#endif
	u = FN(FN(UNION,add),PARTS)(u, part);

	return u;
}

S(UNION,match_bin_data) {
	UNION *u2;
	UNION *res;
	__isl_give PART *(*fn)(__isl_take PART *, __isl_take PART *);
};

/* Check if data->u2 has an element living in the same space as *entry.
 * If so, call data->fn on the two elements and add the result to
 * data->res.
 */
static isl_stat FN(UNION,match_bin_entry)(void **entry, void *user)
{
	S(UNION,match_bin_data) *data = user;
	struct isl_hash_table_entry *entry2;
	isl_space *space;
	PART *part = *entry;
	PART *part2;

	space = FN(PART,get_space)(part);
	entry2 = FN(UNION,find_part_entry)(data->u2, space, 0);
	isl_space_free(space);
	if (!entry2)
		return isl_stat_error;
	if (entry2 == isl_hash_table_entry_none)
		return isl_stat_ok;

	part2 = entry2->data;
	if (!isl_space_tuple_is_equal(part->dim, isl_dim_out,
					part2->dim, isl_dim_out))
		isl_die(FN(UNION,get_ctx)(data->u2), isl_error_invalid,
			"entries should have the same range space",
			return isl_stat_error);

	part = FN(PART, copy)(part);
	part = data->fn(part, FN(PART, copy)(entry2->data));

	data->res = FN(FN(UNION,add),PARTS)(data->res, part);
	if (!data->res)
		return isl_stat_error;

	return isl_stat_ok;
}

/* This function is currently only used from isl_polynomial.c
 * and not from isl_fold.c.
 */
static __isl_give UNION *FN(UNION,match_bin_op)(__isl_take UNION *u1,
	__isl_take UNION *u2,
	__isl_give PART *(*fn)(__isl_take PART *, __isl_take PART *))
	__attribute__ ((unused));
/* For each pair of elements in "u1" and "u2" living in the same space,
 * call "fn" and collect the results.
 */
static __isl_give UNION *FN(UNION,match_bin_op)(__isl_take UNION *u1,
	__isl_take UNION *u2,
	__isl_give PART *(*fn)(__isl_take PART *, __isl_take PART *))
{
	S(UNION,match_bin_data) data = { NULL, NULL, fn };

	u1 = FN(UNION,align_params)(u1, FN(UNION,get_space)(u2));
	u2 = FN(UNION,align_params)(u2, FN(UNION,get_space)(u1));

	if (!u1 || !u2)
		goto error;

	data.u2 = u2;
	data.res = FN(UNION,alloc_same_size)(u1);
	if (isl_hash_table_foreach(u1->space->ctx, &u1->table,
				    &FN(UNION,match_bin_entry), &data) < 0)
		goto error;

	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	return data.res;
error:
	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	FN(UNION,free)(data.res);
	return NULL;
}

/* Compute the sum of "u1" and "u2".
 *
 * If the base expressions have a default zero value, then the sum
 * is computed on the union of the domains of "u1" and "u2".
 * Otherwise, it is computed on their shared domains.
 */
__isl_give UNION *FN(UNION,add)(__isl_take UNION *u1, __isl_take UNION *u2)
{
#if DEFAULT_IS_ZERO
	return FN(UNION,union_add_)(u1, u2);
#else
	return FN(UNION,match_bin_op)(u1, u2, &FN(PART,add));
#endif
}

#ifndef NO_SUB
/* Subtract "u2" from "u1" and return the result.
 */
__isl_give UNION *FN(UNION,sub)(__isl_take UNION *u1, __isl_take UNION *u2)
{
	return FN(UNION,match_bin_op)(u1, u2, &FN(PART,sub));
}
#endif

S(UNION,any_set_data) {
	isl_set *set;
	UNION *res;
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*);
};

static isl_stat FN(UNION,any_set_entry)(void **entry, void *user)
{
	S(UNION,any_set_data) *data = user;
	PW *pw = *entry;

	pw = FN(PW,copy)(pw);
	pw = data->fn(pw, isl_set_copy(data->set));

	data->res = FN(FN(UNION,add),PARTS)(data->res, pw);
	if (!data->res)
		return isl_stat_error;

	return isl_stat_ok;
}

/* Update each element of "u" by calling "fn" on the element and "set".
 */
static __isl_give UNION *FN(UNION,any_set_op)(__isl_take UNION *u,
	__isl_take isl_set *set,
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*))
{
	S(UNION,any_set_data) data = { NULL, NULL, fn };

	u = FN(UNION,align_params)(u, isl_set_get_space(set));
	set = isl_set_align_params(set, FN(UNION,get_space)(u));

	if (!u || !set)
		goto error;

	data.set = set;
	data.res = FN(UNION,alloc_same_size)(u);
	if (isl_hash_table_foreach(u->space->ctx, &u->table,
				   &FN(UNION,any_set_entry), &data) < 0)
		goto error;

	FN(UNION,free)(u);
	isl_set_free(set);
	return data.res;
error:
	FN(UNION,free)(u);
	isl_set_free(set);
	FN(UNION,free)(data.res);
	return NULL;
}

/* Intersect the domain of "u" with the parameter domain "context".
 */
__isl_give UNION *FN(UNION,intersect_params)(__isl_take UNION *u,
	__isl_take isl_set *set)
{
	return FN(UNION,any_set_op)(u, set, &FN(PW,intersect_params));
}

/* Compute the gist of the domain of "u" with respect to
 * the parameter domain "context".
 */
__isl_give UNION *FN(UNION,gist_params)(__isl_take UNION *u,
	__isl_take isl_set *set)
{
	return FN(UNION,any_set_op)(u, set, &FN(PW,gist_params));
}

S(UNION,match_domain_data) {
	isl_union_set *uset;
	UNION *res;
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*);
};

static int FN(UNION,set_has_dim)(const void *entry, const void *val)
{
	isl_set *set = (isl_set *)entry;
	isl_space *dim = (isl_space *)val;

	return isl_space_is_equal(set->dim, dim);
}

/* Find the set in data->uset that lives in the same space as the domain
 * of *entry, apply data->fn to *entry and this set (if any), and add
 * the result to data->res.
 */
static isl_stat FN(UNION,match_domain_entry)(void **entry, void *user)
{
	S(UNION,match_domain_data) *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	PW *pw = *entry;
	isl_space *space;

	space = FN(PW,get_domain_space)(pw);
	hash = isl_space_get_hash(space);
	entry2 = isl_hash_table_find(data->uset->dim->ctx, &data->uset->table,
				     hash, &FN(UNION,set_has_dim), space, 0);
	isl_space_free(space);
	if (!entry2)
		return isl_stat_ok;

	pw = FN(PW,copy)(pw);
	pw = data->fn(pw, isl_set_copy(entry2->data));

	data->res = FN(FN(UNION,add),PARTS)(data->res, pw);
	if (!data->res)
		return isl_stat_error;

	return isl_stat_ok;
}

/* Apply fn to each pair of PW in u and set in uset such that
 * the set lives in the same space as the domain of PW
 * and collect the results.
 */
static __isl_give UNION *FN(UNION,match_domain_op)(__isl_take UNION *u,
	__isl_take isl_union_set *uset,
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*))
{
	S(UNION,match_domain_data) data = { NULL, NULL, fn };

	u = FN(UNION,align_params)(u, isl_union_set_get_space(uset));
	uset = isl_union_set_align_params(uset, FN(UNION,get_space)(u));

	if (!u || !uset)
		goto error;

	data.uset = uset;
	data.res = FN(UNION,alloc_same_size)(u);
	if (isl_hash_table_foreach(u->space->ctx, &u->table,
				   &FN(UNION,match_domain_entry), &data) < 0)
		goto error;

	FN(UNION,free)(u);
	isl_union_set_free(uset);
	return data.res;
error:
	FN(UNION,free)(u);
	isl_union_set_free(uset);
	FN(UNION,free)(data.res);
	return NULL;
}

/* Intersect the domain of "u" with "uset".
 * If "uset" is a parameters domain, then intersect the parameter
 * domain of "u" with this set.
 */
__isl_give UNION *FN(UNION,intersect_domain)(__isl_take UNION *u,
	__isl_take isl_union_set *uset)
{
	if (isl_union_set_is_params(uset))
		return FN(UNION,intersect_params)(u,
						isl_set_from_union_set(uset));
	return FN(UNION,match_domain_op)(u, uset, &FN(PW,intersect_domain));
}

/* Internal data structure for isl_union_*_subtract_domain.
 * uset is the set that needs to be removed from the domain.
 * res collects the results.
 */
S(UNION,subtract_domain_data) {
	isl_union_set *uset;
	UNION *res;
};

/* Take the set (which may be empty) in data->uset that lives
 * in the same space as the domain of "pw", subtract it from the domain
 * of "pw" and add the result to data->res.
 */
static isl_stat FN(UNION,subtract_domain_entry)(__isl_take PW *pw, void *user)
{
	S(UNION,subtract_domain_data) *data = user;
	isl_space *space;
	isl_set *set;

	space = FN(PW,get_domain_space)(pw);
	set = isl_union_set_extract_set(data->uset, space);
	pw = FN(PW,subtract_domain)(pw, set);
	data->res = FN(FN(UNION,add),PARTS)(data->res, pw);

	return isl_stat_ok;
}

/* Subtract "uset' from the domain of "u".
 */
__isl_give UNION *FN(UNION,subtract_domain)(__isl_take UNION *u,
	__isl_take isl_union_set *uset)
{
	S(UNION,subtract_domain_data) data;

	if (!u || !uset)
		goto error;

	data.uset = uset;
	data.res = FN(UNION,alloc_same_size)(u);
	if (FN(FN(UNION,foreach),PARTS)(u,
				&FN(UNION,subtract_domain_entry), &data) < 0)
		data.res = FN(UNION,free)(data.res);

	FN(UNION,free)(u);
	isl_union_set_free(uset);
	return data.res;
error:
	FN(UNION,free)(u);
	isl_union_set_free(uset);
	return NULL;
}

__isl_give UNION *FN(UNION,gist)(__isl_take UNION *u,
	__isl_take isl_union_set *uset)
{
	if (isl_union_set_is_params(uset))
		return FN(UNION,gist_params)(u, isl_set_from_union_set(uset));
	return FN(UNION,match_domain_op)(u, uset, &FN(PW,gist));
}

/* Coalesce an entry in a UNION.  Coalescing is performed in-place.
 * Since the UNION may have several references, the entry is only
 * replaced if the coalescing is successful.
 */
static isl_stat FN(UNION,coalesce_entry)(void **entry, void *user)
{
	PART **part_p = (PART **) entry;
	PART *part;

	part = FN(PART,copy)(*part_p);
	part = FN(PW,coalesce)(part);
	if (!part)
		return isl_stat_error;
	FN(PART,free)(*part_p);
	*part_p = part;

	return isl_stat_ok;
}

__isl_give UNION *FN(UNION,coalesce)(__isl_take UNION *u)
{
	if (FN(UNION,foreach_inplace)(u, &FN(UNION,coalesce_entry), NULL) < 0)
		goto error;

	return u;
error:
	FN(UNION,free)(u);
	return NULL;
}

static isl_stat FN(UNION,domain_entry)(__isl_take PART *part, void *user)
{
	isl_union_set **uset = (isl_union_set **)user;

	*uset = isl_union_set_add_set(*uset, FN(PART,domain)(part));

	return isl_stat_ok;
}

__isl_give isl_union_set *FN(UNION,domain)(__isl_take UNION *u)
{
	isl_union_set *uset;

	uset = isl_union_set_empty(FN(UNION,get_space)(u));
	if (FN(FN(UNION,foreach),PARTS)(u, &FN(UNION,domain_entry), &uset) < 0)
		goto error;

	FN(UNION,free)(u);
	
	return uset;
error:
	isl_union_set_free(uset);
	FN(UNION,free)(u);
	return NULL;
}

#ifdef HAS_TYPE
/* Negate the type of "u".
 */
static __isl_give UNION *FN(UNION,negate_type)(__isl_take UNION *u)
{
	u = FN(UNION,cow)(u);
	if (!u)
		return NULL;
	u->type = isl_fold_type_negate(u->type);
	return u;
}
#else
/* Negate the type of "u".
 * Since "u" does not have a type, do nothing.
 */
static __isl_give UNION *FN(UNION,negate_type)(__isl_take UNION *u)
{
	return u;
}
#endif

static isl_stat FN(UNION,mul_isl_int_entry)(void **entry, void *user)
{
	PW **pw = (PW **)entry;
	isl_int *v = user;

	*pw = FN(PW,mul_isl_int)(*pw, *v);
	if (!*pw)
		return isl_stat_error;

	return isl_stat_ok;
}

__isl_give UNION *FN(UNION,mul_isl_int)(__isl_take UNION *u, isl_int v)
{
	if (isl_int_is_one(v))
		return u;

	if (DEFAULT_IS_ZERO && u && isl_int_is_zero(v)) {
		UNION *zero;
		isl_space *dim = FN(UNION,get_space)(u);
#ifdef HAS_TYPE
		zero = FN(UNION,ZERO)(dim, u->type);
#else
		zero = FN(UNION,ZERO)(dim);
#endif
		FN(UNION,free)(u);
		return zero;
	}

	u = FN(UNION,cow)(u);
	if (isl_int_is_neg(v))
		u = FN(UNION,negate_type)(u);
	if (!u)
		return NULL;

	if (isl_hash_table_foreach(u->space->ctx, &u->table,
				    &FN(UNION,mul_isl_int_entry), &v) < 0)
		goto error;

	return u;
error:
	FN(UNION,free)(u);
	return NULL;
}

/* Multiply *entry by the isl_val "user".
 *
 * Return 0 on success and -1 on error.
 */
static isl_stat FN(UNION,scale_val_entry)(void **entry, void *user)
{
	PW **pw = (PW **)entry;
	isl_val *v = user;

	*pw = FN(PW,scale_val)(*pw, isl_val_copy(v));
	if (!*pw)
		return isl_stat_error;

	return isl_stat_ok;
}

/* Multiply "u" by "v" and return the result.
 */
__isl_give UNION *FN(UNION,scale_val)(__isl_take UNION *u,
	__isl_take isl_val *v)
{
	if (!u || !v)
		goto error;
	if (isl_val_is_one(v)) {
		isl_val_free(v);
		return u;
	}

	if (DEFAULT_IS_ZERO && u && isl_val_is_zero(v)) {
		UNION *zero;
		isl_space *space = FN(UNION,get_space)(u);
#ifdef HAS_TYPE
		zero = FN(UNION,ZERO)(space, u->type);
#else
		zero = FN(UNION,ZERO)(space);
#endif
		FN(UNION,free)(u);
		isl_val_free(v);
		return zero;
	}

	if (!isl_val_is_rat(v))
		isl_die(isl_val_get_ctx(v), isl_error_invalid,
			"expecting rational factor", goto error);

	u = FN(UNION,cow)(u);
	if (isl_val_is_neg(v))
		u = FN(UNION,negate_type)(u);
	if (!u)
		return NULL;

	if (isl_hash_table_foreach(u->space->ctx, &u->table,
				    &FN(UNION,scale_val_entry), v) < 0)
		goto error;

	isl_val_free(v);
	return u;
error:
	isl_val_free(v);
	FN(UNION,free)(u);
	return NULL;
}

/* Divide *entry by the isl_val "user".
 *
 * Return 0 on success and -1 on error.
 */
static isl_stat FN(UNION,scale_down_val_entry)(void **entry, void *user)
{
	PW **pw = (PW **)entry;
	isl_val *v = user;

	*pw = FN(PW,scale_down_val)(*pw, isl_val_copy(v));
	if (!*pw)
		return isl_stat_error;

	return isl_stat_ok;
}

/* Divide "u" by "v" and return the result.
 */
__isl_give UNION *FN(UNION,scale_down_val)(__isl_take UNION *u,
	__isl_take isl_val *v)
{
	if (!u || !v)
		goto error;
	if (isl_val_is_one(v)) {
		isl_val_free(v);
		return u;
	}

	if (!isl_val_is_rat(v))
		isl_die(isl_val_get_ctx(v), isl_error_invalid,
			"expecting rational factor", goto error);
	if (isl_val_is_zero(v))
		isl_die(isl_val_get_ctx(v), isl_error_invalid,
			"cannot scale down by zero", goto error);

	u = FN(UNION,cow)(u);
	if (isl_val_is_neg(v))
		u = FN(UNION,negate_type)(u);
	if (!u)
		return NULL;

	if (isl_hash_table_foreach(FN(UNION,get_ctx)(u), &u->table,
				    &FN(UNION,scale_down_val_entry), v) < 0)
		goto error;

	isl_val_free(v);
	return u;
error:
	isl_val_free(v);
	FN(UNION,free)(u);
	return NULL;
}

S(UNION,plain_is_equal_data)
{
	UNION *u2;
	isl_bool is_equal;
};

static isl_stat FN(UNION,plain_is_equal_entry)(void **entry, void *user)
{
	S(UNION,plain_is_equal_data) *data = user;
	struct isl_hash_table_entry *entry2;
	PW *pw = *entry;

	entry2 = FN(UNION,find_part_entry)(data->u2, pw->dim, 0);
	if (!entry2 || entry2 == isl_hash_table_entry_none) {
		if (!entry2)
			data->is_equal = isl_bool_error;
		else
			data->is_equal = isl_bool_false;
		return isl_stat_error;
	}

	data->is_equal = FN(PW,plain_is_equal)(pw, entry2->data);
	if (data->is_equal < 0 || !data->is_equal)
		return isl_stat_error;

	return isl_stat_ok;
}

isl_bool FN(UNION,plain_is_equal)(__isl_keep UNION *u1, __isl_keep UNION *u2)
{
	S(UNION,plain_is_equal_data) data = { NULL, isl_bool_true };

	if (!u1 || !u2)
		return isl_bool_error;
	if (u1 == u2)
		return isl_bool_true;
	if (u1->table.n != u2->table.n)
		return isl_bool_false;

	u1 = FN(UNION,copy)(u1);
	u2 = FN(UNION,copy)(u2);
	u1 = FN(UNION,align_params)(u1, FN(UNION,get_space)(u2));
	u2 = FN(UNION,align_params)(u2, FN(UNION,get_space)(u1));
	if (!u1 || !u2)
		goto error;

	data.u2 = u2;
	if (FN(UNION,foreach_inplace)(u1,
			       &FN(UNION,plain_is_equal_entry), &data) < 0 &&
	    data.is_equal)
		goto error;

	FN(UNION,free)(u1);
	FN(UNION,free)(u2);

	return data.is_equal;
error:
	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	return isl_bool_error;
}

/* Internal data structure for isl_union_*_drop_dims.
 * type, first and n are passed to isl_*_drop_dims.
 * res collects the results.
 */
S(UNION,drop_dims_data) {
	enum isl_dim_type type;
	unsigned first;
	unsigned n;

	UNION *res;
};

/* Drop the parameters specified by "data" from "part" and
 * add the results to data->res.
 */
static isl_stat FN(UNION,drop_dims_entry)(__isl_take PART *part, void *user)
{
	S(UNION,drop_dims_data) *data = user;

	part = FN(PART,drop_dims)(part, data->type, data->first, data->n);
	data->res = FN(FN(UNION,add),PARTS)(data->res, part);
	if (!data->res)
		return isl_stat_error;

	return isl_stat_ok;
}

/* Drop the specified parameters from "u".
 * That is, type is required to be isl_dim_param.
 */
__isl_give UNION *FN(UNION,drop_dims)( __isl_take UNION *u,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	isl_space *space;
	S(UNION,drop_dims_data) data = { type, first, n };

	if (!u)
		return NULL;

	if (type != isl_dim_param)
		isl_die(FN(UNION,get_ctx)(u), isl_error_invalid,
			"can only project out parameters",
			return FN(UNION,free)(u));

	space = FN(UNION,get_space)(u);
	space = isl_space_drop_dims(space, type, first, n);
	data.res = FN(UNION,alloc_same_size_on_space)(u, space);
	if (FN(FN(UNION,foreach),PARTS)(u,
					&FN(UNION,drop_dims_entry), &data) < 0)
		data.res = FN(UNION,free)(data.res);

	FN(UNION,free)(u);

	return data.res;
}

/* Internal data structure for isl_union_*_set_dim_name.
 * pos is the position of the parameter that needs to be renamed.
 * s is the new name.
 * res collects the results.
 */
S(UNION,set_dim_name_data) {
	unsigned pos;
	const char *s;

	UNION *res;
};

/* Change the name of the parameter at position data->pos of "part" to data->s
 * and add the result to data->res.
 */
static isl_stat FN(UNION,set_dim_name_entry)(__isl_take PART *part, void *user)
{
	S(UNION,set_dim_name_data) *data = user;

	part = FN(PART,set_dim_name)(part, isl_dim_param, data->pos, data->s);
	data->res = FN(FN(UNION,add),PARTS)(data->res, part);
	if (!data->res)
		return isl_stat_error;

	return isl_stat_ok;
}

/* Change the name of the parameter at position "pos" to "s".
 * That is, type is required to be isl_dim_param.
 */
__isl_give UNION *FN(UNION,set_dim_name)(__isl_take UNION *u,
	enum isl_dim_type type, unsigned pos, const char *s)
{
	S(UNION,set_dim_name_data) data = { pos, s };
	isl_space *space;

	if (!u)
		return NULL;

	if (type != isl_dim_param)
		isl_die(FN(UNION,get_ctx)(u), isl_error_invalid,
			"can only set parameter names",
			return FN(UNION,free)(u));

	space = FN(UNION,get_space)(u);
	space = isl_space_set_dim_name(space, type, pos, s);
	data.res = FN(UNION,alloc_same_size_on_space)(u, space);

	if (FN(FN(UNION,foreach),PARTS)(u,
				    &FN(UNION,set_dim_name_entry), &data) < 0)
		data.res = FN(UNION,free)(data.res);

	FN(UNION,free)(u);

	return data.res;
}

/* Reset the user pointer on all identifiers of parameters and tuples
 * of the space of "part" and add the result to *res.
 */
static isl_stat FN(UNION,reset_user_entry)(__isl_take PART *part, void *user)
{
	UNION **res = user;

	part = FN(PART,reset_user)(part);
	*res = FN(FN(UNION,add),PARTS)(*res, part);
	if (!*res)
		return isl_stat_error;

	return isl_stat_ok;
}

/* Reset the user pointer on all identifiers of parameters and tuples
 * of the spaces of "u".
 */
__isl_give UNION *FN(UNION,reset_user)(__isl_take UNION *u)
{
	isl_space *space;
	UNION *res;

	space = FN(UNION,get_space)(u);
	space = isl_space_reset_user(space);
	res = FN(UNION,alloc_same_size_on_space)(u, space);
	if (FN(FN(UNION,foreach),PARTS)(u,
					&FN(UNION,reset_user_entry), &res) < 0)
		res = FN(UNION,free)(res);

	FN(UNION,free)(u);

	return res;
}
