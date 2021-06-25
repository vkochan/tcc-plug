TESTS_LDFLAGS += -L$(CURDIR)/../ -Wl,-rpath,"$(CURDIR)/../"
TESTS_CFLAGS = -I$(CURDIR)/../
TESTS_LIBS = -ltcc-plug
CC ?= tcc

TESTS := test_new_delete test_load_unload

all: run_tests
.PHONY: run_tests

test_%: test_%.o
	$(CC) $< -o $@ $(TESTS_LIBS) $(TESTS_LDFLAGS)

run_tests: $(TESTS)
	@$(foreach t,$(TESTS),\
		echo "Running $(t) ..."; \
		./$(t) && echo "$(t) [OK]" || echo "$(t) [FAILED]";)

%.o: %.c
	$(CC) $(TESTS_CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.so $(TESTS)
