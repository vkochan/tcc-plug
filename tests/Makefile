TESTS_LDFLAGS += -L$(CURDIR)/../ -Wl,-rpath,"$(CURDIR)/../"
TESTS_CFLAGS = -I$(CURDIR)/../
TESTS_LIBS = -ltcc-plug
TESTS_CC = tcc

TESTS := test_new_delete

all: run_tests
.PHONY: run_tests

test_%: test_%.o
	$(TESTS_CC) $< -o $@ $(TESTS_LIBS) $(TESTS_LDFLAGS)

run_tests: $(TESTS)
	@$(foreach t,$(TESTS),\
		echo "Running $(t) ..."; \
		./$(t) && echo "$(t) [OK]" || echo "$(t) [FAILED]";)

%.o: %.c
	$(TESTS_CC) $(TESTS_CFLAGS) -c $< -o $@
