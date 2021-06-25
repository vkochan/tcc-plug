#include <stdio.h>

int dummy2_sum(int x, int y);

int dummy_on_init(void)
{
	printf("Dummy plugin is loaded\n");
	printf("call dummy2_sum(2,7)=%d\n", dummy2_sum(2,7));
	return 0;
};

int dummy_on_remove(void)
{
	printf("Dummy plugin is removed\n");
	return 0;
};
