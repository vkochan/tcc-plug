#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <libtcc.h>
#include <inttypes.h>

#define BIT(x) (1UL << (x))

#define XSTR_BIT_ALLOC	BIT(0)

typedef struct xstr {
	char   *str;
	size_t	len;
	unsigned char flags;
} xstr_t;

static xstr_t xstr(char *str)
{
	xstr_t ret = {0};

	ret.len = strlen(str);
	ret.str = str;

	return ret;
}

static char *xstr_ptr(xstr_t s)
{
	return s.str;
}

static xstr_t xstr_dup(xstr_t s)
{
	xstr_t ret;

	ret.flags = XSTR_BIT_ALLOC;
	ret.str = strdup(s.str);
	ret.len = s.len;

	return ret;
}

static xstr_t xstr_cat(xstr_t s1, xstr_t s2)
{
	xstr_t ret;

	ret.len = s1.len + s2.len;
	ret.str = calloc(ret.len + 1, 1);
	ret.flags = XSTR_BIT_ALLOC;

	strcpy(ret.str, s1.str);
	strcpy(ret.str + s1.len, s2.str);

	return ret;
}

static xstr_t xstr_join(xstr_t s1, xstr_t js, xstr_t s2)
{
	xstr_t ret;

	ret.len = s1.len + js.len + s2.len;
	ret.str = calloc(ret.len + 1, 1);
	ret.flags = XSTR_BIT_ALLOC;

	strcpy(ret.str, s1.str);
	strcpy(ret.str + s1.len, js.str);
	strcpy(ret.str + s1.len + js.len, s2.str);

	return ret;
}

static void xstr_free(xstr_t xs)
{
	if (xs.flags & XSTR_BIT_ALLOC)
		free(xs.str);
}

#define container_of(ptr, type, member) ({	\
		void *__mptr = (void *)(ptr);	\
		((type *)(__mptr - offsetof(type, member))); })

typedef struct list_item {
	struct list_item *prev;
	struct list_item *next;
} list_t;

static void list_init(list_t *list)
{
	list->prev = list->next = list;
}

static void __list_add(list_t *new, list_t *prev, list_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static void list_add_first(list_t *new, list_t *head)
{
	__list_add(new, head, head->next);
}

static void list_add_tail(list_t *new, list_t *head)
{
	__list_add(new, head->prev, head);
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_entry_is_head(pos, head, member)				\
	(&pos->member == (head))

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
	     !list_entry_is_head(pos, head, member);			\
	     pos = list_next_entry(pos, member))

#define list_for_each_entry_safe(pos, n, head, member)		\
	for (pos = list_first_entry(head, typeof(*pos), member), \
	     n = list_next_entry(pos, member); \
	     &pos->member != (head); \
	     pos = n, n = list_next_entry(n, member))

typedef struct tcc_plug_path {
	list_t list;
	xstr_t str;
} tcc_plug_path_t;

typedef struct tcc_plug_symb {
	list_t list;
	xstr_t name;
	void *ptr;
} tcc_plug_symb_t;

typedef struct tcc_plug {
	list_t inc_pathes;
	list_t lib_pathes;
	list_t load_pathes;

	list_t symbs;
} tcc_plug_t;

tcc_plug_t *tcc_plug_new(void)
{
	tcc_plug_t *plug;

	plug = calloc(sizeof(tcc_plug_t), 1);
	if (!plug)
		return NULL;

	list_init(&plug->inc_pathes);
	list_init(&plug->lib_pathes);
	list_init(&plug->load_pathes);

	list_init(&plug->symbs);

	return plug;
}

void tcc_plug_delete(tcc_plug_t *plug)
{
	tcc_plug_path_t *path, *tmp_path;
	tcc_plug_symb_t *symb, *tmp_symb;

	list_for_each_entry_safe(path, tmp_path, &plug->inc_pathes, list) {
		xstr_free(path->str);
		free(path);
	}
	list_for_each_entry_safe(path, tmp_path, &plug->lib_pathes, list) {
		xstr_free(path->str);
		free(path);
	}
	list_for_each_entry_safe(path, tmp_path, &plug->load_pathes, list) {
		xstr_free(path->str);
		free(path);
	}

	list_for_each_entry_safe(symb, tmp_symb, &plug->symbs, list) {
		xstr_free(symb->name);
		free(symb);
	}

	free(plug);
}

int tcc_plug_add_loadpath(tcc_plug_t *plug, char *s)
{
	tcc_plug_path_t *path;

	path = calloc(sizeof(*path), 1);
	if (!path)
		return -ENOMEM;

	path->str = xstr(s);

	list_add_tail(&path->list, &plug->load_pathes);
	return 0;
}

int tcc_plug_add_incpath(tcc_plug_t *plug, char *s)
{
	tcc_plug_path_t *path;

	path = calloc(sizeof(*path), 1);
	if (!path)
		return -ENOMEM;

	path->str = xstr(s);

	list_add_tail(&path->list, &plug->inc_pathes);
	return 0;
}

int tcc_plug_add_libpath(tcc_plug_t *plug, char *s)
{
	tcc_plug_path_t *path;

	path = calloc(sizeof(*path), 1);
	if (!path)
		return -ENOMEM;

	path->str = xstr(s);

	list_add_tail(&path->list, &plug->lib_pathes);
	return 0;
}

int tcc_plug_add_symb(tcc_plug_t *plug, char *name, void *ptr)
{
	tcc_plug_symb_t *symb;

	symb = calloc(sizeof(*symb), 1);
	if (!symb)
		return -ENOMEM;

	symb->name = xstr(name);
	symb->ptr = ptr;

	list_add_tail(&symb->list, &plug->symbs);
	return 0;
}

int tcc_plug_load(tcc_plug_t *plug)
{
	return -1;
}
