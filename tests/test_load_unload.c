#include <libtcc-plug.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

void test_api_hello(void)
{
	printf("call: %s\n", __func__);
};

int main(void)
{
	char load_path[512];
	tcc_plug_t *plug;
	int err;

	assert(getcwd(load_path, sizeof(load_path)));
	assert(strcat(load_path, "/plugins"));

	plug = tcc_plug_new();
	assert(plug);

	err = tcc_plug_add_symb(plug, "test_api_hello", test_api_hello);
	assert(!err);

	err = tcc_plug_set_loadpath(plug, load_path);
	assert(!err);

	err = tcc_plug_load(plug, "noexist");
	assert(err);

	err = tcc_plug_load(plug, "dummy");
	assert(!err);

	err = tcc_plug_load(plug, "dummy");
	assert(!err);

	tcc_plug_delete(plug);
	return 0;
}
