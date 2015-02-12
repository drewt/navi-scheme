/* Copyright 2014-2015 Drew Thoreson
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "navi.h"
#include "navi/unicode.h"

static navi_t list_to_string(navi_t list, navi_env_t env)
{
	navi_t cons;
	size_t size = 0, length = 0;

	navi_list_for_each(cons, list) {
		navi_type_check(navi_car(cons), NAVI_CHAR, env);
		size += u8_length(navi_char(navi_car(cons)));
		length++;
	}

	int32_t i = 0;
	navi_t expr = navi_make_string(size, size, length);
	struct navi_string *str = navi_string(expr);

	navi_list_for_each(cons, list) {
		u8_append(str->data, i, str->capacity, navi_char(navi_car(cons)));
	}
	return expr;
}

static navi_t string_to_list(navi_t expr)
{
	struct navi_pair head, *ptr;
	struct navi_string *str = navi_string(expr);

	ptr = &head;
	for (int32_t i = 0; i < str->size;) {
		UChar32 ch;
		u8_next(str->data, i, str->size, ch);
		ptr->cdr = navi_make_empty_pair();
		ptr = navi_pair(ptr->cdr);
		ptr->car = navi_make_char(ch);
	}
	ptr->cdr = navi_make_nil();
	return head.cdr;
}

static navi_t string_to_vector(navi_t str_obj)
{
	struct navi_string *str = navi_string(str_obj);
	navi_t vec_obj = navi_make_vector(str->length);
	struct navi_vector *vec = navi_vector(vec_obj);
	int32_t si;
	size_t vi;

	for (si = 0, vi = 0; si < str->size; vi++) {
		UChar32 ch;
		assert(vi < vec->size);
		u8_next(str->data, si, str->size, ch);
		vec->data[vi] = navi_make_char(ch);
	}
	assert(vi == vec->size);
	return vec_obj;
}

static int32_t u32_size(struct navi_vector *vec)
{
	size_t size = 0;
	for (size_t i = 0; i < vec->size; i++) {
		assert(navi_type(vec->data[i]) == NAVI_CHAR);
		size += u8_length(navi_char(vec->data[i]));
	}
	return size;
}

static void vector_to_string_ip(struct navi_string *str,
		struct navi_vector *vec)
{
	size_t vi;
	int32_t si;

	for (si = 0, vi = 0; vi < vec->size; vi++) {
		u8_append(str->data, si, str->capacity, navi_char(vec->data[vi]));
	}
	str->size = si;
	str->data[si] = '\0';
}

static navi_t vector_to_string(navi_t vec_obj)
{
	int32_t size;
	navi_t str_obj;
	struct navi_string *str;
	struct navi_vector *vec = navi_vector(vec_obj);

	size = u32_size(vec);
	str_obj = navi_make_string(size, size, vec->size);
	str = navi_string(str_obj);

	vector_to_string_ip(str, vec);
	return str_obj;
}

DEFUN(scm_stringp, args, env)
{
	return navi_make_bool(navi_type(navi_car(args)) == NAVI_STRING);
}

DEFUN(scm_make_string, args, env)
{
	UChar32 ch = ' ';
	int nr_args = navi_list_length(args);
	int32_t length = navi_fixnum_cast(navi_car(args), env);
	if (length < 0)
		navi_error(env, "invalid length");
	if (nr_args > 1)
		ch = navi_char_cast(navi_cadr(args), env);

	int32_t size = length * u8_length(ch);
	navi_t obj = navi_make_string(size, size, length);
	struct navi_string *str = navi_string(obj);
	for (int32_t i = 0; i < size;) {
		u8_append(str->data, i, size, ch);
	}
	return obj;
}

DEFUN(scm_string, args, env)
{
	return list_to_string(args, env);
}

DEFUN(scm_string_length, args, env)
{
	return navi_make_num(navi_string_cast(navi_car(args), env)->length);
}

DEFUN(scm_string_ref, args, env)
{
	UChar32 ch;
	struct navi_string *str = navi_string_cast(navi_car(args), env);
	int32_t k = navi_type_check_range(navi_cadr(args), 0, str->length, env);

	u8_get(str->data, 0, k, str->size, ch);
	return navi_make_char(ch);
}

/*
 * Shift @str by @diff bytes, starting at @pos.  If @diff is negative, the
 * string is shifted left, with bytes disappearing beyond @pos.  Otherwise it's
 * shifted to the right.  This function is not UTF-8-aware; it is assumed that
 * @pos and @diff make sense for the task at hand.
 */
static void string_shift(struct navi_string *str, int32_t pos, int diff)
{
	if (diff > 0) {
		navi_string_grow_storage(str, diff);
		// shift right
		for (int32_t i = str->size-1; i > pos; i--)
			str->data[i+diff] = str->data[i];
	}
	else if (diff < 0) {
		// shift left
		for (int32_t i = pos+1; i < str->size-1; i++)
			str->data[i] = str->data[i-diff];
	}
}

void navi_string_set(struct navi_string *str, long k, UChar32 new_ch)
{
	int diff;
	UChar32 old_ch;
	int32_t i = 0;

	u8_fwd_n(str->data, i, str->size, k);
	u8_get(str->data, 0, i, str->size, old_ch);

	diff = u8_length(new_ch) - u8_length(old_ch);
	string_shift(str, i, diff);
	u8_append(str->data, i, str->size+diff, new_ch);
	str->size += diff;
	str->data[str->size] = '\0';
}

DEFUN(scm_string_set, args, env)
{
	int32_t k;
	struct navi_string *str;

	navi_type_check(navi_car(args),   NAVI_STRING, env);
	navi_type_check(navi_cadr(args),  NAVI_NUM,    env);
	navi_type_check(navi_caddr(args), NAVI_CHAR,   env);

	str = navi_string(navi_car(args));
	k = navi_num(navi_cadr(args));

	if (k < 0 || k >= str->size)
		navi_error(env, "string index out of bounds");

	navi_string_set(str, k, navi_char(navi_caddr(args)));
	return navi_unspecified();
}

static int navi_strcoll(struct navi_string *a, struct navi_string *b, bool ci)
{
	int result;
	UErrorCode error = U_ZERO_ERROR;
	UCollator *coll = ucol_open(NULL, &error); // FIXME: use system locale
	assert(U_SUCCESS(error));
	if (ci)
		ucol_setStrength(coll, UCOL_SECONDARY);
	result = ucol_strcollUTF8(coll, (char*)a->data, a->size, (char*)b->data,
			b->size, &error);
	assert(U_SUCCESS(error));
	ucol_close(coll);
	return result;
}

#define STRING_COMPARE(cname, op, ci) \
	DEFUN(cname, args, env) \
	{ \
		navi_t cons; \
		struct navi_string *fst, *snd; \
		fst = navi_string_cast(navi_car(args), env); \
		navi_list_for_each(cons, navi_cdr(args)) { \
			snd = navi_string_cast(navi_car(cons), env); \
			if (!(navi_strcoll(fst, snd, ci) op 0)) \
				return navi_make_bool(false); \
		} \
		return navi_make_bool(true); \
	}

STRING_COMPARE(scm_string_lt,     <,  false);
STRING_COMPARE(scm_string_gt,     >,  false);
STRING_COMPARE(scm_string_eq,     ==, false);
STRING_COMPARE(scm_string_lte,    <=, false);
STRING_COMPARE(scm_string_gte,    >=, false);
STRING_COMPARE(scm_string_ci_lt,  <,  true);
STRING_COMPARE(scm_string_ci_gt,  >,  true);
STRING_COMPARE(scm_string_ci_eq,  ==, true);
STRING_COMPARE(scm_string_ci_lte, <=, true);
STRING_COMPARE(scm_string_ci_gte, >=, true);

static int32_t count_length(struct navi_string *str)
{
	int32_t count = 0;
	for (int32_t i = 0; i < str->size; count++) {
		UChar32 ch;
		u8_next(str->data, i, str->size, ch);
	}
	return count;
}

#define STRING_CASEMAP(name, fun) \
	DEFUN(name, args, env) \
	{ \
		struct navi_string *src = navi_string_cast(navi_car(args), env); \
		navi_t dst_obj = navi_make_string(src->size, src->size, src->length); \
		struct navi_string *dst = navi_string(dst_obj); \
		UErrorCode error = U_ZERO_ERROR; \
		UCaseMap *map = ucasemap_open(NULL, 0, &error); \
		assert(!error); \
		do { \
			error = U_ZERO_ERROR; \
			navi_string_grow_storage(dst, dst->size - dst->capacity); \
			dst->size = fun(map, (char*)dst->data, dst->capacity, \
					(char*)src->data, src->size, &error); \
		} while (dst->size > dst->capacity); \
		assert(U_SUCCESS(error)); \
		dst->length = count_length(dst); \
		ucasemap_close(map); \
		return dst_obj; \
	}

STRING_CASEMAP(scm_string_upcase,   ucasemap_utf8ToUpper);
STRING_CASEMAP(scm_string_downcase, ucasemap_utf8ToLower);
STRING_CASEMAP(scm_string_foldcase, ucasemap_utf8FoldCase);

DEFUN(scm_substring, args, env)
{
	return scm_string_copy(args, env);
}

DEFUN(scm_string_append, args, env)
{
	navi_t cons, obj;
	struct navi_string *str;
	int32_t i = 0, size = 0, length = 0;

	// count combined size/length
	navi_list_for_each(cons, args) {
		str = navi_string_cast(navi_car(cons), env);
		size += str->size;
		length += str->length;
	}

	// allocate
	obj = navi_make_string(size, size, length);
	str = navi_string(obj);

	// copy
	navi_list_for_each(cons, args) {
		struct navi_string *other = navi_string(navi_car(cons));
		for (int32_t j = 0; j < other->size; j++)
			str->data[i++] = other->data[j];
	}
	return obj;
}

DEFUN(scm_string_to_list, args, env)
{
	return string_to_list(navi_type_check(navi_car(args), NAVI_STRING, env));
}

DEFUN(scm_list_to_string, args, env)
{
	return list_to_string(navi_type_check_list(navi_car(args), env), env);
}

static navi_t copy_to(navi_t to, int32_t at, navi_t from, int32_t start,
		int32_t end)
{
	UChar32 ch;
	int32_t k, j, to_i = 0, from_i = 0;
	int32_t to_size = 0, from_size = 0;
	struct navi_string *tos = navi_string(to), *froms = navi_string(from);

	u8_fwd_n(tos->data, to_i, tos->size, at);
	u8_fwd_n(froms->data, from_i, froms->size, start);

	// count size of to area
	for (j = to_i, k = 0; k < end - start; k++) {
		u8_next(tos->data, j, tos->size, ch);
		to_size += u8_length(ch);
	}
	// count size of from area
	for (j = from_i, k = 0; k < end - start; k++) {
		u8_next(froms->data, j, froms->size, ch);
		from_size += u8_length(ch);
	}
	// adjust target string
	string_shift(tos, to_i, from_size - to_size);
	memcpy(tos->data+to_i, froms->data+from_i, from_size);
	tos->size += from_size - to_size;
	tos->data[tos->size] = '\0';
	return to;
}

DEFUN(scm_string_fill, args, env)
{
	UChar32 ch;
	int32_t start, end, k, j, i = 0;
	int32_t new_size, old_size;
	struct navi_string *str;
	int nr_args = navi_list_length(args);

	str = navi_string_cast(navi_car(args), env);
	ch = navi_char_cast(navi_cadr(args), env);
	start = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env) : 0;
	end = (nr_args > 3) ? navi_fixnum_cast(navi_cadddr(args), env) : str->size;
	navi_check_copy(str->length, start, end, env);

	// determine old_size/new_size
	u8_fwd_n(str->data, i, str->size, start);
	for (k = 0, j = i; k < end - start; k++) {
		UChar32 old_ch;
		u8_next(str->data, j, str->size, old_ch);
	}
	old_size = j - i;
	new_size = u8_length(ch) * (end - start);

	string_shift(str, i, new_size - old_size);
	for (k = 0; k < end - start; k++) {
		u8_append(str->data, i, str->capacity, ch);
	}
	str->size += new_size - old_size;
	str->data[str->size] = '\0';
	return navi_unspecified();
}

navi_t navi_strdup(navi_t from_obj)
{
	struct navi_string *from = navi_string(from_obj);
	navi_t to_obj = navi_make_string(from->size, from->size, from->length);
	struct navi_string *to = navi_string(to_obj);
	memcpy(to->data, from->data, from->size);
	return to_obj;
}

DEFUN(scm_string_copy, args, env)
{
	navi_t from, to;
	long start, end;
	int nr_args = navi_list_length(args);

	from = navi_type_check(navi_car(args), NAVI_STRING, env);
	start = (nr_args > 1) ? navi_fixnum_cast(navi_cadr(args), env) : 0;
	end = (nr_args > 2) ? navi_fixnum_cast(navi_caddr(args), env)
			: navi_string(from)->size;
	navi_check_copy(navi_string(from)->length, start, end, env);

	to = navi_make_string(end - start, end - start, end - start);
	return copy_to(to, 0, from, start, end);
}

DEFUN(scm_string_copy_to, args, env)
{
	navi_t to, from;
	long at, start, end;
	int nr_args = navi_list_length(args);

	to = navi_type_check(navi_car(args), NAVI_STRING, env);
	at = navi_fixnum_cast(navi_cadr(args), env);
	from = navi_type_check(navi_caddr(args), NAVI_STRING, env);
	start = (nr_args > 3) ? navi_fixnum_cast(navi_cadddr(args), env) : 0;
	end = (nr_args > 4) ? navi_fixnum_cast(navi_caddddr(args), env)
			: navi_string(from)->size;
	navi_check_copy_to(navi_string(to)->length, at,
			navi_string(from)->length, start, end, env);

	copy_to(to, at, from, start, end);
	return navi_unspecified();
}

static navi_t string_map(navi_t proc, navi_t str, navi_env_t env)
{
	navi_t u32_in = string_to_vector(str);
	navi_t u32_out = navi_make_vector(navi_vector(u32_in)->size);
	return navi_vector_map(proc, u32_out, u32_in, env);
}

DEFUN(scm_string_map_ip, args, env)
{
	int32_t size;
	navi_t vec_obj;
	struct navi_vector *vec;
	struct navi_string *str;
	navi_type_check_proc(navi_car(args), 1, env);
	str = navi_string_cast(navi_cadr(args), env);

	vec_obj = string_map(navi_car(args), navi_cadr(args), env);
	vec = navi_vector(vec_obj);
	size = u32_size(vec);

	if (size > str->capacity)
		navi_string_grow_storage(str, size - str->capacity);

	vector_to_string_ip(str, vec);
	return navi_cadr(args);
}

DEFUN(scm_string_map, args, env)
{
	navi_type_check_proc(navi_car(args), 1, env);
	navi_type_check(navi_cadr(args), NAVI_STRING, env);
	return vector_to_string(string_map(navi_car(args), navi_cadr(args), env));
}
