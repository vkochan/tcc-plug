#include <string.h>
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

typedef struct tcc_plugin {
	xstr_t name;
	list_t path;
	uint8_t *code;
} tcc_plugin_t;

typedef struct tcc_plug {
	list_t pathes;
} tcc_plug_t;

tcc_plug_t *tcc_plug_new(void)
{
	return calloc(sizeof(tcc_plug_t), 1);
}

void tcc_plug_delete(tcc_plug_t *plug)
{
	free(plug);
}

int tcc_plug_add_path(tcc_plug_t *plug, const char *s)
{
	return -1;
}

int tcc_plug_load(tcc_plug_t *plug)
{
	return -1;
}
