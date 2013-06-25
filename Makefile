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

PLUGINS = gimp-dbus


# +------------------+------------------------------------------------
# | Standard Targets |
# +------------------+

default: install-local

install-local: $(PLUGINS)
	gimptool-2.0 --install-bin $(PLUGINS)

install: gimp-dbus
	gimptool-2.0 --install-admin-bin $(PLUGINS)

clean:
	rm -f $(PLUGINS)

distclean: clean


# +-----------------+-------------------------------------------------
# | Primary Targets |
# +-----------------+

gimp-dbus: gimp-dbus.c
	$(BUILD_PLUGIN) $^
