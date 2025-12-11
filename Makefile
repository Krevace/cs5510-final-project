CFLAGS = -O2 -g -std=gnu99 -Wall -fopenmp

programs = test-queue-lock test-queue-lockfree test-queue-lockfree-ck
all: $(programs)

test-queue-lock: test-queue.c
	$(CC) $(CFLAGS) -DLOCK $^ -o $@ -lpthread

test-queue-lockfree: test-queue.c
	$(CC) $(CFLAGS) -DLOCKFREE $^ -o $@

test-queue-lockfree-ck: test-queue.c
	$(CC) $(CFLAGS) -DCK -I $(HOME)/.local/include -L $(HOME)/.local/lib -lck $^ -o $@

%:%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm -f *.o
	-rm -f $(programs)
