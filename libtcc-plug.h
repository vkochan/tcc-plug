#ifndef LIBTCC_PLUG_H
#define LIBTCC_PLUG_H

struct tcc_plug;
typedef struct tcc_plug tcc_plug_t;

tcc_plug_t *tcc_plug_new(void);
void tcc_plug_delete(tcc_plug_t *);

int tcc_plug_add_path(tcc_plug_t *plug, const char *s);
int tcc_plug_load(tcc_plug_t *plug);

#endif /* LIBTCC_PLUG_H */
