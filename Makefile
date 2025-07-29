.PHONY: all clean dist

DIST_DIR := dist

# Targets
all: clean dist/main_c dist/main_objc_arc dist/main_objc_noarc dist/arc_app dist/noarc_app

# Create dist dir
dist:
	mkdir -p $(DIST_DIR)

# Rules
dist/main.m: main.c | dist
	cp $< $@

dist/main_c: main.c | dist
	clang -framework Cocoa -framework Carbon -o $@ $<

dist/main_objc_arc: dist/main.m | dist
	clang -framework Cocoa -framework Carbon -fobjc-arc -o $@ $<

dist/main_objc_noarc: dist/main.m | dist
	clang -framework Cocoa -framework Carbon -fno-objc-arc -o $@ $<

dist/arc_app: dist/main_objc_arc | dist
	./appify -s $< -n $(DIST_DIR)/SimpleCApp_arc

dist/noarc_app: dist/main_objc_noarc | dist
	./appify -s $< -n $(DIST_DIR)/SimpleCApp_noarc

# Clean everything in dist
clean:
	rm -rf $(DIST_DIR)

