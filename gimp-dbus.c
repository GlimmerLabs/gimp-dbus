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
#include <stdlib.h>        
#include <stdio.h>      
#include <stdarg.h>
#include <string.h>


// +-----------+-------------------------------------------------------
// | Constants |
// +-----------+

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
 * Where we put this service in the menu.
 */
#define GIMP_DBUS_MENU "<Toolbox>/Xtns/MediaScript/DBus Server"


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


// +-----------------+------------------------------------------------
// | Predeclarations |
// +-----------------+

/** 
 * Handle a call to method_name on connection, using the 
 * specified parameters.  Send the result of the call to
 * invocation.
 */
void gimp_dbus_handle_method_call (GDBusConnection       *connection,
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
static void run   (const gchar      *name,
                   gint              nparams,
                   const GimpParam  *param,
                   gint             *nreturn_vals,
                   GimpParam       **return_vals);

/**
 * DBus method call handler.
 */
static void handle_method_call (GDBusConnection       *connection,
                                const gchar           *sender,
                                const gchar           *object_path,
                                const gchar           *interface_name,
                                const gchar           *method_name,
                                GVariant              *parameters,
                                GDBusMethodInvocation *invocation,
                                gpointer               user_data);

/**
 * DBus get-propery handler.
 */
static GVariant *handle_get_property (GDBusConnection  *connection,
                                      const gchar      *sender,
                                      const gchar      *object_path,
                                      const gchar      *interface_name,
                                      const gchar      *property_name,
                                      GError          **error,
                                      gpointer          user_data);

/**
 * DBus set-property handler.
 */
static gboolean handle_set_property (GDBusConnection  *connection,
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


// +---------+---------------------------------------------------------
// | Globals |
// +---------+

/**
 * The GDBusNodeInfo to be published to the dbus.
 */
static GDBusNodeInfo *finalnode = NULL;

/**
 * Information on the registration id for this process.
 */
static guint registration_id;

/**
 * The standard DBus handlers.
 */
static const GDBusInterfaceVTable interface_vtable =
  {
    handle_method_call,
    handle_get_property,
    handle_set_property
  };


// +------------------------+------------------------------------------
// | Standard DBus Handlers |
// +------------------------+

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  gimp_dbus_handle_method_call (connection, 
  			        method_name, 
			        parameters, 
			        invocation);
} // gimp_dbus_handle_method_call

static GVariant *
handle_get_property (GDBusConnection  *connection,
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
} //handle_get_property

static gboolean
handle_set_property (GDBusConnection  *connection,
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
}//handle_set_property

/**
 * What to do when the bus is acquired.
 */
static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  registration_id = 
    g_dbus_connection_register_object (connection,
                                       GIMP_DBUS_APPLICATION_OBJECT,
                                       finalnode->interfaces[0],
                                       &interface_vtable,
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
  exit (1);
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
      result = ((const GVariantType *) "ai");
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
  unsigned long nchildren;
  int arraycounter = 0;
  char* tempc;
  gint32 temp32;
  gdouble tempd;
  gchar** sarray = NULL;
  gint32* array32 = NULL;
  gint16* array16 = NULL;
  guint8* array8 = NULL;
  gdouble* arrayd = NULL;

  // Make sure that types match
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
	  g_variant_get_child (parameter, arraycounter, "i", &temp32);
	  array8[arraycounter] = (gint16) temp32;  
	}
      param->data.d_int16array = array16;
      LOG ("  parameter '%s' is an array of int16s", paramdef->name);
      return TRUE; 

    case GIMP_PDB_INT8ARRAY:
      nchildren = g_variant_n_children (parameter);
      array8 = g_try_malloc ((nchildren) * sizeof (guint8));
      for (arraycounter = 0; arraycounter<nchildren; arraycounter++)
	{
	  g_variant_get_child (parameter, arraycounter, "i", &temp32);
	  array8[arraycounter] = (guint8) temp32;  
	}
      param->data.d_int8array = array8;
      LOG ("  parameter '%s' is an array of int8s", paramdef->name);
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
  int arrcounter = 0;

  switch (value.type)
    {
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
      for(arrcounter = 0; arrcounter< *asize; arrcounter++)
	{
	  g_variant_builder_add_value 
	    (&abuilder, g_variant_new("s", 
				      value.data.d_stringarray[arrcounter]));

	  fprintf(stderr, "%s",value.data.d_stringarray[arrcounter] );
	  
	}
      return g_variant_builder_end(&abuilder);

    case GIMP_PDB_INT32ARRAY:    
      g_variant_builder_init (&abuilder, ((const GVariantType *) "ai"));
         
      for(arrcounter = 0; arrcounter< *asize; arrcounter++)
	{
	  g_variant_builder_add_value 
	    (&abuilder, g_variant_new("i",
				      value.data.d_int32array[arrcounter]));
	 
	}
      return g_variant_builder_end(&abuilder);

    case GIMP_PDB_INT16ARRAY:
      g_variant_builder_init (&abuilder, G_VARIANT_TYPE_TUPLE);
      for(arrcounter = 0; arrcounter< *asize; arrcounter++)
	{
	  g_variant_builder_add_value 
	    (&abuilder, g_variant_new("n", 
				      value.data.d_int16array[arrcounter]));
	 
	}
      return g_variant_builder_end(&abuilder);

      //INT8ARRAY
    case GIMP_PDB_INT8ARRAY:
      //use uchar to get each bit from array
      arrcounter = 0;
      

      g_variant_builder_init (&abuilder, ((const GVariantType *) "ai"));

      for(arrcounter = 0; arrcounter< *asize; arrcounter++)

	{
	  g_variant_builder_add_value 
	    (&abuilder, g_variant_new("i", 
				      value.data.d_int8array[arrcounter]));
	}
   
      return g_variant_builder_end(&abuilder);

      //FLOATARRAY
    case GIMP_PDB_FLOATARRAY:
      g_variant_builder_init (&abuilder, G_VARIANT_TYPE_TUPLE);
      for (arrcounter = 0; arrcounter< *asize; arrcounter++)
	{
	  g_variant_builder_add_value 
	    (&abuilder, g_variant_new("d", 
				      value.data.d_floatarray[arrcounter]));
	 
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


// +------------------+------------------------------------------------
// | Primary Handlers |
// +------------------+

/**
 * What to do when we get a method call.
 */
void
gimp_dbus_handle_method_call (GDBusConnection       *connection,
                              const gchar           *method_name,
                              GVariant              *parameters,
                              GDBusMethodInvocation *invocation)
{
  LOG ("gimp_dbus_handle_method_call (%p, %s, %p, %p)",
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
      g_dbus_method_invocation_return_error (invocation,
                                             G_IO_ERROR,
                                             G_IO_ERROR_INVALID_ARGUMENT,
                                             "Invalid method: '%s'",
                                             method_name);
      return;
    } // if we can't get the information
  LOG ("Successfully extracted PDB info.");

  // Check the number of parameters
  //nparams = g_variant_n_children (parameters);

  // build the parameters
  if (! gimp_dbus_g_variant_to_gimp_array (parameters, formals, &actuals))
    {
      LOG ("invalid procedure call - could not convert parameters");
      g_dbus_method_invocation_return_error (invocation,
                                             G_IO_ERROR,
                                             G_IO_ERROR_INVALID_ARGUMENT,
                                             "Invalid parameter in call to '%s'",
                                             method_name);
    } // if we could not convert to a gimp_array

  // Do the call
  LOG ("About to run %s", proc_name);
  values = gimp_run_procedure2 (proc_name, &nvalues, nparams, actuals);
  LOG ("Ran %s", proc_name);

  // Check to make sure that the call succeeded.  TODO!

  // Convert the values back to a GVariant
  result = gimp_dbus_gimp_array_to_g_variant (values+1, nvalues-1);

  // Return via DBus
  g_dbus_method_invocation_return_value (invocation, result);
  LOG("actually, it's this one");

  // Cleanup: TODO
  // g_variant_unref (result);
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
  // saving in constant memory the data that I expect from Gimp
  static GimpParamDef args[] =
    {
      {
	// determines the run-mode whether it is interactive or non-interactive
	GIMP_PDB_INT32, 
	"run-mode",
	"RM"
      },

      {
	// Input image
	GIMP_PDB_IMAGE,
	"image",
	"Input image"
      },

      {
	// Input drawable
	GIMP_PDB_DRAWABLE,
	"drawable",
	"Input drawable"
      }
    };


  gimp_install_procedure (
			  "GimpDbusServer",
			  "Node Info Test",
			  "Publishes Node on the DBUS",
			  "Samuel A. Rebelsky and a host of students.",
			  "Copyright (c) 2012-13 Samuel A. Rebelsky "
                           "and some students",
			  "2012-13",
                          GIMP_DBUS_MENU,
			  NULL, 
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (args), 0,
			  args, NULL);
} // query

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  int               pid;    // Process ID; for debugging

  pid = getpid ();
  LOG ("pid is %d", pid);

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
   

  GMainLoop *loop;

  g_type_init ();

  LOG ("About to make node.");
  finalnode = g_dbus_node_info_new (NULL, interfaces, NULL, NULL);
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
  g_dbus_node_info_unref (finalnode);
 
  // update all the changes we have made to the user interface 
  gimp_displays_flush(); 

  return;
} // run

