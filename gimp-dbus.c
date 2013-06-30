/*
 * gimp-dbus 
 *   A GIMP plugin that serves PDB calls (and other related calls) over
 *   DBus.
 *
 * Copyright (c) 2012-13 Alexandra Greenberg, Mark Lewis, Evan Manuella, 
 *   Samuel Rebelsky, Hart Russell, Mani Tiwaree, and Christine Tran
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


// +----------+--------------------------------------------------------
// | Manifest |
// +----------+


// +---------+---------------------------------------------------------
// | Headers |
// +---------+

#include <libgimp/gimp.h>    
#include <libgimp/gimpui.h>    
#include <gio/gio.h> 
#include <gtk/gtk.h>       
#include <stdlib.h>        
#include <stdio.h>      
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "tile-stream.h"


// +-----------+-------------------------------------------------------
// | Constants |
// +-----------+

/**
 * The "about" message.
 */
#define GIMP_DBUS_ABOUT "Glimmer Labs' Gimp D-Bus plugin version 0.0.8"

/**
 * The service name that we use for gimp-dbus.
 */
#define GIMP_DBUS_SERVICE "edu.grinnell.cs.glimmer.GimpDBus"

/**
 * The standard object that we use for gimp-dbus.
 */
#define GIMP_DBUS_APPLICATION_OBJECT "/edu/grinnell/cs/glimmer/gimp"

/**
 * The PDB interface.  
 */
#define GIMP_DBUS_INTERFACE_PDB "edu.grinnell.cs.glimmer.pdb"

/**
 * The additional interface.
 */
#define GIMP_DBUS_INTERFACE_ADDITIONAL "edu.grinnell.cs.glimmer.gimpplus"

/**
 * Where we put this service in the menu.
 */
#define GIMP_DBUS_MENU "<Toolbox>/MediaScript/"


// +--------+----------------------------------------------------------
// | Macros |
// +--------+

/**
 * (Optionally) Print a log message.
 *
 * Plugins can be hard to debug, so we sometimes print log messages to
 * see what's happening.  LOG works like printf when the DEBUG flag is 
 * set and is a noop when the flag is not set.
 *
 * To turn on log messages, compile with the DEBUG flag set.
 */

#ifdef DEBUG
#define LOG(...) do { fprintf (stderr, __VA_ARGS__); fprintf (stderr, "\n"); } while (0)
#define BEGIN(FUN) fprintf (stderr, "BEGIN[%s]\n", FUN);
#define END(FUN) fprintf (stderr, "END[%s]\n", FUN);
#else
#define LOG(...) do { } while (0)
#define BEGIN(FUN) do { } while (0)
#define END(FUN) do { } while (0)
#endif

/**
 * Signal an error having to do with an argument.  
 */
#define SIGNAL_ARGUMENT_ERROR(INVOCATION, MESSAGE, ...) \
  g_dbus_method_invocation_return_error ( \
        INVOCATION, \
        G_IO_ERROR, \
        G_IO_ERROR_INVALID_ARGUMENT, \
        MESSAGE, \
        ##__VA_ARGS__)

/**
 * Signal a more general error.  (Unfortunately, I don't know what other
 * parameters to use.
 */
#define SIGNAL_ERROR(INVOCATION, MESSAGE, ...) \
  g_dbus_method_invocation_return_error ( \
        INVOCATION, \
        G_IO_ERROR, \
        G_IO_ERROR_INVALID_ARGUMENT, \
        MESSAGE, \
        ##__VA_ARGS__)


// +-------+-----------------------------------------------------------
// | Types |
// +-------+

/**
 * A simple way to store both the procedure names and a count of the
 * procedure names.
 */
struct gimpnames {
  gchar **procnames;
  gint nprocs;
};

/**
 * A simple dbus message handler.
 */
typedef void (*SimpleMessageHandler)(const gchar *method_name,
                                     GDBusMethodInvocation *invocation,
                                     GVariant *parameters);

/**
 * An entry in a table of message handlers.  We terminate the table
 * with an entry whose name is NULL.
 */
struct HandlerEntry
  {
    char *name;
    SimpleMessageHandler handler;
  };
typedef struct HandlerEntry HandlerEntry;

/**
 * An entry in a table of GIMP run handlers.  We terminate the table
 * with an entry whose name is NULL.
 */
struct RunTableEntry
  {
    char *name;
    GimpRunProc proc;
  };
typedef struct RunTableEntry RunTableEntry;


// +-----------------+------------------------------------------------
// | Predeclarations |
// +-----------------+

/** 
 * Handle a call to method_name on connection, using the 
 * specified parameters.  Send the result of the call to
 * invocation.
 */
int gimp_dbus_handle_pdb_method_call (GDBusConnection       *connection,
                                      const gchar           *method_name,
                                      GVariant              *parameters,
                                      GDBusMethodInvocation *invocation);

/**
 * GIMP plugin query.
 */
static void query (void);

/**
 * GIMP plugin run.
 */
static void run (const gchar      *name,
                 gint              nparams,
                 const GimpParam  *param,
                 gint             *nreturn_vals,
                 GimpParam       **return_vals);

/**
 * DBus method call handler for PDB methods.
 */
static void pdb_handle_method_call (GDBusConnection       *connection,
                                    const gchar           *sender,
                                    const gchar           *object_path,
                                    const gchar           *interface_name,
                                    const gchar           *method_name,
                                    GVariant              *parameters,
                                    GDBusMethodInvocation *invocation,
                                    gpointer               user_data);

/**
 * DBus get-propery handler for PDB properties.
 */
static GVariant *pdb_handle_get_property (GDBusConnection  *connection,
                                          const gchar      *sender,
                                          const gchar      *object_path,
                                          const gchar      *interface_name,
                                          const gchar      *property_name,
                                          GError          **error,
                                          gpointer          user_data);

/**
 * DBus set-property handler for PDB properties.
 */
static gboolean pdb_handle_set_property (GDBusConnection  *connection,
                                         const gchar      *sender,
                                         const gchar      *object_path,
                                         const gchar      *interface_name,
                                         const gchar      *property_name,
                                         GVariant         *value,
                                         GError          **error,
                                         gpointer          user_data);

/**
 * DBus method call handler for alternate methods.
 */
static void alt_handle_method_call (GDBusConnection       *connection,
                                    const gchar           *sender,
                                    const gchar           *object_path,
                                    const gchar           *interface_name,
                                    const gchar           *method_name,
                                    GVariant              *parameters,
                                    GDBusMethodInvocation *invocation,
                                    gpointer               user_data);

/**
 * DBus get-propery handler for alternate properties.
 */
static GVariant *alt_handle_get_property (GDBusConnection  *connection,
                                          const gchar      *sender,
                                          const gchar      *object_path,
                                          const gchar      *interface_name,
                                          const gchar      *property_name,
                                          GError          **error,
                                          gpointer          user_data);

/**
 * DBus set-property handler for alternate properties.
 */
static gboolean alt_handle_set_property (GDBusConnection  *connection,
                                         const gchar      *sender,
                                         const gchar      *object_path,
                                         const gchar      *interface_name,
                                         const gchar      *property_name,
                                         GVariant         *value,
                                         GError          **error,
                                         gpointer          user_data);

/**
 * Build information on one argument to a method.  Returns NULL if
 * it is unable to build.
 */
static GDBusArgInfo *
g_dbus_arg_new (gchar                *name,
                const gchar          *signature,
                GDBusAnnotationInfo **annotations);


// +---------+--------------------------------------------------------
// | Globals |
// +---------+

/**
 * The XML to describe the additional services that we provide.
 */
static const gchar alt_introspection_xml[] = 
  "<node>"
  "  <interface name='" GIMP_DBUS_INTERFACE_ADDITIONAL "'>"
  "    <method name='ggimp_about'>"
  "      <arg type='s' name='result' direction='out'/>"
  "    </method>"
  "    <method name='ggimp_quit'>"
  "    </method>"
  "    <method name='ggimp_rgb_red'>"
  "      <arg type='i' name='color' direction='in'/>"
  "      <arg type='i' name='red' direction='out'/>"
  "    </method>"
  "    <method name='tile_stream_advance'>"
  "      <arg type='i' name='stream' direction='in'/>"
  "      <arg type='i' name='continues' direction='out'/>"
  "    </method>"
  "    <method name='tile_stream_close'>"
  "      <arg type='i' name='stream' direction='in'/>"
  "    </method>"
  "    <method name='tile_stream_get'>"
  "      <arg type='i' name='stream' direction='in'/>"
  "      <arg type='i' name='size' direction='out'/>"
  "      <arg type='ay' name='data' direction='out'/>"
  "      <arg type='i' name='bpp' direction='out'/>"
  "      <arg type='i' name='rowstride' direction='out'/>"
  "      <arg type='i' name='x' direction='out'/>"
  "      <arg type='i' name='y' direction='out'/>"
  "      <arg type='i' name='width' direction='out'/>"
  "      <arg type='i' name='height' direction='out'/>"
  "    </method>"
  "    <method name='tile_stream_is_valid'>"
  "      <arg type='i' name='stream' direction='in'/>"
  "      <arg type='i' name='valid' direction='out'/>"
  "    </method>"
  "    <method name='tile_stream_new'>"
  "      <arg type='i' name='image' direction='in'/>"
  "      <arg type='i' name='drawable' direction='in'/>"
  "      <arg type='i' name='stream' direction='out'/>"
  "    </method>"
  "    <method name='tile_update'>"
  "      <arg type='i' name='stream' direction='in'/>"
  "      <arg type='i' name='size' direction='in'/>"
  "      <arg type='ay' name='data' direction='in'/>"
  "      <arg type='i' name='success' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static const GDBusNodeInfo *alt_introspection_data = NULL;

/**
 * The GDBusNodeInfo on the PDB to be published to the dbus.
 */
static GDBusNodeInfo *pdbnode = NULL;

/**
 * Information on the registration id for the PDB interface.
 */
static guint pdb_registration_id;

/**
 * Information on the registration id for the alternate interface.
 */
static guint alt_registration_id;

/**
 * The standard DBus handlers for PDB.
 */
static const GDBusInterfaceVTable pdb_interface_vtable =
  {
    pdb_handle_method_call,
    pdb_handle_get_property,
    pdb_handle_set_property
  };

/**
 * The standard DBus handlers for alternate functions.
 */
static const GDBusInterfaceVTable alt_interface_vtable =
  {
    alt_handle_method_call,
    alt_handle_get_property,
    alt_handle_set_property
  };

/**
 * The event loop.
 */
GMainLoop *loop = NULL;


// +----------------------------+--------------------------------------
// | Support for Error Checking |
// +----------------------------+

/* 
Sample code for error checking.  (This code may not be necessary if
DBus does the checking for us.

  int nparams = g_variant_n_children (parameters);
  // Check that we have one parameter and that it's an integer.
  if (nparams != 1)
    {
      report_invalid_paramcount (invocation, "FUN", 1, nparams);
      return;
    } 

  GVariant *child = g_variant_get_child_value (parameters, 0);
  if (g_variant_get_type (child) != != G_VARIANT_TYPE_INT32)
    {
      report_invalid_parameter (invocation, "FUN", 0, "integer", 
                                          child);
    }
 */

/**
 * Return an error about a particular parameter.
 */
static void
report_invalid_parameter (GDBusMethodInvocation *invocation,
                                    const gchar *method_name,
                                    int paramnum,
                                    const gchar *paramtype,
                                    GVariant *param)
{
  SIGNAL_ARGUMENT_ERROR (
    invocation,
    "%s expects %s for parameter %d, received %s",
     method_name, paramtype, paramnum, g_variant_get_type_string);
} // report_invalid_parameter

/**
 * Return an error about the number of parameters.
 */
static void
report_invalid_paramcount (GDBusMethodInvocation *invocation,
                                     const gchar *method_name,
                                     int expected,
                                     int actual)
{
  if (expected == 1)
    {
      SIGNAL_ARGUMENT_ERROR (invocation,
                             "%s expects 1 parameter, received %d",
                             method_name, actual);
    } // if we expected one parameter
  else
    {
      SIGNAL_ARGUMENT_ERROR (invocation,
                             "%s expects %d parameters, received %d",
                             method_name, expected, actual);
    } // if we expected multiple parameters
} // report_invalid_paramcount

/**
 * Make sure that a tile stream sent to a handler is valid.  If not,
 * return an error (which should stop the handler).
 */
static int
handler_validate_tile_stream (int stream, GDBusMethodInvocation *invocation)
{
  if (! tile_stream_is_valid (stream))
    {
      LOG ("tile-stream-get: Invalid tile stream: %d", stream);
      SIGNAL_ERROR (invocation, "could not create stream");
      return 0;
    } // if tile stream is invalid
  return 1;
} // handler_validate_tile_stream


// +---------------------------------+---------------------------------
// | Methods for Alternate Interface |
// +---------------------------------+

void
ggimp_dbus_handle_about (const gchar *method_name,
                         GDBusMethodInvocation *invocation,
                         GVariant *parameters)
{
  GVariant *result = g_variant_new ("(s)", GIMP_DBUS_ABOUT);
  g_dbus_method_invocation_return_value (invocation, result);
} // ggimp_dbus_handle_about

void
ggimp_dbus_handle_default (const gchar *method_name,
                           GDBusMethodInvocation *invocation,
                           GVariant *parameters)
{
  SIGNAL_ARGUMENT_ERROR (invocation, "Invalid method: '%s'", method_name);
} // ggimp_dbus_handle_default

void
ggimp_dbus_handle_quit (const gchar *method_name,
                        GDBusMethodInvocation *invocation,
                        GVariant *parameters)
{
  g_dbus_method_invocation_return_value (invocation, g_variant_new ("()"));
  g_main_loop_quit (loop);
} // ggimp_dbus_handle_quit

void
ggimp_dbus_handle_rgb_red (const gchar *method_name,
                           GDBusMethodInvocation *invocation,
                           GVariant *parameters)
{
  // Grab the parameter
  GVariant *param = g_variant_get_child_value (parameters, 0);
  // Grab the integer
  int color = g_variant_get_int32 (param);
  // Extract the red component
  int red = (color >> 16) & 255;
  // Convert it back to a GVariant
  GVariant *result = g_variant_new ("(i)", red);
  // And return it
  g_dbus_method_invocation_return_value (invocation, result);
} // gimp_gbus_handle_rgb_red

void
ggimp_dbus_handle_tile_stream_advance (const gchar *method_name,
                                       GDBusMethodInvocation *invocation,
                                       GVariant *parameters)
{
  // Grab the parameters
  int stream = 
    g_variant_get_int32 (g_variant_get_child_value (parameters, 0));
  // Validate
  if (! handler_validate_tile_stream (stream, invocation))
    return;
  // Advance and return
  GVariant *result = g_variant_new ("(i)", tile_stream_advance (stream));
  g_dbus_method_invocation_return_value (invocation, result);
} // ggimp_dbus_handle_tile_stream_advance

void
ggimp_dbus_handle_tile_stream_close (const gchar *method_name,
                                     GDBusMethodInvocation *invocation,
                                     GVariant *parameters)
{
  // Grab the parameters
  int stream = 
    g_variant_get_int32 (g_variant_get_child_value (parameters, 0));
  // Validate
  if (! handler_validate_tile_stream (stream, invocation))
    return;
  // Close and return
  tile_stream_close (stream);
  g_dbus_method_invocation_return_value (invocation, g_variant_new ("()"));
} // ggimp_dbus_handle_tile_stream_close

void
ggimp_dbus_handle_tile_stream_get (const gchar *method_name,
                                   GDBusMethodInvocation *invocation,
                                   GVariant *parameters)
{
  // Grab the parameters
  int stream = 
    g_variant_get_int32 (g_variant_get_child_value (parameters, 0));
  // Validate
  if (! handler_validate_tile_stream (stream, invocation))
    return;

  // Get the region
  GimpPixelRgn *rgn = tile_stream_get (stream);
  if (rgn == NULL)
    {
      LOG ("tile-stream-get: Failed to get tile.\n");
      SIGNAL_ERROR (invocation, "could not get tile");
      return; 
    } // if the region is null

  // Grab the bytes
  int size = rgn->rowstride * rgn->h;
  GVariant *bytes = g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE,
                                               rgn->data,
                                               size,
                                               sizeof (guint8));
  // Build the return value
  GVariantBuilder builder;
  g_variant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);
  g_variant_builder_add_value (&builder, g_variant_new_int32 (size));
  g_variant_builder_add_value (&builder, bytes);
  g_variant_builder_add_value (&builder, g_variant_new_int32 (rgn->bpp));
  g_variant_builder_add_value (&builder, g_variant_new_int32 (rgn->rowstride));
  g_variant_builder_add_value (&builder, g_variant_new_int32 (rgn->x));
  g_variant_builder_add_value (&builder, g_variant_new_int32 (rgn->y));
  g_variant_builder_add_value (&builder, g_variant_new_int32 (rgn->w));
  g_variant_builder_add_value (&builder, g_variant_new_int32 (rgn->h));

  // And we're done
  g_dbus_method_invocation_return_value (invocation, 
                                         g_variant_builder_end (&builder));
} // ggimp_dbus_handle_tile_stream_get

void
ggimp_dbus_handle_tile_stream_is_valid (const gchar *method_name,
                                        GDBusMethodInvocation *invocation,
                                        GVariant *parameters)
{
  // Grab the parameters
  int stream = 
    g_variant_get_int32 (g_variant_get_child_value (parameters, 0));
  // Do the computation and return
  GVariant *result = g_variant_new ("(i)", tile_stream_is_valid (stream));
  g_dbus_method_invocation_return_value (invocation, result);
} // ggimp_dbus_handle_tile_stream_is_valid

void
ggimp_dbus_handle_tile_stream_new (const gchar *method_name,
                                   GDBusMethodInvocation *invocation,
                                   GVariant *parameters)
{
  // Grab the parameters
  int image = 
    g_variant_get_int32 (g_variant_get_child_value (parameters, 0));
  int drawable = 
    g_variant_get_int32 (g_variant_get_child_value (parameters, 1));

  // Validate the parameters
  if (! gimp_image_is_valid (image))
    {
      LOG ("tile_stream_new: invalid image %d\n", image);
      report_invalid_parameter (invocation, 
                                "tile_stream_new",
                                1,
                                "image",
                                g_variant_get_child_value (parameters, 0));
      return;
    } // if it's an invalid image
  if (! gimp_drawable_is_valid (drawable))
    {
      LOG ("tile_stream_new: invalid drawable %d\n", drawable);
      report_invalid_parameter (invocation, 
                                "tile_stream_new",
                                2,
                                "drawable",
                                g_variant_get_child_value (parameters, 1));
      return;
    } // if it's an invalid drawable
  if (gimp_drawable_get_image (drawable) != image)
    {
      LOG ("tile_stream_new: drawable %d does not match image %d\n", 
           drawable, image);
      SIGNAL_ARGUMENT_ERROR (invocation, "drawable does not match image");
      return;
    } // if the items don't match.

  // Build the tile stream
  int stream = drawable_new_tile_stream (image, drawable);
  if (! tile_stream_is_valid (stream))
    {
      SIGNAL_ERROR (invocation, "could not create stream");
      return;
    } // if tile stream is invalid
  // Convert it back to a GVariant
  GVariant *result = g_variant_new ("(i)", stream);
  // And return it
  g_dbus_method_invocation_return_value (invocation, result);
} // ggimp_dbus_handle_tile_stream_new

void
ggimp_dbus_handle_tile_update (const gchar *method_name,
                               GDBusMethodInvocation *invocation,
                               GVariant *parameters)
{
  // Grab the parameters
  int stream = 
    g_variant_get_int32 (g_variant_get_child_value (parameters, 0));
  int size =
    g_variant_get_int32 (g_variant_get_child_value (parameters, 1));
  gsize realsize;
  GVariant *wrapped_data = g_variant_get_child_value (parameters, 2);
  guint8 *data = (guint8 *) g_variant_get_fixed_array (wrapped_data,
                                                       &realsize,
                                                       sizeof (guint8));
  if (size > realsize)
    {
      SIGNAL_ARGUMENT_ERROR (invocation, "size > number of bytes");
      return;
    }
  // Validate
  if (! handler_validate_tile_stream (stream, invocation))
    return;

  // Call the underlying function
  int result = tile_update (stream, size, data);
  // And return
  g_dbus_method_invocation_return_value (invocation, 
                                         g_variant_new ("(i)", result));
} // ggimp_dbus_handle_tile_update


// +------------------------+------------------------------------------
// | Standard DBus Handlers |
// +------------------------+

static void
alt_handle_method_call (GDBusConnection       *connection,
                        const gchar           *sender,
                        const gchar           *object_path,
                        const gchar           *interface_name,
                        const gchar           *method_name,
                        GVariant              *parameters,
                        GDBusMethodInvocation *invocation,
                        gpointer               user_data)
{
  static HandlerEntry alt_handlers[] =
    {
      { "ggimp_about",          ggimp_dbus_handle_about                },
      { "ggimp_quit",           ggimp_dbus_handle_quit                 },
      { "ggimp_rgb_red",        ggimp_dbus_handle_rgb_red              },
      { "tile_stream_advance",  ggimp_dbus_handle_tile_stream_advance  },
      { "tile_stream_close",    ggimp_dbus_handle_tile_stream_close    },
      { "tile_stream_get",      ggimp_dbus_handle_tile_stream_get      },
      { "tile_stream_is_valid", ggimp_dbus_handle_tile_stream_is_valid },
      { "tile_stream_new",      ggimp_dbus_handle_tile_stream_new      },
      { "tile_update",          ggimp_dbus_handle_tile_update          },
      { NULL,                   ggimp_dbus_handle_default              }
    };

  int i;
  // Loop through the list of handlers, looking for one that matches.
  for (i = 0; alt_handlers[i].name != NULL; i++)
    {
      if (g_strcmp0 (method_name, alt_handlers[i].name) == 0)
        {
          (*(alt_handlers[i].handler)) (method_name, invocation, parameters);
          return;
        } // if the name matches
    } // for each handler

  // If we've gotten this far, nothing has matched.  Give up.
  ggimp_dbus_handle_default (method_name, invocation, parameters);
} // alt_handle_method_call

static GVariant *
alt_handle_get_property (GDBusConnection  *connection,
                         const gchar      *sender,
                         const gchar      *object_path,
                         const gchar      *interface_name,
                         const gchar      *property_name,
                         GError          **error,
                         gpointer          user_data)
{
  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
               "No property %s found in interface %s.",
               property_name, 
               interface_name);

  // NULL signals failure
  return NULL;    
} // alt_handle_get_property

static gboolean
alt_handle_set_property (GDBusConnection  *connection,
                         const gchar      *sender,
                         const gchar      *object_path,
                         const gchar      *interface_name,
                         const gchar      *property_name,
                         GVariant         *value,
                         GError          **error,
                         gpointer          user_data)
{
  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
               "No property %s found in interface %s.",
               property_name, 
               interface_name);
  // false signals failure.
  return FALSE; 
}// alt_handle_set_property

static void
pdb_handle_method_call (GDBusConnection       *connection,
                        const gchar           *sender,
                        const gchar           *object_path,
                        const gchar           *interface_name,
                        const gchar           *method_name,
                        GVariant              *parameters,
                        GDBusMethodInvocation *invocation,
                        gpointer               user_data)
{
  gimp_dbus_handle_pdb_method_call (connection, 
                                    method_name, 
                                    parameters, 
                                    invocation);
} // pdb_handle_method_call

static GVariant *
pdb_handle_get_property (GDBusConnection  *connection,
                         const gchar      *sender,
                         const gchar      *object_path,
                         const gchar      *interface_name,
                         const gchar      *property_name,
                         GError          **error,
                         gpointer          user_data)
{
  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
               "No property %s found in interface %s.",
               property_name, 
               interface_name);

  // NULL signals failure
  return NULL;    
} // pdb_handle_get_property

static gboolean
pdb_handle_set_property (GDBusConnection  *connection,
                         const gchar      *sender,
                         const gchar      *object_path,
                         const gchar      *interface_name,
                         const gchar      *property_name,
                         GVariant         *value,
                         GError          **error,
                         gpointer          user_data)
{
  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
               "No property %s found in interface %s.",
               property_name, 
               interface_name);
  // false signals failure.
  return FALSE; 
}// pdb_handle_set_property

/**
 * What to do when the bus is acquired.
 */
static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  pdb_registration_id = 
    g_dbus_connection_register_object (connection,
                                       GIMP_DBUS_APPLICATION_OBJECT,
                                       pdbnode->interfaces[0],
                                       &pdb_interface_vtable,
                                       NULL,  /* user_data */
                                       NULL,  /* user_data_free_func */
                                       NULL); /* GERROR */
   
  alt_introspection_data = 
    g_dbus_node_info_new_for_xml (alt_introspection_xml, NULL);

  if (alt_introspection_data == NULL)
    {
      // Hack!
      fprintf (stderr, "Could not create alt_introspection_data\n");
      exit (1);
    }

  alt_registration_id = 
    g_dbus_connection_register_object (connection,
                                       GIMP_DBUS_APPLICATION_OBJECT,
                                       alt_introspection_data->interfaces[0],
                                       &alt_interface_vtable,
                                       NULL,  /* user_data */
                                       NULL,  /* user_data_free_func */
                                       NULL); /* GERROR */
} // on_bus_acquired

/**
 * What to do when the service name is acquired.
 */
static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  // Right now, we do nothing.  But the standard in D-Bus is to have
  // a callback, so it's in place in case we decide to do something 
  // in the future.
} // on_name_acquired

/**
 * Handling the loss of a name on the bus.
 */
static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  LOG ("Lost name %s", name);
  g_main_loop_quit (loop);
} // on_name_lost


// +-----------------+-------------------------------------------------
// | General Helpers |
// +-----------------+

/**
 * Replace one character by another.
 */
gchar *
strrep (gchar *str, gchar target, gchar replacement)
{
  gchar *tmp = str;

  while ((tmp = strchr (tmp, target)) != NULL)
    *tmp = replacement;

  return str;
} // strrep


// +-----------------+-------------------------------------------------
// | Type Conversion |
// +-----------------+

/**
 * Convert a GimpPDBArtType (type information for a GIMP parameter) to 
 * a GVariant signature (type information for GVariants).
 */
const GVariantType *
gimp_dbus_pdb_type_to_signature (GimpPDBArgType type)
{
  const GVariantType *result;

  switch (type)
    {
    case GIMP_PDB_COLOR:
      result = G_VARIANT_TYPE_INT32;
      break;
    case GIMP_PDB_INT32:
      result = G_VARIANT_TYPE_INT32;
      break;
    case GIMP_PDB_INT16:
      result = G_VARIANT_TYPE_INT16;
      break;
    case GIMP_PDB_INT8:
      result = G_VARIANT_TYPE_BYTE;
      break;
    case GIMP_PDB_FLOAT:
      result = G_VARIANT_TYPE_DOUBLE;
      break;
    case GIMP_PDB_STRING:
      result = G_VARIANT_TYPE_STRING;
      break;
    case GIMP_PDB_STRINGARRAY:
      result = G_VARIANT_TYPE_STRING_ARRAY;
      break;
    case GIMP_PDB_INT32ARRAY:
      result = ((const GVariantType *) "ai");
      break;
    case GIMP_PDB_INT16ARRAY:
      result = ((const GVariantType *) "an");
      break;
    case GIMP_PDB_INT8ARRAY:
      result = ((const GVariantType *) "ay");
      break;
    case GIMP_PDB_FLOATARRAY:
      result = ((const GVariantType *) "ad");
      break;
    case GIMP_PDB_DISPLAY:
      result = G_VARIANT_TYPE_INT32;
      break;
    case GIMP_PDB_IMAGE:
      result = G_VARIANT_TYPE_INT32;
      break;
    case GIMP_PDB_LAYER:
      result = G_VARIANT_TYPE_INT32;
      break;
    // We should have covered everything.  To be safe, we have
    // a default type of 32-bit integers.   TODO: Pick a better
    // default type.
    default:
      result = G_VARIANT_TYPE_INT32;
      break;
    } // switch

  return result;
} // gimp_dbus_pdb_param_to_signature

/**
 * Convert a GVariant to a GimpParam and make param point to it.
 * Returns success/failure.
 */
gboolean
gimp_dbus_g_variant_to_gimp_param (GVariant         *parameter,
                                   GimpParamDef     *paramdef,
                                   GimpParam        *param)
{
  gsize size;
  unsigned long nchildren;
  int arraycounter = 0;
  char* tempc;
  gint32 temp32;
  gint16 temp16;
  gdouble tempd;
  gchar** sarray = NULL;
  gint32* array32 = NULL;
  gint16* array16 = NULL;
  gdouble* arrayd = NULL;
  guchar r, g, b;      //int rgb
  GimpRGB tempRGB; 

  // Make sure that types match.  FIX TO HANDLE MULTIPLE REPRESENTATIONS
  // OF COLORS.
  const gchar *paramtype = 
    (const gchar *) gimp_dbus_pdb_type_to_signature (paramdef->type);
  if (! g_variant_type_equal (paramtype,
                              g_variant_get_type (parameter)))
    {
      LOG ("you're not converting types correctly");
      return FALSE;
    } // g_variant_type_equal

  param->type = paramdef->type;

  switch (paramdef->type)
    {
    // Special case: Colors.  Need to convert from whatever
    // type we received to a gimp_rgb.  Right now, we only
    // handle integers.  (Will need fixing above to handle
    // other representations
    case GIMP_PDB_COLOR:
      temp32 = g_variant_get_int32 (parameter);
      LOG ("  parameter  '%s' is %d, will be color",
           paramdef->name, param->data.d_int32);   
      // Unpack r, g, and b as uchars 
      r = (guchar) (temp32 >> 16);
      g = (guchar) ((temp32 >> 8) & 255);
      b = (guchar) (temp32 & 255);      
      // Create the RGB
      gimp_rgb_set_uchar (&tempRGB, r, g, b);
      // Set data field to the RGB we created
      param->data.d_color = tempRGB;
   
      return TRUE;

    // All of these types are effectively integers.
    case GIMP_PDB_INT32:
    case GIMP_PDB_DISPLAY:
    case GIMP_PDB_IMAGE:  
    case GIMP_PDB_LAYER:
    case GIMP_PDB_CHANNEL:
    case GIMP_PDB_DRAWABLE:
    case GIMP_PDB_SELECTION:
    case GIMP_PDB_BOUNDARY:
    case GIMP_PDB_VECTORS:
      param->data.d_int32 = g_variant_get_int32 (parameter);
      LOG ("  parameter '%s' is %d", paramdef->name, param->data.d_int32);
      return TRUE;

    case GIMP_PDB_FLOAT:
      param->data.d_float = g_variant_get_double (parameter);
      LOG ("  parameter '%s' is %lf", paramdef->name, param->data.d_float);
      return TRUE;

    case GIMP_PDB_STRING:
      param->data.d_string = g_variant_dup_string (parameter, NULL);
      LOG ("  parameter '%s' is %s", paramdef->name, param->data.d_string);
      return TRUE;

    case GIMP_PDB_STRINGARRAY:
      nchildren = g_variant_n_children (parameter);
         
      sarray = g_try_malloc ((nchildren+1) * sizeof (gchar *));
      for (arraycounter = 0; arraycounter<nchildren; arraycounter++)
        {
          g_variant_get_child (parameter, arraycounter, "s", &tempc);
          sarray[arraycounter] = tempc; 
        }
      sarray[nchildren] = NULL;
      param->data.d_stringarray = sarray;
      LOG ("  parameter '%s' is an array of strings", paramdef->name);
      return TRUE; 

    case GIMP_PDB_INT32ARRAY:
      nchildren = g_variant_n_children (parameter);
      array32 = g_try_malloc ((nchildren) * sizeof (gint32));
      if (array32 == NULL)
        return FALSE;
      for (arraycounter = 0; arraycounter<nchildren; arraycounter++)
        {
          g_variant_get_child (parameter, arraycounter, "i", &temp32);
          array32[arraycounter] = temp32; 
        }
      param->data.d_int32array = array32;
      LOG ("  parameter '%s' is an array of int32s", paramdef->name);
      return TRUE; 

    case GIMP_PDB_INT16ARRAY:
      nchildren = g_variant_n_children (parameter);
      array16 = g_try_malloc ((nchildren) * sizeof (gint16));
      if (array16 == NULL)
        return FALSE;
      for (arraycounter = 0; arraycounter<nchildren; arraycounter++)
        {
          g_variant_get_child (parameter, arraycounter, "n", &temp16);
          array16[arraycounter] = (gint16) temp16;  
        }
      param->data.d_int16array = array16;
      LOG ("  parameter '%s' is an array of int16s", paramdef->name);
      return TRUE; 

    case GIMP_PDB_INT8ARRAY:
      param->data.d_int8array = 
        (guint8 *) g_variant_get_fixed_array (parameter,
                                              &size,
                                              sizeof (guint8));
      LOG ("  parameter '%s' is an array of int8s of size %lu", 
           paramdef->name, size);
      return TRUE; 

    case GIMP_PDB_FLOATARRAY:
      nchildren = g_variant_n_children (parameter);
      arrayd = g_try_malloc ((nchildren) * sizeof (gdouble));
      for (arraycounter = 0; arraycounter<nchildren; arraycounter++)
        {
          g_variant_get_child (parameter, arraycounter, "d", &tempd);
          arrayd[arraycounter] = tempd;  
        }
      param->data.d_floatarray = arrayd;
      LOG ("  parameter '%s' is an array of floats", paramdef->name);
      return TRUE; 

    default:
      return FALSE;
    } // switch
} // gimp_dbus_g_variant_to_gimp_param


/**
 * Convert a GVariant to a newly allocated array of GimpParams.
 */
gboolean
gimp_dbus_g_variant_to_gimp_array (GVariant       *parameters, 
                                   GimpParamDef   *types,
                                   GimpParam     **actuals)
{
  int  nparams = g_variant_n_children (parameters);
  GimpParam *result = g_new (GimpParam, nparams);
  int i;

  for (i = 0; i < nparams; i++)
    {
      GVariant *param = g_variant_get_child_value (parameters, i);
      if (! gimp_dbus_g_variant_to_gimp_param (param, 
                                               &types[i], 
                                               &(result[i])))
        {
          g_free (result);
          return FALSE;
        }
    } // for each parameter

  *actuals = result;
  return TRUE;
} // gimp_dbus_g_variant_to_gimp_array


/**
 * Convert a GimpParam to a newly allocated GVariant.
 */
GVariant *
gimp_dbus_gimp_param_to_g_variant (GimpParam value, int *asize)
{
  GVariantBuilder abuilder;
  int index = 0;             // Index into the array.
  int icolor;                // A color represented as an integer
  guchar r, g, b;            // Components of the color.

  switch (value.type)
    {
      // Special case: Colors
    case GIMP_PDB_COLOR:
      gimp_rgb_get_uchar (&(value.data.d_color), &r, &g, &b);
      guint gur = CLAMP ((gint) r, 0, 255);
      guint gug = CLAMP ((gint) g, 0, 255);
      guint gub = CLAMP ((gint) b, 0, 255);
      icolor = (gur << 16) | (gug << 8) | (gub << 0);
      return g_variant_new ("i", icolor);

    case GIMP_PDB_INT32:
    case GIMP_PDB_DISPLAY:
    case GIMP_PDB_IMAGE:
    case GIMP_PDB_LAYER:
    case GIMP_PDB_CHANNEL:
    case GIMP_PDB_DRAWABLE:
    case GIMP_PDB_SELECTION:
    case GIMP_PDB_BOUNDARY:
    case GIMP_PDB_VECTORS:
      return g_variant_new ("i", value.data.d_int32);

    case GIMP_PDB_STRING:
      return g_variant_new ("s", value.data.d_string);
      
    case GIMP_PDB_INT16:
      return g_variant_new ("n", value.data.d_int16);

    case GIMP_PDB_INT8:
      return g_variant_new ("i", value.data.d_int8);

    case GIMP_PDB_FLOAT:
      return g_variant_new ("d", value.data.d_float);
      
    case GIMP_PDB_STRINGARRAY:
      g_variant_builder_init (&abuilder, G_VARIANT_TYPE_STRING_ARRAY);
      for (index = 0; index< *asize; index++)
        {
          g_variant_builder_add_value 
            (&abuilder, g_variant_new("s", 
                                      value.data.d_stringarray[index]));
        } // for 
      return g_variant_builder_end(&abuilder);

    case GIMP_PDB_INT32ARRAY:    
      g_variant_builder_init (&abuilder, ((const GVariantType *) "ai"));
         
      for (index = 0; index< *asize; index++)
        {
          g_variant_builder_add_value 
            (&abuilder, g_variant_new("i",
                                      value.data.d_int32array[index]));
         
        }
      return g_variant_builder_end(&abuilder);

    case GIMP_PDB_INT16ARRAY:
      g_variant_builder_init (&abuilder, G_VARIANT_TYPE_TUPLE);
      for (index = 0; index< *asize; index++)
        {
          g_variant_builder_add_value 
            (&abuilder, g_variant_new("n", 
                                      value.data.d_int16array[index]));
         
        }
      return g_variant_builder_end(&abuilder);

      //INT8ARRAY
    case GIMP_PDB_INT8ARRAY:
      //use uchar to get each bit from array
      index = 0;
      GVariant *bytes = g_variant_new_fixed_array (G_VARIANT_TYPE_BYTE,
                                                   value.data.d_int8array,
                                                   *asize,
                                                   sizeof (guint8));
      LOG ("Working with int8arrays, created a something of type %s.",
           g_variant_get_type_string (bytes));
      return bytes;
#ifdef OLD
      g_variant_builder_init (&abuilder, ((const GVariantType *) "ai"));

      for (index = 0; index< *asize; index++)
        {
          guint8 byte = value.data.d_int8array[index];
          g_variant_builder_add_value (&abuilder, g_variant_new("i", byte));
          LOG ("bytes[%d] = %d\n", index, byte);
        } // for
   
      return g_variant_builder_end (&abuilder);
#endif
      //FLOATARRAY
    case GIMP_PDB_FLOATARRAY:
      g_variant_builder_init (&abuilder, G_VARIANT_TYPE_TUPLE);
      for (index = 0; index< *asize; index++)
        {
          g_variant_builder_add_value 
            (&abuilder, g_variant_new("d", 
                                      value.data.d_floatarray[index]));
         
        }
      return g_variant_builder_end(&abuilder);
    default:
      return NULL;
    }
} // gimp_dbus_gimp_param_to_g_variant

//if arraytype, get the length in the gimpparam before it
GVariant *
gimp_dbus_gimp_array_to_g_variant (GimpParam *values, int nvalues)
{
  GVariantBuilder  builder;   
  int              i; 
  gint             arrsize = 0; 
  GVariant        *val;      

  g_variant_builder_init (&builder, G_VARIANT_TYPE_TUPLE);

  for (i = 0; i < nvalues; i++)
    {
      if ((values[i].type == GIMP_PDB_STRINGARRAY)
          || (values[i].type == GIMP_PDB_INT8ARRAY)
          || (values[i].type == GIMP_PDB_INT32ARRAY) 
          || (values[i].type == GIMP_PDB_INT16ARRAY) 
          || (values[i].type == GIMP_PDB_FLOATARRAY))
        {
          arrsize = values[i-1].data.d_int32;
        }

      val = gimp_dbus_gimp_param_to_g_variant (values[i], &arrsize);
      if (val == NULL)
        {
          LOG ("failed to add GimpParam ");
          return NULL;
        } // if (val == NULL)

      g_variant_builder_add_value (&builder, val);
    } // for each value

  return g_variant_builder_end (&builder);
} // gimp_dbus_gimp_array_to_g_variant

/**
 * Convert a GimpParamDef (from GIMP) to a GDBusArgInfo (for DBus).
 */
GDBusArgInfo *
gimp_dbus_pdb_param_to_arginfo (GimpParamDef param)
{
  gchar *name = strrep (g_strdup (param.name), '-', '_');

  const gchar *type = 
    (const gchar *) gimp_dbus_pdb_type_to_signature (param.type);
  GDBusArgInfo *result = g_dbus_arg_new (name, type, NULL);
  if (result == NULL)
    {
      // fprintf (stderr, "Could not allocate argument.\n");
      return NULL;
    }
  return result;
} // gimp_dbus_pdb_param_to_arginfo


// +------------------------+------------------------------------------
// | GDBus Helper Functions |
// +------------------------+

/**
 * Creates GDBusNodeInfo to register on DBUS
 */
GDBusNodeInfo *
g_dbus_node_info_new (gchar *path,
          GDBusInterfaceInfo **interfaces,
          GDBusNodeInfo **nodes,
          GDBusAnnotationInfo **annotations)
{
  GDBusNodeInfo *node = g_try_malloc (sizeof (GDBusNodeInfo));
  if (node == NULL)
    return NULL;
  node->ref_count = 1;
  node->path = path;
  node->interfaces = interfaces;
  node->nodes = nodes;
  node->annotations = annotations;
  return node;
} // g_dbus_node_info_new

/**
 * Build information on one argument to a method.  Returns NULL if
 * it is unable to build.
 */
static GDBusArgInfo *
g_dbus_arg_new (gchar                *name,
                const gchar          *signature,
                GDBusAnnotationInfo **annotations)
{
  GDBusArgInfo *arg = g_try_malloc (sizeof (GDBusArgInfo));
  if (arg == NULL)
    return NULL;
  
  arg->ref_count = 0;
  arg->name = name;
  arg->signature = (gchar *) signature;
  arg->annotations = annotations;
  
  return arg;
} // g_dbus_arg_new

/**
 * Build method information.
 */
GDBusMethodInfo *
g_dbus_method_info_build (gchar *name,
                          GDBusArgInfo **in_args, 
                          GDBusArgInfo **out_args,
                          GDBusAnnotationInfo **annotations)
{
  GDBusMethodInfo *method = g_try_malloc (sizeof (GDBusMethodInfo));
  if (method == NULL)
    return NULL;
  method->name = name;
  method->ref_count = 1;
  method->in_args = in_args;
  method->out_args = out_args;
  method->annotations = annotations;
  return method;
} // g_dbus_method_info_build

/* *
 * Makes a gimpnames struct with all 
 * GIMP proc names and # of proc names 
 */
struct gimpnames *
procnamesbuilder ()
{
  struct gimpnames *gimpnamelist;
  gimpnamelist =  g_try_malloc (sizeof (struct gimpnames));
  gimp_procedural_db_query (".*", ".*", ".*", ".*", ".*", ".*", ".*", 
                            &gimpnamelist->nprocs, &gimpnamelist->procnames);
  return gimpnamelist;
} //gimpnames

/**
 * Given a PDB proc name, returns the method info.
 */
static GDBusMethodInfo *
generate_pdb_method_info (gchar *proc_name)
{

  // Lots and lots and lots of fields for getting info.
  gchar           *proc_blurb;
  gchar           *proc_help;
  gchar           *proc_author;
  gchar           *proc_copyright;
  gchar           *proc_date;
  GimpPDBProcType  proc_type;
  GimpParamDef    *formals;       
  gint             nparams;
  GimpParamDef    *return_types;
  gint             nreturn_vals;

  // Parts of our return information
  gint i;                        // Counter variable
  GDBusArgInfo   **args = NULL;         // Argument info
  GDBusArgInfo   **returns = NULL;      // Return value info

  // Get the information 

  if (! gimp_procedural_db_proc_info (proc_name,
                                      &proc_blurb,
                                      &proc_help,
                                      &proc_author,
                                      &proc_copyright,
                                      &proc_date,
                                      &proc_type,
                                      &nparams, &nreturn_vals,
                                      &formals, &return_types))
    {
      return NULL;
    }
 

  // Process the parameters
  if (nparams > 0)
    {
      args = g_new (GDBusArgInfo *, nparams+1);
      for (i = 0; i < nparams; i++)
        {
         
          args[i] = gimp_dbus_pdb_param_to_arginfo (formals[i]);
          g_assert (args[i] != NULL);
        } // for
      // Terminate the array
      args[nparams] = NULL;
    } // if (nparams > 0)

  // Process the return values
  if (nreturn_vals > 0)
    {
      returns = g_new (GDBusArgInfo *, nreturn_vals+1);
      for (i = 0; i < nreturn_vals ; i++)
        {
         
          returns[i] = gimp_dbus_pdb_param_to_arginfo (return_types[i]);
          g_assert (returns[i] != NULL);
        } // for
      // Terminate the array
      returns[nreturn_vals] = NULL;
    } // if (nreturn_vals > 0)

  //used to be proc_name
  GDBusMethodInfo * result =
    g_dbus_method_info_build (strrep (g_strdup (proc_name), '-', '_'),
                              args,
                              returns,
                              NULL);


  return result;
} // gimp_dbus_pdb_method

// methodmaker- returns GDBusMethodInfo with all proc info 
GDBusMethodInfo **
methodmaker (struct gimpnames *nms)
{
  GDBusMethodInfo **nfo = g_new (GDBusMethodInfo *, nms->nprocs + 1); 
  if (nfo == NULL)
    {
      fprintf (stderr, "Could not allocate method information.\n");
      return NULL;
    } // if (nfo == NULL)

  int i;

  for (i = 0; i < nms->nprocs; i++)
    {
      nfo[i] = generate_pdb_method_info (nms->procnames[i]);
    } // for

  nfo[nms->nprocs] = NULL;
  
  return nfo;

}//methodmaker


// +------------------------------+------------------------------------
// | Primary Method Call Handlers |
// +------------------------------+

/**
 * What to do when we get a method call.
 */
int
gimp_dbus_handle_pdb_method_call (GDBusConnection       *connection,
                                  const gchar           *method_name,
                                  GVariant              *parameters,
                                  GDBusMethodInvocation *invocation)
{
  LOG ("gimp_dbus_handle_pdb_method_call (%p, %s, %p, %p)",
       connection, method_name, parameters, invocation);

  // Information we extract about the procedure.
  gchar           *proc_name;
  gchar           *proc_blurb;
  gchar           *proc_help;
  gchar           *proc_author;
  gchar           *proc_copyright;
  gchar           *proc_date;
  GimpPDBProcType  proc_type;
  GimpParamDef    *formals;
  gint             nparams;
  GimpParamDef    *return_types;
  gint             nreturn_vals;

  GimpParam       *actuals = NULL;   // The arguments to the call.
  GimpParam       *values = NULL;    // The return values from the call.
  gint             nvalues;          // Number of return values.
  GVariant        *result;

  // Normal case; PDB functions
  proc_name = strrep (g_strdup (method_name), '_', '-');

  // Look up the information on the procedure in the PDB
  if (! gimp_procedural_db_proc_info (proc_name,
                                      &proc_blurb,
                                      &proc_help,
                                      &proc_author,
                                      &proc_copyright,
                                      &proc_date,
                                      &proc_type,
                                      &nparams, &nreturn_vals,
                                      &formals, &return_types))
    {
      LOG ("invalid procedure call - no such method %s", proc_name);
      SIGNAL_ARGUMENT_ERROR (invocation, "Invalid method: '%s'", method_name);
      return FALSE;
    } // if we can't get the information
  LOG ("Successfully extracted PDB info.");

  // Check the number of parameters
  //nparams = g_variant_n_children (parameters);

  // build the parameters
  if (! gimp_dbus_g_variant_to_gimp_array (parameters, formals, &actuals))
    {
      LOG ("invalid procedure call - could not convert parameters");
      SIGNAL_ARGUMENT_ERROR (invocation, 
                             "Invalid parameter in call to '%s'",
                             method_name);
      return FALSE;
    } // if we could not convert to a gimp_array

  // Do the call
  LOG ("About to run %s", proc_name);
  values = gimp_run_procedure2 (proc_name, &nvalues, nparams, actuals);
  LOG ("Ran %s", proc_name);

  // Check to make sure that the call succeeded.  
  if (values == NULL)
    {
      LOG ("Call to %s failed", proc_name);
      SIGNAL_ERROR (invocation, 
                    "call to %s failed for unknown reason", 
		    proc_name);
      return FALSE;
    } // If call to procedure2 fails

  if (values[0].data.d_status != GIMP_PDB_SUCCESS)
    {
      int status = values[0].data.d_status;
      char *reason = "for an unknown reason";
      switch (status)
        {
          case GIMP_PDB_EXECUTION_ERROR:
            reason = "with an execution error";
            break;
          case GIMP_PDB_CALLING_ERROR:
            reason = "with invalid inputs";
            break;
          case GIMP_PDB_PASS_THROUGH:
            reason = "with a pass-through error";
            break;
          case GIMP_PDB_CANCEL:
            reason = "because it was canceled";
            break;
        } // switch (status)
      SIGNAL_ERROR (invocation, "call to %s failed %s", proc_name, reason);
      return FALSE;
    } // if gimp reports an error

  // Convert the values back to a GVariant
  result = gimp_dbus_gimp_array_to_g_variant (values+1, nvalues-1);

  // Return via DBus
  g_dbus_method_invocation_return_value (invocation, result);

  // Cleanup: TODO
  // g_variant_unref (result);
  return TRUE;
} // gimp_dbus_handle_pdb_method_call


// +-------------------------+-----------------------------------------
// | GIMP Plugin Boilerplate |
// +-------------------------+

GimpPlugInInfo PLUG_IN_INFO =
  { NULL, NULL, query, run };//PLUG_IN_INFO

MAIN()

static void
query (void)
{
#ifdef EXTENSION
  // Install the default server.  Since this is an extension, it should
  // start immediately.
  gimp_install_procedure ("GimpDBusServer",
                          "A Gimp D-Bus Server",
                          "Publishes the PDB and some other procedures on D-Bus",
                          "Samuel A. Rebelsky and a host of his students.",
                          "Copyright (c) 2012-13 Samuel A. Rebelsky",
                          "2012-13",
                          GIMP_DBUS_MENU "DBus Server",
                          NULL, 
                          GIMP_EXTENSION,
                          0, 0,
                          NULL, NULL);
#endif

  // Install support for restarting the server.
  static GimpParamDef server_args[] =
    {
      { GIMP_PDB_INT32, "run-mode", "Run mode" }
    };
  gimp_install_procedure ("GimpDBusServer",
                          "A Gimp D-Bus Server",
                          "Publishes the PDB and some other procedures on D-Bus",
                          "Samuel A. Rebelsky and a host of his students.",
                          "Copyright (c) 2012-13 Samuel A. Rebelsky",
                          "2012-13",
                          GIMP_DBUS_MENU "DBus Server",
                          NULL, 
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (server_args), 0,
                          server_args, NULL);

  // Install a silly procedure (for testing)
  static GimpParamDef silly_args[] =
    {
      { GIMP_PDB_INT32, "run-mode", "Run mode" }
    };
  static GimpParamDef silly_return[] =
    {
      { GIMP_PDB_STRING, "message", "a message from the server." }
    };
  gimp_install_procedure ("silly",
                          "A silly experiment",
                          "An experiment with serving multiple functions",
                          "Samuel A. Rebelsky",
                          "Copyright (c) 2013 Samuel A. Rebelsky",
                          "2013",
                          NULL,
                          NULL, 
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (silly_args), 
                          G_N_ELEMENTS (silly_return),
                          silly_args, 
                          silly_return);
} // query

static void
run_default (const gchar *name,
             gint              nparams,
             const GimpParam  *param,
             gint             *nreturn_vals,
             GimpParam       **return_vals)
{
  static GimpParam  values[1];
  /* Setting mandatory output values */
  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
} // run_default

static void
run_server (const gchar      *name,
            gint              nparams,
            const GimpParam  *param,
            gint             *nreturn_vals,
            GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

#ifdef DEBUG
  LOG ("Waiting for debugger, pid is %d", getpid ());
  sleep (1);
  LOG ("Done waiting.");
#endif

  /* Setting mandatory output values */
  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
  guint owner_id;

  GString *xml = g_string_new ("");

  GDBusInterfaceInfo **interfaces = g_try_malloc 
    (2 * sizeof (GDBusInterfaceInfo *));
  GDBusInterfaceInfo *interface = g_try_malloc (sizeof (GDBusInterfaceInfo));
  // TODO: Handle failed mallocs!

  interface->ref_count = 1;
  interface->name = GIMP_DBUS_INTERFACE_PDB;

  struct gimpnames *gnames;
  GDBusMethodInfo **info;

  gnames = procnamesbuilder();
  info = methodmaker (gnames);

  interface->methods = info;
  interface->signals = NULL; 
  interface->properties = NULL;
  interface->annotations = NULL;

  interfaces[0] = interface;
  interfaces[1] = NULL;

  g_dbus_interface_info_generate_xml (interface, 0, xml);
   
  g_type_init ();

  LOG ("About to make node.");
  pdbnode = g_dbus_node_info_new (NULL, interfaces, NULL, NULL);
  LOG ("Made node.");

  LOG ("About to own name");
  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             GIMP_DBUS_SERVICE,
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);
  LOG ("Owned name");

  // Event loop.  Wait for functions to get called asynchronously.
  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);

  // We've escaped the loop.  Time to clean up.
  g_bus_unown_name (owner_id);
  g_dbus_node_info_unref (pdbnode);
 
  // update all the changes we have made to the user interface 
  gimp_displays_flush(); 

  return;
} // run_server

static void
run_silly (const gchar      *name,
           gint              nparams,
           const GimpParam  *param,
           gint             *nreturn_vals,
           GimpParam       **return_vals)
{
  static GimpParam results[2];
  results[0].type = GIMP_PDB_STATUS;
  results[0].data.d_status = GIMP_PDB_SUCCESS;
  results[1].type = GIMP_PDB_STRING;
  results[1].data.d_string = "hello world";

  *nreturn_vals = 2;
  *return_vals = results;
} // run_silly

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  LOG ("Running '%s' in process %d", name, getpid ());
  static RunTableEntry runners[] =
    {
      { "GimpDBusServer",       run_server     },
      { "silly",                run_silly      },
      { NULL,                   run_default    }
    };

  int i;
  // Loop through the list of handlers, looking for one that matches.
  for (i = 0; runners[i].name != NULL; i++)
    {
      if (g_strcmp0 (name, runners[i].name) == 0)
        {
          (*(runners[i].proc)) (name, nparams, param, 
                                nreturn_vals, return_vals);
          return;
        } // if the name matches
    } // for each runner

  // If we've made it through all the handlers, just use the deafult.
  LOG ("Could not find '%s'", name);
  run_default (name, nparams, param, nreturn_vals, return_vals);
} // run

