/**
 * ggimp-irgb-new.c
 *   PDB function to create new integer-encoded RGB color.
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
  static GimpParamDef irgb_new_args[] =
    {
      { GIMP_PDB_INT32, "red", "Red component" },
      { GIMP_PDB_INT32, "green", "Green component" },
      { GIMP_PDB_INT32, "blue", "Blue component" }
    };
  static GimpParamDef irgb_new_return[] =
    {
      { GIMP_PDB_INT32, "color", "An irgb color." }
    };
  gimp_install_procedure ("ggimp-irgb-new",
			  "Generate an integer-encoded RGB color",
			  "Generate an integer-encoded RGB color",
			  "Samuel A. Rebelsky",
			  "Copyright (c) 2013 Samuel A. Rebelsky",
			  "2013",
                          NULL,
			  NULL, 
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (irgb_new_args), 
                          G_N_ELEMENTS (irgb_new_return),
			  irgb_new_args, 
                          irgb_new_return);
} // query

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *params,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam results[2];

  // Grab the parameters
  int r = params[0].data.d_int32;
  int g = params[1].data.d_int32;
  int b = params[2].data.d_int32;

  // Compute the color
  int irgb = (r << 16) | (g << 8) | b;

  // Build the result
  results[0].type = GIMP_PDB_STATUS;
  results[0].data.d_status = GIMP_PDB_SUCCESS;
  results[1].type = GIMP_PDB_INT32;
  results[1].data.d_int32 = irgb;
  *nreturn_vals = 2;
  *return_vals = results;
} // run
