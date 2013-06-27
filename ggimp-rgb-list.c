/**
 * ggimp-rgb-list.c
 *   PDB function to list all the color names that GIMP knows about.
 *
 * Copyright (c) 2013 Mark Lewis, Samuel A. Rebelsky, and Christine Tran.
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
#include <glib.h>


// +--------------------------+----------------------------------------
// | Function Predeclarations |
// +--------------------------+

static void query (void);
static void run (const gchar *name,
                       gint nparams,
                       const GimpParam *params,
                       gint *nreturn_vals,
                       GimpParam **return_vals);


// +-------------+-----------------------------------------------------
// | Boilerplate |
// +-------------+

/**
 * The four key functions.  This structure *must* be called PLUG_IN_INFO.
 */
GimpPlugInInfo PLUG_IN_INFO =
  {
    NULL,
    NULL,
    query,
    run
  };

/**
 * Indicate that we're ready to begin the main part of the code.
 */
MAIN ()


// +-------------------------+-----------------------------------------
// | Standard GIMP Functions |
// +-------------------------+

static void 
query (void)
{
  // Build the description of the parameters that the function expects.  
  // Each parameter has a type, a name, and a description
  // All plug-ins must take a run-mode.  

  static GimpParamDef results[] =
    {
      { GIMP_PDB_INT32, "ncolors", "the number of colors returned" },
      { GIMP_PDB_STRINGARRAY, "colors", "a list of pre-defined rgb colors" }
    }; // results

  // Tell the GIMP about our plugin.
  gimp_install_procedure (
    "ggimp-rgb-list",                                   // Name
    "List all of the predefined colors",                // Blurb
    "List the names of all of the GIMP predefined colors",      // Help
    "Mark Lewis, Samuel A. Rebelsky & Christine Tran",  // Author
    "Copyright (c) Mark Lewis, Samuel A. Rebelsky, and Christine Tran."
    "All rights reserved.",
                                                        // Copyright
    "2013",                                             // Year
    "",                                                 // Path
    NULL,                                               // Image types
    GIMP_PLUGIN,                                        // Type
    0,                                                  // Number of params
    2,                                                  // Number of return vals
    NULL,                                               // Param descriptions
    results                                             // Return descriptions
    );
} // query

static void
run (const gchar            *name,
           gint              nparams,
           const GimpParam  *params,
           gint             *nreturn_vals,
           GimpParam       **return_vals)
{
  static GimpParam results[3];
  results[0].type = GIMP_PDB_STATUS;
  results[0].data.d_status = GIMP_PDB_SUCCESS;
  *nreturn_vals = 3;
  *return_vals = results;

  gint length;
  static const gchar **names;
  GimpRGB *colors = NULL;
  length = gimp_rgb_list_names (&names, &colors);

  results[1].type = GIMP_PDB_INT32;
  results[1].data.d_int32 = length;
  results[2].type = GIMP_PDB_STRINGARRAY;
  results[2].data.d_stringarray = (gchar **) names;
} // run
