all:

define install_so
$$(DESTDIR)/usr/lib/$1.$$(VER_MAJOR).$$(VER_MINOR): $1
	install -D -m 0644 '$$<' '$$(DESTDIR)/usr/lib/$1.$$(VER_MAJOR).$$(VER_MINOR)'
	ln -fs '$1.$$(VER_MAJOR).$$(VER_MINOR)' '$$(DESTDIR)/usr/lib/$1.$$(VER_MAJOR)'
	ln -fs '$1.$$(VER_MAJOR).$$(VER_MINOR)' '$$(DESTDIR)/usr/lib/$1'

install: $$(DESTDIR)/usr/lib/$1.$$(VER_MAJOR).$$(VER_MINOR)
endef

-include config.mk
include  project.mk
-include $(shell find -name '*.d')

ifndef NOLTO
CFLAGS += -flto
endif

Makefile: project.mk $(shell ls config.mk 2>/dev/null)

CFLAGS += -MMD

CLEAN += build dpkg $(shell find -name '*.o' -or -name '*.so' -or -name '*.d' -or -name '*.tar.xz')

.PHONY: clean install all dpkg
clean:
	rm -rv $(CLEAN) 2>/dev/null || true

%: %.o Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter %.o, $^) $(LIBS) -o '$@'

%: %.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter %.c, $^) $(LIBS) -o '$@'

%.o: %.c Makefile
	$(CC) -c $(CFLAGS) '$<' -o '$@'

%.o: %.cpp Makefile
	$(CXX) -c $(CFLAGS) '$<' -o '$@'

%.so: CFLAGS += -fPIC
%.so: %.o Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(filter %.o, $^) $(LIBS) -shared -Wl,-soname,$@.$(VER_MAJOR) -o $@

%.so.$(VER_MAJOR): %.so
	ln -sf '$<' '$@'

$(DESTDIR)/usr/include/%: %
	install -D -m 0644 '$<' '$@'

$(PROJECT_NAME)_$(VER_MAJOR).$(VER_MINOR).orig.tar.xz $(PROJECT_NAME)-$(VER_MAJOR).$(VER_MINOR).tar.xz:
	git archive --format tar --prefix '$(PROJECT_NAME)-$(VER_MAJOR).$(VER_MINOR)/' HEAD | xz -9 > '$@'

dpkg: $(PROJECT_NAME)_$(VER_MAJOR).$(VER_MINOR).orig.tar.xz
	mkdir -p build
	cd build && \
	tar xf '../$<' && \
	cd $(PROJECT_NAME)-$(VER_MAJOR).$(VER_MINOR) && \
	debuild -us -uc

# vim: tabstop=4 : noexpandtab :
