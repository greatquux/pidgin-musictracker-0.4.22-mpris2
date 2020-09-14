#include "musictracker.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <string.h>

#ifndef WIN32
#include <config.h>
#else
#include <config-win32.h>
#endif

#include "gettext.h"
#define _(String) dgettext (PACKAGE, String)

#define mpris2_debug(fmt, ...)	purple_debug(PURPLE_DEBUG_INFO, "MPRIS2", \
					fmt, ## __VA_ARGS__);
#define mpris2_error(fmt, ...)	purple_debug(PURPLE_DEBUG_ERROR, "MPRIS2", \
					fmt, ## __VA_ARGS__);

void
get_mpris2_info(struct TrackInfo* ti)
{
  DBusGConnection *connection;
  GError *error;
  DBusGProxy *proxy, *proxy2;
  char **name_list;
  char **name_list_ptr;
  GValue get_value = {0,{{0}}};
  GValue h = {0,{{0}}};
  GValue pos = {0,{{0}}};
  
  error = NULL;
  connection = dbus_g_bus_get (DBUS_BUS_SESSION,
                               &error);
  if (connection == NULL)
  {
    mpris2_error ("Failed to open connection to bus: %s\n",
                error->message);
    g_error_free (error);
    return;
  }
  
  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);
  if (!dbus_g_proxy_call (proxy, "ListNames", &error, G_TYPE_INVALID,
                          G_TYPE_STRV, &name_list, G_TYPE_INVALID))
  {
    if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
    {
      mpris2_error ("Caught remote method exception %s: %s",
                    dbus_g_error_get_name (error),
                    error->message);
    }
    else
    {
      mpris2_error ("Error: %s\n", error->message);
    }
    g_error_free (error);
    return;
  }
  
  for (name_list_ptr = name_list; *name_list_ptr; name_list_ptr++)
  {
    if (strncmp("org.mpris.MediaPlayer2.", *name_list_ptr, strlen("org.mpris.MediaPlayer2.")) == 0)
    {
      mpris2_debug("Found player %s\n", *name_list_ptr);
      proxy2 = dbus_g_proxy_new_for_name (connection,
                                          *name_list_ptr,
                                          "/org/mpris/MediaPlayer2",
                                          "org.freedesktop.DBus.Properties");
      if (!dbus_g_proxy_call (proxy2, "Get", &error, 
                              G_TYPE_STRING, "org.mpris.MediaPlayer2.Player",
                              G_TYPE_STRING, "Metadata",
                              G_TYPE_INVALID,
                              G_TYPE_VALUE, &get_value,
                              G_TYPE_INVALID))
      {
        if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
        {
          mpris2_error ("Caught remote method exception %s: %s",
                        dbus_g_error_get_name (error),
                        error->message);
        }
        else
        {
          mpris2_error ("Error: %s\n", error->message);
        }
        g_error_free(error);
        goto cleanup;
      }
      GHashTable *dict = g_value_get_boxed(&get_value);
      gpointer tmp;
      tmp = g_hash_table_lookup(dict, "xesam:artist");
      if (tmp != NULL)
      {
        GStrv strv = g_value_get_boxed(g_hash_table_lookup(dict, "xesam:artist"));
        g_strlcpy(ti->artist, *strv, STRLEN);
        ti->artist[STRLEN-1] = 0;
        g_strfreev(strv);
      }

      tmp = g_hash_table_lookup(dict, "xesam:album");
      if (tmp != NULL)
      {
        g_strlcpy(ti->album,
                  g_value_get_string(g_hash_table_lookup(dict, "xesam:album")),
                  STRLEN);
        ti->album[STRLEN-1] = 0;
      }

      tmp = g_hash_table_lookup(dict, "xesam:title");
      if (tmp != NULL)
      {
        g_strlcpy(ti->track,
                  g_value_get_string(g_hash_table_lookup(dict, "xesam:title")),
                  STRLEN);
        ti->track[STRLEN-1] = 0;
      }

      if (!dbus_g_proxy_call (proxy2, "Get", &error, 
                              G_TYPE_STRING, "org.mpris.MediaPlayer2.Player",
                              G_TYPE_STRING, "PlaybackStatus",
                              G_TYPE_INVALID,
                              G_TYPE_VALUE, &h,
                              G_TYPE_INVALID))
      {
        if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
        {
          mpris2_error ("Caught remote method exception %s: %s",
                        dbus_g_error_get_name (error),
                        error->message);
        }
        else
        {
          mpris2_error ("Error: %s\n", error->message);
        }
        g_error_free (error);
        goto cleanup;
      }
/*
      // get position
      if (!dbus_g_proxy_call (proxy2, "Get", &error, 
                              G_TYPE_STRING, "org.mpris.MediaPlayer2.Player",
                              G_TYPE_STRING, "Position",
                              G_TYPE_INVALID,
                              G_TYPE_VALUE, &pos,
                              G_TYPE_INVALID))
      {
        if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
        {
          mpris2_error ("Caught remote method exception %s: %s",
                        dbus_g_error_get_name (error),
                        error->message);
        }
        else
        {
          mpris2_error ("Error: %s\n", error->message);
        }
        g_error_free (error);
        goto cleanup;
      }
      gint64 position = g_value_get_int64 (&pos);
      ti->currentSecs = position/1000000;
      // [end] get position
*/     
      const gchar *status = g_value_get_string(&h);
      if (g_strcmp0(status, "Playing") == 0)
        ti->status = PLAYER_STATUS_PLAYING;
      else if (g_strcmp0(status, "Paused") == 0)
        ti->status = PLAYER_STATUS_PAUSED;
      else if (g_strcmp0(status, "Stopped") == 0)
        ti->status = PLAYER_STATUS_STOPPED;
      goto cleanup;
    }
  }
  ti->status = PLAYER_STATUS_CLOSED;
  
 cleanup:
  g_strfreev(name_list);
}
