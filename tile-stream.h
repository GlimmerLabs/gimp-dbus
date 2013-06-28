#ifndef __TILE_STREAM_H__
#define __TILE_STREAM_H__

/**
 * tile-stream.h
 *   Simple support for streams of tiles using a slightly simpler
 *   API than that the one built into the GIMP.  Intended primarily
 *   as support for sending data over D-Bus, but potentially usable
 *   for other purposes.
 *
 * Copyright (c) 2013 Samuel A. Rebelsky.  All rights reserved.
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


// +-------+-----------------------------------------------------------
// | Notes |
// +-------+

/*
  // Sample usage
  stream = drawable_new_tile_stream (image, drawable);
  assert (stream >= 0);
  while ((tile = tile_stream_get (stream) != NULL)
    {
      ...
      tile_stream_advance (stream);
    } // while
  tile_stream_close (stream);
 */


// +---------+---------------------------------------------------------
// | Headers |
// +---------+

#include <libgimp/gimp.h>


// +--------------+----------------------------------------------------
// | Constructors |
// +--------------+

/**
 * Get a new tile stream for a drawable.
 * Returns a negative number if it cannot create the stream.
 */
int drawable_new_tile_stream (int image, int drawable);


// +---------+---------------------------------------------------------
// | Methods |
// +---------+

/**
 * Advance to the next tile.
 */
gboolean tile_stream_advance (int id);

/**
 * Close the tile stream.  Should always be called when you are done
 * with the stream.
 */
void tile_stream_close (int id);

/**
 * Get the data for the current tile.  Returns NULL if no tiles remain.
 */
GimpPixelRgn *tile_stream_get (int id);

/**
 * Update the pixels in the current tile.
 */
int tile_update (int id, guchar *data);

/**
 * Determine if an id is valid.
 */
int tile_stream_is_valid (int id);

#endif // __TILE_STREAM_H__
