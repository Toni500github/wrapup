CXX       	?= g++
PREFIX	  	?= /usr
MANPREFIX	?= $(PREFIX)/share/man
APPPREFIX 	?= $(PREFIX)/share/applications
VARS  	  	?=

DEBUG 		?= 1
ONLINE		?= 1
# https://stackoverflow.com/a/1079861
# WAY easier way to build debug and release builds
ifeq ($(DEBUG), 1)
        BUILDDIR  = build/debug
        CXXFLAGS := -ggdb3 -Wall -Wextra -Wpedantic -Wno-unused-parameter -DDEBUG=1 $(DEBUG_CXXFLAGS) $(CXXFLAGS)
else
	# Check if an optimization flag is not already set
	ifneq ($(filter -O%,$(CXXFLAGS)),)
    		$(info Keeping the existing optimization flag in CXXFLAGS)
	else
    		CXXFLAGS := -O3 $(CXXFLAGS)
	endif
        BUILDDIR  = build/release
endif

NAME		= wrapup
TARGET		= $(NAME)
OLDVERSION	= 0.0.1
VERSION    	= 0.0.1
BRANCH     	= $(shell git rev-parse --abbrev-ref HEAD)
SRC 	   	= $(wildcard src/*.cpp)
OBJ 	   	= $(SRC:.cpp=.o)
LDFLAGS   	+= -L./$(BUILDDIR)/fmt -lfmt -ldl
CXXFLAGS  	?= -mtune=generic -march=native
CXXFLAGS        += -fvisibility=hidden -Iinclude -std=c++17 $(VARS) -DVERSION=\"$(VERSION)\" -DBRANCH=\"$(BRANCH)\"

ifeq ($(ONLINE), 1)
	# pkg-config --static --libs libcurl (but fixed)
	CURL_LIBS ?= -lcurl -lnghttp3 -lnghttp2 -lidn2 -lssh2 -lssl -lcrypto -lpsl -lgssapi_krb5 -lzstd -lbrotlidec -lz
	LDFLAGS   += -L./$(BUILDDIR)/cpr -lcpr $(CURL_LIBS)
	CXXFLAGS  += -DONLINE=1
endif

is_cpr_installed = $(shell ldconfig -p | grep libcpr > /dev/null || test -d $(BUILDDIR)/cpr && test $(ONLINE) -eq 1 && echo -n yes)

all: cpr fmt toml $(TARGET)

cpr:
ifneq ($(is_cpr_installed), yes)
        #git submodule init
        #git submodule update --init --recursive
        #git -C $@ checkout 3b15fa8
        mkdir -p $(BUILDDIR)/cpr
        cmake -S $@ -B $@/build -DCMAKE_BUILD_TYPE=Release -DCPR_BUILD_TESTS=OFF -DCPR_USE_SYSTEM_CURL=OFF -DBUILD_SHARED_LIBS=OFF
        cmake --build $@/build --parallel
        mv -f $@/build/lib/*.a $(BUILDDIR)/cpr
        # the absence of this one line didn't matter for a long time, despite it being critical, this caused toni to go mentally insane when trying to make changes to the way the project is being built.
        mv -f $@/build/cpr_generated_includes/cpr/cprver.h include/cpr/
        #sudo cmake --install $@/build --prefix /usr
endif

fmt:
ifeq ($(wildcard $(BUILDDIR)/fmt/libfmt.a),)
	mkdir -p $(BUILDDIR)/fmt
	make -C src/fmt BUILDDIR=$(BUILDDIR)/fmt CXX=$(CXX)
endif

toml:
ifeq ($(wildcard $(BUILDDIR)/toml++/toml.o),)
	mkdir -p $(BUILDDIR)/toml++
	make -C src/toml++ BUILDDIR=$(BUILDDIR)/toml++ CXX=$(CXX)
endif

$(TARGET): cpr fmt toml $(OBJ)
	mkdir -p $(BUILDDIR)
	$(CXX) $(OBJ) $(BUILDDIR)/toml++/toml.o -o $(BUILDDIR)/$(TARGET) $(LDFLAGS)

dist:
	bsdtar -zcf $(NAME)-v$(VERSION).tar.gz LICENSE $(TARGET).desktop $(TARGET).1 assets/ascii/ -C $(BUILDDIR) $(TARGET)

clean:
	rm -rf $(BUILDDIR)/$(TARGET) $(OBJ)

distclean:
	rm -rf $(BUILDDIR) ./tests/$(BUILDDIR) $(OBJ)
	find . -type f -name "*.tar.gz" -exec rm -rf "{}" \;
	find . -type f -name "*.o" -exec rm -rf "{}" \;
	find . -type f -name "*.a" -exec rm -rf "{}" \;

install: $(TARGET)
	install $(BUILDDIR)/$(TARGET) -Dm 755 -v $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1/
	sed -e "s/@VERSION@/$(VERSION)/g" -e "s/@BRANCH@/$(BRANCH)/g" < $(TARGET).1 > $(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1

uninstall:
	rm -f  $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f  $(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1

remove: uninstall
delete: uninstall

updatever:
	sed -i "s#$(OLDVERSION)#$(VERSION)#g" $(wildcard .github/workflows/*.yml) compile_flags.txt

.PHONY: $(TARGET) updatever remove uninstall delete dist distclean fmt toml install all
