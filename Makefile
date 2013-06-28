# gimp-dbus/Makefile
#   Makefile a a DBus server for GIMP.

# +----------+--------------------------------------------------------
# | Settings |
# +----------+

CFLAGS = -g -Wall -DDEBUG

# +----------+--------------------------------------------------------
# | Commands |
# +----------+

# Instructions for building a plugin.  Note that gimptool-2.0 only
# takes CFLAGS and LDFLAGS from the environment, so we need to put
# them into the environment.
BUILD_PLUGIN = \
        export CFLAGS="$(CFLAGS)"; \
        export LDFLAGS="$(LDFLAGS)"; \
        gimptool-2.0 --build


# +-------+-----------------------------------------------------------
# | Files |
# +-------+

PLUGINS = gimp-dbus \
        ggimp-irgb-new \
        ggimp-irgb-components \
	ggimp-rgb-list \
        test-bytes-get \
        test-bytes-put

INSTALL = $(addsuffix .install,$(PLUGINS))
LOCAL = $(addsuffix .local,$(PLUGINS))

LIBRARIES=libtilestream.a


# +------------------+------------------------------------------------
# | Standard Targets |
# +------------------+

default: install-local

install-local: $(PLUGINS) $(LOCAL)

install: $(INSTALL)

clean:
	rm -f $(PLUGINS) $(LIBRARIES) $(LOCAL) $(INSTALL)

distclean: clean


# +------------------+------------------------------------------------
# | Building Plugins |
# +------------------+

%: %.c
	$(BUILD_PLUGIN) $^

# +--------------------+----------------------------------------------
# | Installing Plugins |
# +--------------------+

%.install: %
	gimptool-2.0 --install-admin-bin $<

%.local: %
	gimptool-2.0 --install-bin $<

%.uninstall: %
	gimptool-2.0 --uninstall-admin-bin $<
	gimptool-2.0 --uninstall-bin $<
