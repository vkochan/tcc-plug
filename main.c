#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include <libtcc.h>

#define max(a,b)             \
	({                           \
	 __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a > _b ? _a : _b;       \
	 })

#define min(a,b)             \
	({                           \
	 __typeof__ (a) _a = (a); \
	 __typeof__ (b) _b = (b); \
	 _a < _b ? _a : _b;       \
	 })


#define BIT(x) (1UL << (x))

#define XMEM_BIT_ALLOC	BIT(0)

typedef struct xstr {
	char   *str;
	size_t	len;
	unsigned char flags;
} xstr_t;

static xstr_t xstr(const char *s)
{
	xstr_t ret = {0};

	if (s) {
		ret.len = strlen(s);
		ret.str = s;
	}

	return ret;
}

static char *xstr_cptr(xstr_t s)
{
	return s.str;
}

static xstr_t xstr_dup(xstr_t s)
{
	xstr_t ret;

	ret.str = strndup(s.str, s.len);
	ret.flags = XMEM_BIT_ALLOC;
	ret.len = s.len;

	return ret;
}

static xstr_t xstr_cat(xstr_t s1, xstr_t s2)
{
	xstr_t ret;

	if (!s1.len)
		return s2;
	if (!s2.len)
		return s1;

	ret.len = s1.len + s2.len;
	ret.str = calloc(ret.len + 1, 1);
	ret.flags = XMEM_BIT_ALLOC;

	strncpy(ret.str, s1.str, s1.len);
	strncpy(ret.str + s1.len, s2.str, s2.len);

	return ret;
}

static xstr_t xstr_join(xstr_t s1, xstr_t js, xstr_t s2)
{
	xstr_t ret;

	ret.len = s1.len + js.len + s2.len;
	ret.str = calloc(ret.len + 1, 1);
	ret.flags = XMEM_BIT_ALLOC;

	strncpy(ret.str, s1.str, s1.len);
	strncpy(ret.str + s1.len, js.str, js.len);
	strncpy(ret.str + s1.len + js.len, s2.str, s2.len);

	return ret;
}

static size_t xstr_len(xstr_t s)
{
	return s.len;
}

static bool xstr_is_empty(xstr_t s)
{
	return s.len == 0;
}

static bool xstr_has_prefix(xstr_t s, xstr_t p)
{
	char *s_cptr = s.str;
	char *p_cptr = p.str;
	int i;

	if (p.len > s.len)
		return false;

	for (i = 0; i < p.len; i++) {
		if (*s_cptr != *p_cptr)
			return false;
	}

	return i == p.len;
}

static bool xstr_eq(xstr_t s1, xstr_t s2)
{
	if (s1.len != s2.len)
		return false;

	return strncmp(s1.str, s2.str, min(s1.len, s2.len)) == 0;
}

static xstr_t xstr_tok(xstr_t *s, char delim)
{
	char *start, *end;
	xstr_t ret = {0};

	if (s->len == 0)
		return ret;

	for (start = s->str; start != s->str + s->len && *start == delim; start++)
		;
	for (end = start; end != s->str + s->len && *end != delim; end++)
		;

	ret.len = end - start;
	ret.str = start;

	s->len -= end - s->str;
	s->str = end;

	return ret;
}

static void xstr_del(xstr_t s)
{
	if (s.flags & XMEM_BIT_ALLOC)
		free(s.str);
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

static bool list_empty(list_t *list)
{
	return list->next == list;
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

static void __list_del(list_t *prev, list_t *next)
{
	next->prev = prev;
	prev->next = next;
}

static void list_del(list_t *entry)
{
	__list_del(entry->prev, entry->next);
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

typedef struct xpath {
	list_t list;
	xstr_t path;
} xpath_t;

static xpath_t xpath(xstr_t s)
{
	xpath_t p = {0};

	p.path = s;

	return p;
}

static xpath_t *xpath_new(xstr_t s)
{
	xpath_t *p;

	p = calloc(sizeof(*p), 1);
	if (!p)
		return NULL;

	p->path = s;

	return p;
}

static void xpath_del(xpath_t *p)
{
	if (p) {
		xstr_del(p->path);
		free(p);
	}
}

static xstr_t xpath_str(xpath_t *path)
{
	return path->path;
}

void xpath_set(xpath_t *path, xstr_t new)
{
	path->path = new;
}

void xpath_append(xpath_t *path, xstr_t add)
{
	path->path = xstr_join(path->path, xstr("/"), add);
}

xstr_t xpath_basename(xpath_t *path)
{
	char *end_cptr, *base_cptr;
	xstr_t *s;

	if (!path)
		return xstr("");

	s = &path->path;
	end_cptr = s->str + s->len - 1;

	for (base_cptr = end_cptr; base_cptr != s->str; base_cptr--) {
		if (*base_cptr == '/') {
			xstr_t ret = {0};

			ret.len = end_cptr - base_cptr;
			ret.str = ++base_cptr;

			return ret;
		}
	}

	return xstr("");
}

typedef struct tcc_plug_symb {
	list_t list;
	xstr_t name;
	const void *ptr;
} tcc_plug_symb_t;

typedef struct tcc_plug {
	list_t inc_pathes;
	list_t lib_pathes;

	xstr_t load_path;

	list_t plugins;
	list_t symbs;
} tcc_plug_t;

typedef struct tcc_plugin {
	list_t list;
	tcc_plug_t *plug;
	list_t exp_symbs;
	xstr_t abspath;
	xstr_t name;
	list_t srcs;
	list_t deps;

	int (*on_init)(void);
	int (*on_remove)(void);
	char *code;

	bool is_loaded;
} tcc_plugin_t;

typedef struct tcc_plug_deps {
	list_t list;
	tcc_plugin_t *plug;
	xstr_t name;
} tcc_plug_deps_t;

static tcc_plugin_t *tcc_plugin_get(tcc_plug_t *plug, xstr_t name)
{
	struct dirent *de;
	tcc_plugin_t *p;
	xstr_t abspath;
	struct stat st;
	DIR *dir;

	list_for_each_entry(p, &plug->plugins, list) {
		if (xstr_eq(p->name, name))
			return p;
	}
	p = NULL;

	abspath = xstr_join(plug->load_path, xstr("/"), name);

	if (stat(xstr_cptr(abspath), &st) == -1)
		goto out;
	if ((st.st_mode & S_IFMT) != S_IFDIR)
		goto out;

	dir = opendir(xstr_cptr(abspath));
	if (!dir)
		goto out;

	p = calloc(sizeof(*p), 1);
	if (!p)
		goto out;

	list_init(&p->exp_symbs);
	list_init(&p->srcs);
	list_init(&p->deps);
	p->abspath = abspath;
	p->plug = plug;

	while (de = readdir(dir)) {
		xpath_t *path;

		if (strcmp(de->d_name, "config.c") == 0)
			continue;
		if (strcmp(".", de->d_name) == 0)
			continue;
		if (strcmp("..", de->d_name) == 0)
			continue;
		if (!de->d_type == DT_REG)
			continue;
		if (!strstr(de->d_name, ".c"))
			continue;

		path = xpath_new(abspath);
		if (!path)
			goto out;

		xpath_append(path, xstr(de->d_name));

		list_add_tail(&path->list, &p->srcs);
	}

	p->name = xstr_dup(name);

	if (list_empty(&p->srcs)) {
		free(p);
		p = NULL;
	}

	closedir(dir);
out:
	return p;
}

static void tcc_plugin_put(tcc_plugin_t *p)
{
	tcc_plug_symb_t *symb, *tmp_symb;
	tcc_plug_deps_t *dep, *tmp_dep;
	xpath_t *path, *tmp_path;

	list_for_each_entry_safe(symb, tmp_symb, &p->exp_symbs, list) {
		list_del(&symb->list);
		xstr_del(symb->name);
		free(symb);
	}
	list_for_each_entry_safe(dep, tmp_dep, &p->deps, list) {
		list_del(&dep->list);
		xstr_del(dep->name);
		free(dep);
	}
	list_for_each_entry_safe(path, tmp_path, &p->srcs, list) {
		xpath_del(path);
	}
	xstr_del(p->abspath);
	xstr_del(p->name);
	free(p->code);
	free(p);
}


static void tcc_plugin_get_symbs_cb(void *ctx, const char *name, const void *val)
{
	tcc_plug_symb_t *symb;
	tcc_plugin_t *p = ctx;
	xstr_t prefix;

	prefix = xstr_cat(p->name, xstr("_"));

	if (!xstr_has_prefix(xstr(name), prefix))
		goto out;

	symb = calloc(sizeof(*symb), 1);
	if (!symb)
		goto out;

	symb->name = xstr_dup(xstr(name));
	symb->ptr = val;

	list_add_tail(&symb->list, &p->exp_symbs);

out:
	xstr_del(prefix);
}

static tcc_plugin_t *tcc_plugin_load(tcc_plug_t *plug, xstr_t name);

static int tcc_plugin_compile(tcc_plugin_t *p)
{
	tcc_plug_symb_t *symb;
	tcc_plug_deps_t *dep;
	xstr_t symb_name;
	xpath_t *src;
	TCCState *s;
	int err = 0;
	int len;

	s = tcc_new();
	if (!s)
		return -ENOMEM;

	err = tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
	if (err)
		goto out;

	list_for_each_entry(src, &p->srcs, list) {
		err = tcc_add_file(s, xstr_cptr(xpath_str(src)));
		if (err)
			goto out;
	}

	/* export symbols */
	list_for_each_entry(dep, &p->deps, list) {
		tcc_plugin_t *d = tcc_plugin_load(p->plug, dep->name);

		if (!d) {
			tcc_plugin_put(p);
			return -EINVAL;
		}

		list_for_each_entry(symb, &d->exp_symbs, list) {
			err = tcc_add_symbol(s, xstr_cptr(symb->name), symb->ptr);
			if (err) {
				tcc_plugin_put(p);
				return -EINVAL;
			}
		}
	}
	list_for_each_entry(symb, &p->plug->symbs, list) {
		err = tcc_add_symbol(s, xstr_cptr(symb->name), symb->ptr);
		if (err) {
			tcc_plugin_put(p);
			return -EINVAL;
		}
	}

	len = tcc_relocate(s, NULL);
	if (!len) {
		err = -EINVAL;
		goto out;
	}

	p->code = calloc(1, len);
	if (!p->code) {
		err = -ENOMEM;
		goto out;
	}

	err = tcc_relocate(s, p->code);
	if (err)
		goto out;

	symb_name = xstr_cat(p->name, xstr("_on_init"));
	p->on_init = tcc_get_symbol(s, xstr_cptr(symb_name));
	xstr_del(symb_name);

	symb_name = xstr_cat(p->name, xstr("_on_remove"));
	p->on_remove = tcc_get_symbol(s, xstr_cptr(symb_name));
	xstr_del(symb_name);

	tcc_list_symbols(s, p, tcc_plugin_get_symbs_cb);
out:
	tcc_delete(s);
	return err;
}

static int tcc_plugin_init(tcc_plugin_t *p)
{
	if (p->on_init)
		return p->on_init();
	return 0;
}

static int tcc_plugin_remove(tcc_plugin_t *p)
{
	if (p->on_remove)
		return p->on_remove();

	list_del(&p->list);
	tcc_plugin_put(p);

	return 0;
}

static int tcc_plugin_config_get(tcc_plugin_t *p)
{
	xstr_t tok = xstr(""), iter = xstr("");
	TCCState *s = NULL;
	xstr_t symb_name;
	xstr_t cfg_path;
	struct stat st;
	int err = 0;
	char *deps;
	int len;

	cfg_path = xstr_cat(p->abspath, xstr("/config.c"));

	if (stat(xstr_cptr(cfg_path), &st) == -1)
		goto out;

	s = tcc_new();
	if (!s) {
		err = -ENOMEM;
		goto out;
	}

	err = tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
	if (err)
		goto out;

	err = tcc_add_file(s, xstr_cptr(cfg_path));
	if (err)
		goto out;

	err = tcc_relocate(s, TCC_RELOCATE_AUTO);
	if (err)
		goto out;

	symb_name = xstr_cat(p->name, xstr("_deps_require"));
	deps = tcc_get_symbol(s, xstr_cptr(symb_name));
	xstr_del(symb_name);

	iter = xstr(deps);
	tok = xstr_tok(&iter, ' ');

	while (!xstr_is_empty(tok)) {
		tcc_plug_deps_t *dep;

		dep = calloc(sizeof(*dep), 1);
		if (!dep) {
			err = -ENOMEM;
			goto out;
		}

		dep->name = xstr_dup(tok);
		if (xstr_is_empty(dep->name)) {
			err = -ENOMEM;
			goto out;
		}

		list_add_tail(&dep->list, &p->deps);

		tok = xstr_tok(&iter, ' ');
	}
	xstr_del(iter);
	xstr_del(tok);

out:
	if (s)
		tcc_delete(s);

	xstr_del(cfg_path);

	return err;
}

static tcc_plugin_t *tcc_plugin_load(tcc_plug_t *plug, xstr_t name)
{
	tcc_plugin_t *p;
	int err;

	p = tcc_plugin_get(plug, name);
	if (!p)
		return p;

	if (p->is_loaded)
		return p;

	err = tcc_plugin_config_get(p);
	if (err) {
		tcc_plugin_put(p);
		return NULL;
	}

	err = tcc_plugin_compile(p);
	if (err) {
		tcc_plugin_put(p);
		return NULL;
	}

	err = tcc_plugin_init(p);
	if (err) {
		tcc_plugin_put(p);
		return NULL;
	}

	p->is_loaded = true;

	list_add_tail(&p->list, &plug->plugins);

	return p;
}

tcc_plug_t *tcc_plug_new(void)
{
	tcc_plug_t *plug;
	char path[512];
	xstr_t cwd;

	path[sizeof(path)-1] = '\0';

	cwd = xstr(getcwd(path, sizeof(path)));
	if (xstr_is_empty(cwd))
		return NULL;

	plug = calloc(sizeof(tcc_plug_t), 1);
	if (!plug)
		return NULL;

	plug->load_path = xstr_dup(cwd);

	list_init(&plug->inc_pathes);
	list_init(&plug->lib_pathes);
	list_init(&plug->plugins);
	list_init(&plug->symbs);

	return plug;
}

void tcc_plug_delete(tcc_plug_t *plug)
{
	tcc_plug_symb_t *symb, *tmp_symb;
	xpath_t *path, *tmp_path;
	tcc_plugin_t *tmp_p, *p;

	list_for_each_entry_safe(path, tmp_path, &plug->inc_pathes, list) {
		xpath_del(path);
	}
	list_for_each_entry_safe(path, tmp_path, &plug->lib_pathes, list) {
		xpath_del(path);
	}
	list_for_each_entry_safe(symb, tmp_symb, &plug->symbs, list) {
		xstr_del(symb->name);
		free(symb);
	}
	list_for_each_entry_safe(p, tmp_p, &plug->plugins, list) {
		tcc_plugin_remove(p);
	}
	xstr_del(plug->load_path);
	free(plug);
}

int tcc_plug_set_loadpath(tcc_plug_t *plug, char *path)
{
	plug->load_path = xstr(path);
	return 0;
}

char *tcc_plug_get_loadpath(tcc_plug_t *plug)
{
	return xstr_cptr(plug->load_path);
}

int tcc_plug_add_incpath(tcc_plug_t *plug, char *s)
{
	xpath_t *path;

	path = calloc(sizeof(*path), 1);
	if (!path)
		return -ENOMEM;

	path->path = xstr(s);

	list_add_tail(&path->list, &plug->inc_pathes);
	return 0;
}

int tcc_plug_add_libpath(tcc_plug_t *plug, char *s)
{
	xpath_t *path;

	path = calloc(sizeof(*path), 1);
	if (!path)
		return -ENOMEM;

	path->path = xstr(s);

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

int tcc_plug_load(tcc_plug_t *plug, char *name)
{
	tcc_plugin_t *p = tcc_plugin_load(plug, xstr(name));

	if (!p)
		return -ENOENT;
	return 0;
}

int tcc_plug_unload(tcc_plug_t *plug, char *name)
{
	tcc_plugin_t *tmp, *p, *found = NULL;

	list_for_each_entry_safe(p, tmp, &plug->plugins, list) {
		if (xstr_eq(p->name, xstr(name)))
			return tcc_plugin_remove(p);
	}

	return -ENOENT;
}
