/**
 * test-bytes-get.c
 *   PDB function that makes an array of bytes.
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
  static GimpParamDef returns[] =
    {
      { GIMP_PDB_INT32, "nbytes", "The number of bytes" },
      { GIMP_PDB_INT8ARRAY, "bytes", "The bytes" }
    };
  gimp_install_procedure ("test-bytes-get",
			  "Creates a test array of bytes.",
			  "An experiment",
			  "Samuel A. Rebelsky",
			  "Copyright (c) 2013 Samuel A. Rebelsky",
			  "2013",
                          NULL,
			  NULL, 
			  GIMP_PLUGIN,
			  0,
                          G_N_ELEMENTS (returns),
			  NULL,
                          returns);
} // query

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *params,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam results[3];

  int ints[12] = { 11, 4, 127, 0, 14, 0, 255, 11, 5, 6, 0, 7 };
  static guint8 data[12];
  int i;
  for (i = 0; i < 12; i++)
    {
      data[i] = (guint8) ints[i];
      fprintf (stderr, "data[%d] = %d\n", i, data[i]);
    } // for

  // Build the result
  results[0].type = GIMP_PDB_STATUS;
  results[0].data.d_status = GIMP_PDB_SUCCESS;
  results[1].type = GIMP_PDB_INT32;
  results[1].data.d_int32 = G_N_ELEMENTS (data);
  results[2].type = GIMP_PDB_INT8ARRAY;
  results[2].data.d_int8array = data;
  *nreturn_vals = 3;
  *return_vals = results;
} // run
