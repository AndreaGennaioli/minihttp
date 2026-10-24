CC=gcc
CLFAGS=-Wall -Wextra -Werror -g

SRC_DIR=src
OBJS_DIR=obj
DIST_DIR=.

OBJS=$(OBJS_DIR)/main.o

mhttp: $(OBJS)
	$(CC) $(CFLAGS) -o $(DIST_DIR)/$@ $^


$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

clean:
	rm -f $(DIST_DIR)/mt $(OBJS)

.PHONY: clean

