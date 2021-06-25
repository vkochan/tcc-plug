#include <stdio.h>

int dummy2_sum(int x, int y)
{
	return x + y;
}

int dummy2_on_init(void)
{
	printf("Dummy2 plugin is loaded\n");
	return 0;
};

int dummy2_on_remove(void)
{
	printf("Dummy2 plugin is removed\n");
	return 0;
};
