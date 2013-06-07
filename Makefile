# gimp-dbus/Makefile
#   Makefile a a DBus server for GIMP.

# +----------+--------------------------------------------------------
# | Settings |
# +----------+

CFLAGS = -g -Wall -DDEBUG


# +-------+-----------------------------------------------------------
# | Files |
# +-------+

PLUGINS = gimp-dbus


# +------------------+------------------------------------------------
# | Standard Targets |
# +------------------+

default: $(PLUGINS)

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
	gimptool-2.0 --build $^
