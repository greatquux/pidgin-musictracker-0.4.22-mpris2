/*
 * musictracker
 * dbusbird.c
 * retrieve track info from songbird using the dbusbird interface
 *
 * Copyright (C) 2008, Jon TURNEY <jon.turney@dronecode.org.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 *
 */

#include "musictracker.h"
#include "utils.h"
#include <string.h>

/*
  Dbusbird provides a simple dbus interface to Songbird

  http://addons.songbirdnest.com/addon/181

  Dbusbird doesn't currently detect when the player is stopped after completing a playlist,
  which is a bit unfortunate..., in fact the getStatus result seems to get confused sometimes
*/

static gboolean
dbusbird_dbus_string(DBusGProxy *proxy, const char *method, char* dest)
{
  char *str = 0;
  GError *error = 0;

  if (!dbus_g_proxy_call_with_timeout (proxy, method, DBUS_TIMEOUT, &error,
                                       G_TYPE_INVALID,
                                       G_TYPE_STRING, &str,
                                       G_TYPE_INVALID))
    {
      trace("Failed to make dbus call %s: %s", method, error->message);
      return FALSE;
    }
  
  assert(str);
  strncpy(dest, str, STRLEN);
  dest[STRLEN-1] = 0;
  g_free(str);

  trace("dbusbird_dbus_string: '%s' => '%s'", method, dest);

  return TRUE;
}

void
get_dbusbird_info(struct TrackInfo* ti)
{
  static DBusGProxy *proxy = 0;
  char status[STRLEN];

  ti->status = PLAYER_STATUS_CLOSED;

  if (!dbus_g_running("org.mozilla.songbird")) {
    return;
  }

  if (!proxy)
    {
      proxy = dbus_g_proxy_new_for_name (connection,
                                         "org.mozilla.songbird",
                                         "/org/mozilla/songbird",
                                         "org.mozilla.songbird");
    }

  if (!dbusbird_dbus_string(proxy, "getStatus", status))
    {
      return;
    }

  ti->player = "Songbird";

  if (strcmp(status, "stopped") == 0) {
    ti->status = PLAYER_STATUS_STOPPED;
    return;
  } else if (strcmp(status, "playing") == 0) {
    ti->status = PLAYER_STATUS_PLAYING;
  } else { // paused
    ti->status = PLAYER_STATUS_PAUSED;
  }

  ti->currentSecs = 0;

  {
    char buf[STRLEN];
    int hours, mins, secs;

    dbusbird_dbus_string(proxy, "getLength", buf);
    if (sscanf(buf, "%d:%d:%d", &hours, &mins, &secs) == 3)
      {
        ti->totalSecs = hours * 3600 + mins*60 + secs;
      }
  }

  dbusbird_dbus_string(proxy, "getArtist", ti->artist);
  dbusbird_dbus_string(proxy, "getAlbum", ti->album);
  dbusbird_dbus_string(proxy, "getTitle", ti->track);
}
