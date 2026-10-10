CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c11
CPPFLAGS += -Iinclude
LDLIBS += -lmosquitto

SOURCES := src/main.c src/decision.c src/sensor_parser.c src/cmd_builder.c src/mqtt_bus.c

all: rpib-decision

rpib-decision: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SOURCES) $(LDLIBS)

clean:
	rm -f rpib-decision

.PHONY: all clean
