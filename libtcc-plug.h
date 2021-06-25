#ifndef LIBTCC_PLUG_H
#define LIBTCC_PLUG_H

struct tcc_plug;
typedef struct tcc_plug tcc_plug_t;

tcc_plug_t *tcc_plug_new(void);
void tcc_plug_delete(tcc_plug_t *);

int tcc_plug_add_symb(tcc_plug_t *plug, const char *name, void *ptr);

int tcc_plug_add_libpath(tcc_plug_t *plug, char *s);
int tcc_plug_add_incpath(tcc_plug_t *plug, char *s);

int tcc_plug_set_loadpath(tcc_plug_t *plug, char *path);
char *tcc_plug_get_loadpath(tcc_plug_t *plug);

int tcc_plug_load(tcc_plug_t *plug, char *name);
int tcc_plug_unload(tcc_plug_t *plug, char *name);

#endif /* LIBTCC_PLUG_H */
