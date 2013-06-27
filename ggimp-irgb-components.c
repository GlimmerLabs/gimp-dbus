/*
 * irgb-components.c
 *   GIMP plugin to extract components of an integer-encoded RGB
 *   color.
 *
 * Copyright (c) 2013 Samuel A. Rebelsky
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


// +---------+---------------------------------------------------------
// | Headers |
// +---------+

#include <libgimp/gimp.h>    
#include <libgimp/gimpui.h>    


// +--------+----------------------------------------------------------
// | Macros |
// +--------+

#define RETURNS(COLOR)  static GimpParamDef COLOR ## _returns[] = { { GIMP_PDB_INT32, #COLOR, "The " #COLOR " component." } }

#define INSTALL(COLOR) \
  gimp_install_procedure ("ggimp-irgb-" #COLOR, \
			  "Extract " #COLOR " component", \
			  "Extract the " #COLOR "component from " \
                            "an encoded RGB color (created by irgb-new)", \
			  "Samuel A. Rebelsky", \
			  "Copyright (c) 2013 Samuel A. Rebelsky", \
			  "2013", \
                          NULL, \
			  NULL, \
			  GIMP_PLUGIN, \
			  G_N_ELEMENTS (args), \
                          G_N_ELEMENTS (COLOR ## _returns), \
			  args,  \
                          COLOR ## _returns)

// +-----------------+------------------------------------------------
// | Predeclarations |
// +-----------------+

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


// +-------------------------+-----------------------------------------
// | GIMP Plugin Boilerplate |
// +-------------------------+

GimpPlugInInfo PLUG_IN_INFO =
  { NULL, NULL, query, run };//PLUG_IN_INFO

MAIN()

static void
query (void)
{
  static GimpParamDef args[] =
    {
      { GIMP_PDB_INT32, "color", "Integer-encoded RGB color" }
    };
  RETURNS(red);
  INSTALL(red);
  RETURNS(green);
  INSTALL(green);
  RETURNS(blue);
  INSTALL(blue);
} // query

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *params,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam results[2];
  int shift = 0;

  // Prepare the result
  results[0].type = GIMP_PDB_STATUS;
  results[0].data.d_status = GIMP_PDB_SUCCESS;
  results[1].type = GIMP_PDB_INT32;
  *nreturn_vals = 2;
  *return_vals = results;

  // Grab the parameters
  int irgb = params[0].data.d_int32;

  // Determine the shift for the component
  if (g_strcmp0 (name, "ggimp-irgb-alpha") == 0)
    shift = 24;
  if (g_strcmp0 (name, "ggimp-irgb-red") == 0)
    shift = 16;
  else if (g_strcmp0 (name, "ggimp-irgb-green") == 0)
    shift = 8;
  else if (g_strcmp0 (name, "ggimp-irgb-blue") == 0)
    shift = 0;
  else
    {
      fprintf (stderr, "Could not determine shift for %s\n", name);
      results[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      return;
    } // default

  fprintf (stderr, "For %s using a shift of %d.\n", name, shift);

  // And fill in the results
  results[1].data.d_int32 = (irgb >> shift) & 255;
} // run
