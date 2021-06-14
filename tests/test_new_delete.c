#include <libtcc-plug.h>
#include <assert.h>

int main(void)
{
	tcc_plug_ctx_t *plug;

	plug = tcc_plug_new();
	assert(plug);

	tcc_plug_delete(plug);
	return 0;
}
