/**
 * ggimp-rgb-parse.c
 *   PDB function to get the rgb color corresponding to a name of a color
 *   known in GIMP
 *
 * Copyright (c) 2013 Mark Lewis and Christine Tran.
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
  static GimpParamDef args[] =
    {
      { GIMP_PDB_STRING, "color-name", "The name of a color" }
    }; //args
  static GimpParamDef results[] =
    {
      { GIMP_PDB_INT32, "color", "The RGB color packed into 32 bits" }
    }; // results

  // Tell the GIMP about our plugin.
  gimp_install_procedure (
    "ggimp-rgb-parse",                                       // Name
    "Return the RGB integer corresponding to a color name",  // Blurb
    "Return an RGB packed into a 32 bit integer given its name",   // Help
    "Mark Lewis and Christine Tran",  // Author
    "Copyright (c) Mark Lewis and Christine Tran."
    "All rights reserved.",
                                                        // Copyright
    "2013",                                             // Year
    "",                                                 // Path
    NULL,                                               // Image types
    GIMP_PLUGIN,                                        // Type
    1,                                                  // Number of params
    1,                                                  // Number of return vals
    args,                                               // Param descriptions
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
  static GimpParam results[2];
  results[0].type = GIMP_PDB_STATUS;
  results[0].data.d_status = GIMP_PDB_SUCCESS;
  *nreturn_vals = 2;
  *return_vals = results;

  const gchar *color_name = params[0].data.d_string;
  GimpRGB rgb;
  gint length;  //String is null-terminated
  gint r, g, b, icolor;
  gboolean checkProc;
  length = -1; //String is null-terminated

  checkProc = gimp_rgb_parse_name(&rgb, color_name, length);

  if (! checkProc)
    {
      results[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    }// if (! checkProc)
  else
    {
      //RGB stored 0-1, scale to 0-255
      r = (gint)(rgb.r * 255);      
      g = (gint)(rgb.g * 255);      
      b = (gint)(rgb.b * 255);  
 
      icolor = (r << 16) | (g << 8) | (b << 0);
      results[1].type = GIMP_PDB_INT32;
      results[1].data.d_int32 = icolor;
    }
} // run
