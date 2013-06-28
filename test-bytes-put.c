/**
 * test-bytes-put.c
 *   PDB function that reads an array of bytes.
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
  static GimpParamDef formals[] =
    {
      { GIMP_PDB_INT32, "nbytes", "The number of bytes" },
      { GIMP_PDB_INT8ARRAY, "bytes", "The bytes" }
    };
  static GimpParamDef returns[] =
    {
      { GIMP_PDB_INT32, "sum", "A sum of the bytes" }
    };
  gimp_install_procedure ("test-bytes-put",
			  "Lets the server put bytes",
			  "Experiment!",
			  "Samuel A. Rebelsky",
			  "Copyright (c) 2013 Samuel A. Rebelsky",
			  "2013",
                          NULL,
			  NULL, 
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (formals),
                          G_N_ELEMENTS (returns),
			  formals,
                          returns);
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
  int nbytes = params[0].data.d_int32;
  guint8 *data = params[1].data.d_int8array;

  // Compute the result
  int sum = 0;
  int i;
  for (i = 0; i < nbytes; i++)
    {
      fprintf (stderr, "data[%d] == %d\n", i, data[i]);
      sum += data[i];
    }

  // Build the result
  results[0].type = GIMP_PDB_STATUS;
  results[0].data.d_status = GIMP_PDB_SUCCESS;
  results[1].type = GIMP_PDB_INT32;
  results[1].data.d_int32 = sum;
  *nreturn_vals = 2;
  *return_vals = results;
} // run
