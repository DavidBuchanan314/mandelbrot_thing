CONFIGS += -DPROGRESSIVE_ENHANCE
#CONFIGS += -DBENCHMARK

LIBS := gtk+-3.0 epoxy
CFLAGS := -std=gnu99 -O3 -Wall -Wpedantic $(CONFIGS)
CFLAGS += `pkg-config --cflags $(LIBS)`
LDFLAGS += `pkg-config --libs $(LIBS)` -lm

OBJS := client.o common.o rendering.o
TARGET := client
HEADER_GEN := vertex.glsl.h fragment.glsl.h

all: client

$(TARGET): $(OBJS)

$(OBJS): $(HEADER_GEN) *.h

# Bake shader sources into C headers
%.glsl.h: shaders/%.glsl
	xxd -i $< $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET) $(HEADER_GEN)
