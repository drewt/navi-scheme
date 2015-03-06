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

bool navi_string_equal(navi_obj a, navi_obj b)
{
	struct navi_string *sa = navi_string(a), *sb = navi_string(b);
	if (sa->size != sb->size || sa->length != sb->length)
		return false;
	for (int32_t i = 0; i < sa->size; i++)
		if (sa->data[i] != sb->data[i])
			return false;
	return true;
}

static navi_obj list_to_string(navi_obj list, navi_env env)
{
	navi_obj cons;
	size_t size = 0, length = 0;

	navi_list_for_each(cons, list) {
		navi_type_check(navi_car(cons), NAVI_CHAR, env);
		size += u8_length(navi_char(navi_car(cons)));
		length++;
	}

	int32_t i = 0;
	navi_obj expr = navi_make_string(size, size, length);
	struct navi_string *str = navi_string(expr);

	navi_list_for_each(cons, list) {
		u8_append(str->data, i, str->capacity, navi_char(navi_car(cons)));
	}
	return expr;
}

static navi_obj string_to_list(navi_obj expr)
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

static navi_obj string_to_vector(navi_obj str_obj)
{
	struct navi_string *str = navi_string(str_obj);
	navi_obj vec_obj = navi_make_vector(str->length);
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

static navi_obj vector_to_string(navi_obj vec_obj)
{
	int32_t size;
	navi_obj str_obj;
	struct navi_string *str;
	struct navi_vector *vec = navi_vector(vec_obj);

	size = u32_size(vec);
	str_obj = navi_make_string(size, size, vec->size);
	str = navi_string(str_obj);

	vector_to_string_ip(str, vec);
	return str_obj;
}

DEFUN(stringp, "string?", 1, 0, NAVI_ANY)
{
	return navi_make_bool(navi_type(scm_arg1) == NAVI_STRING);
}

DEFUN(make_string, "make-string", 1, NAVI_PROC_VARIADIC, NAVI_NUM)
{
	UChar32 ch = ' ';
	int32_t length = navi_num(scm_arg1);
	if (length < 0)
		navi_error(scm_env, "invalid length");
	if (scm_nr_args > 1)
		ch = navi_char_cast(scm_arg2, scm_env);

	int32_t size = length * u8_length(ch);
	navi_obj obj = navi_make_string(size, size, length);
	struct navi_string *str = navi_string(obj);
	for (int32_t i = 0; i < size;) {
		u8_append(str->data, i, size, ch);
	}
	return obj;
}

DEFUN(string, "string", 0, NAVI_PROC_VARIADIC)
{
	return list_to_string(scm_args, scm_env);
}

DEFUN(string_length, "string-length", 1, 0, NAVI_STRING)
{
	return navi_make_num(navi_string(scm_arg1)->length);
}

DEFUN(string_ref, "string-ref", 2, 0, NAVI_STRING, NAVI_NUM)
{
	UChar32 ch;
	struct navi_string *str = navi_string(scm_arg1);
	int32_t k = navi_type_check_range(scm_arg2, 0, str->length, scm_env);

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

DEFUN(string_set, "string-set!", 3, 0, NAVI_STRING, NAVI_NUM, NAVI_CHAR)
{
	int32_t k;
	struct navi_string *str;

	str = navi_string(scm_arg1);
	k = navi_num(scm_arg2);

	if (k < 0 || k >= str->size)
		navi_error(scm_env, "string index out of bounds");

	navi_string_set(str, k, navi_char(scm_arg3));
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

#define STRING_COMPARE(cname, scmname, op, ci) \
	DEFUN(cname, scmname, 2, NAVI_PROC_VARIADIC, NAVI_STRING, NAVI_STRING) \
	{ \
		navi_obj cons; \
		struct navi_string *fst, *snd; \
		fst = navi_string(scm_arg1); \
		navi_list_for_each(cons, navi_cdr(scm_args)) { \
			snd = navi_string_cast(navi_car(cons), scm_env); \
			if (!(navi_strcoll(fst, snd, ci) op 0)) \
				return navi_make_bool(false); \
		} \
		return navi_make_bool(true); \
	}

STRING_COMPARE(string_lt,     "string<?",     <,  false);
STRING_COMPARE(string_gt,     "string>?",     >,  false);
STRING_COMPARE(string_eq,     "string=?",     ==, false);
STRING_COMPARE(string_lte,    "string<=?",    <=, false);
STRING_COMPARE(string_gte,    "string>=?",    >=, false);
STRING_COMPARE(string_ci_lt,  "string-ci<?",  <,  true);
STRING_COMPARE(string_ci_gt,  "string-ci>?",  >,  true);
STRING_COMPARE(string_ci_eq,  "string-ci=?",  ==, true);
STRING_COMPARE(string_ci_lte, "string-ci<=?", <=, true);
STRING_COMPARE(string_ci_gte, "string-ci>=?", >=, true);

static int32_t count_length(struct navi_string *str)
{
	int32_t count = 0;
	for (int32_t i = 0; i < str->size; count++) {
		UChar32 ch;
		u8_next(str->data, i, str->size, ch);
	}
	return count;
}

#define STRING_CASEMAP(cname, scmname, fun) \
	DEFUN(cname, scmname, 1, 0, NAVI_STRING) \
	{ \
		struct navi_string *src = navi_string(scm_arg1); \
		navi_obj dst_obj = navi_make_string(src->size, src->size, src->length); \
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

STRING_CASEMAP(string_upcase,   "string-upcase",   ucasemap_utf8ToUpper);
STRING_CASEMAP(string_downcase, "string-downcase", ucasemap_utf8ToLower);
STRING_CASEMAP(string_foldcase, "string-foldcase", ucasemap_utf8FoldCase);

DEFUN(string_append, "string-append", 0, NAVI_PROC_VARIADIC)
{
	navi_obj cons, obj;
	struct navi_string *str;
	int32_t i = 0, size = 0, length = 0;

	// count combined size/length
	navi_list_for_each(cons, scm_args) {
		str = navi_string_cast(navi_car(cons), scm_env);
		size += str->size;
		length += str->length;
	}

	// allocate
	obj = navi_make_string(size, size, length);
	str = navi_string(obj);

	// copy
	navi_list_for_each(cons, scm_args) {
		struct navi_string *other = navi_string(navi_car(cons));
		for (int32_t j = 0; j < other->size; j++)
			str->data[i++] = other->data[j];
	}
	return obj;
}

DEFUN(string_to_list, "string->list", 1, 0, NAVI_STRING)
{
	return string_to_list(scm_arg1);
}

DEFUN(list_to_string, "list->string", 1, 0, NAVI_LIST)
{
	return list_to_string(scm_arg1, scm_env);
}

DEFUN(string_to_vector, "string->vector", 1, 0, NAVI_STRING)
{
	return string_to_vector(scm_arg1);
}

DEFUN(vector_to_string, "vector->string", 1, 0, NAVI_VECTOR)
{
	struct navi_vector *vec = navi_vector(scm_arg1);
	for (size_t i = 0; i < vec->size; i++)
		navi_type_check(vec->data[i], NAVI_CHAR, scm_env);
	return vector_to_string(scm_arg1);
}

static navi_obj copy_to(navi_obj to, int32_t at, navi_obj from, int32_t start,
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

DEFUN(string_fill, "string-fill!", 2, NAVI_PROC_VARIADIC, NAVI_STRING, NAVI_CHAR)
{
	UChar32 ch;
	int32_t start, end, k, j, i = 0;
	int32_t new_size, old_size;
	struct navi_string *str;

	str = navi_string(scm_arg1);
	ch = navi_char(scm_arg2);
	start = (scm_nr_args > 2) ? navi_fixnum_cast(scm_arg3, scm_env) : 0;
	end = (scm_nr_args > 3) ? navi_fixnum_cast(scm_arg4, scm_env) : str->size;
	navi_check_copy(str->length, start, end, scm_env);

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

navi_obj navi_strdup(navi_obj from_obj)
{
	struct navi_string *from = navi_string(from_obj);
	navi_obj to_obj = navi_make_string(from->size, from->size, from->length);
	struct navi_string *to = navi_string(to_obj);
	memcpy(to->data, from->data, from->size);
	return to_obj;
}

DEFUN(string_copy, "string-copy", 1, NAVI_PROC_VARIADIC, NAVI_STRING)
{
	navi_obj from, to;
	long start, end;

	from = scm_arg1;
	start = (scm_nr_args > 1) ? navi_fixnum_cast(scm_arg2, scm_env) : 0;
	end = (scm_nr_args > 2) ? navi_fixnum_cast(scm_arg3, scm_env)
			: navi_string(from)->size;
	navi_check_copy(navi_string(from)->length, start, end, scm_env);

	to = navi_make_string(end - start, end - start, end - start);
	return copy_to(to, 0, from, start, end);
}

DEFUN(string_copy_to, "string-copy!", 3, NAVI_PROC_VARIADIC,
		NAVI_STRING, NAVI_NUM, NAVI_STRING)
{
	navi_obj to, from;
	long at, start, end;

	to = scm_arg1;
	at = navi_num(scm_arg2);
	from = scm_arg3;
	start = (scm_nr_args > 3) ? navi_fixnum_cast(scm_arg4, scm_env) : 0;
	end = (scm_nr_args > 4) ? navi_fixnum_cast(scm_arg5, scm_env)
			: navi_string(from)->size;
	navi_check_copy_to(navi_string(to)->length, at,
			navi_string(from)->length, start, end, scm_env);

	copy_to(to, at, from, start, end);
	return navi_unspecified();
}

DEFUN(substring, "substring", 3, 0, NAVI_STRING, NAVI_NUM, NAVI_NUM)
{
	return scm_string_copy(3, scm_args, scm_env, NULL);
}
