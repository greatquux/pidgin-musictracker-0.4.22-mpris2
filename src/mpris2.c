/* Adds MPRIS2 support to Pidgin without weird CPU issues I encoutered
 * and upgrades to using newer GDBusConnection methods
 * from https://aur.archlinux.org/packages/pidgin-musictracker-mpris2/
 * modified by greatquux after much trial, error, and remembering
 * classes about C from 20 years ago
 * MJR 10/7/20 fixed memory leaks, properly unref GVariants
 */

#include "musictracker.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <string.h>
#include <gio/gio.h>

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
  GDBusConnection *connection;
  GError *error;
  GDBusProxy *proxy, *proxy2;
  gchar **name_list;
  gchar **name_list_ptr;
  GVariant *answer, *value, *tmp;
  GVariantIter *iter;
  gchar *key, *tmpstr;
  
  error = NULL; connection = NULL; proxy = NULL; proxy2 = NULL; 
  answer = NULL; value = NULL; tmp = NULL;

//mpris2_error ("getting connection\n");
 
  connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
  if (connection == NULL)
  {
    mpris2_error ("Failed to open connection to bus: %s\n",
                error->message);
    g_error_free (error);
    goto cleanup;
  }

//mpris2_error ("got connection\n");
  
  proxy = g_dbus_proxy_new_sync (connection,
                                 G_DBUS_PROXY_FLAGS_NONE,
				 NULL,
				 "org.freedesktop.DBus",
				 "/org/freedesktop/DBus",
				 "org.freedesktop.DBus",
				 NULL,
				 &error);
  if (error != NULL) {
	mpris2_error ("Error: %s", error->message);
	g_error_free (error);
	goto cleanup;
  }

//mpris2_error ("got proxy\n");

  answer = g_dbus_proxy_call_sync (proxy, "ListNames", NULL, G_DBUS_CALL_FLAGS_NONE,
			  -1, NULL, &error);
  if ((error != NULL) || (answer == NULL)) {
	mpris2_error ("Error: %s", error->message);
	g_error_free (error);
	goto cleanup;
  }

//mpris2_error ("got answer\n");
    
  tmp = g_variant_get_child_value (answer, 0);
  name_list = g_variant_dup_strv (tmp, NULL);
  if (answer != NULL) g_variant_unref(answer);
  if (tmp != NULL) g_variant_unref(tmp);
  
  for (name_list_ptr = name_list; *name_list_ptr; name_list_ptr++)
  {
//mpris2_debug("name list: %s\n",*name_list_ptr);
    if (strncmp("org.mpris.MediaPlayer2.", *name_list_ptr, strlen("org.mpris.MediaPlayer2.")) == 0)
    {
      mpris2_debug("Found player %s\n", *name_list_ptr);
      proxy2 = g_dbus_proxy_new_sync (connection,
		      		G_DBUS_PROXY_FLAGS_NONE,
				NULL,
                                *name_list_ptr,
                                "/org/mpris/MediaPlayer2",
                                "org.freedesktop.DBus.Properties",
				NULL,
				&error);

	// get playback status
      answer = g_dbus_proxy_call_sync (proxy2, "Get", 
	      		g_variant_new("(ss)","org.mpris.MediaPlayer2.Player","PlaybackStatus"),
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);
      if ((error != NULL) || (answer == NULL)) {
	mpris2_error ("Error: %s", error->message);
	g_error_free (error);
	goto cleanup;
      }

//mpris2_debug("Got answer PlaybackStatus\n");
//mpris2_debug("it is type: %s\n", g_variant_get_type_string(answer));

	tmp = g_variant_get_child_value (answer, 0);

//mpris2_debug("Got child\n");
//mpris2_debug("it is type: %s\n", g_variant_get_type_string(tmp));

	const gchar *status = g_variant_print(tmp, FALSE);

//mpris2_debug("status %s\n",status);

	  if (answer != NULL) g_variant_unref(answer);
	  if (tmp != NULL) g_variant_unref(tmp);

	// if player is stopped or paused, continue and see if 
	// we can find a player that is Playing
      ti->status = PLAYER_STATUS_CLOSED;
      if (g_strcmp0(status, "<'Playing'>") == 0)
        ti->status = PLAYER_STATUS_PLAYING;
      else if (g_strcmp0(status, "<'Paused'>") == 0) {
        ti->status = PLAYER_STATUS_PAUSED;
	continue;
      }	
      else if (g_strcmp0(status, "<'Stopped'>") == 0) {
        ti->status = PLAYER_STATUS_STOPPED;
	continue;
      }

     // if playing, get metadata 
      answer = g_dbus_proxy_call_sync (proxy2, "Get", 
			      g_variant_new("(ss)","org.mpris.MediaPlayer2.Player","Metadata"),
			      G_DBUS_CALL_FLAGS_NONE,
			      -1,
			      NULL,
			      &error);
  
      if ((error != NULL) || (answer == NULL)) {
	mpris2_error ("Error: %s", error->message);
	g_error_free (error);
	goto cleanup;
      }

//mpris2_debug("Got answer Get\n");

  g_variant_get(answer,"(v)", &tmp);
  g_variant_get(tmp, "a{sv}", &iter);
  while(g_variant_iter_loop(iter, "{sv}", &key, &value)) {
//mpris2_debug("key is %s\n", key);
    if(strncmp(key, "xesam:title", 11) == 0) {
	g_variant_get(value, "s", &tmpstr);
	g_strlcpy(ti->track, tmpstr, STRLEN);
//mpris2_debug("title:%s\n",ti->track);
    }
    else if(strncmp(key, "xesam:album", 11) == 0) {
      if(g_variant_is_container(value)) {
        g_variant_get_child(value, 0, "s", &tmpstr);
	g_strlcpy(ti->album, tmpstr, STRLEN);
//mpris2_debug("album_cont:%s\n",ti->album);
      }
      else {
        g_variant_get(value, "s", &tmpstr);
	g_strlcpy(ti->album, tmpstr, STRLEN);
//mpris2_debug("album:%s\n",ti->album);
      }
    }
    else if(strncmp(key, "xesam:artist", 12) == 0) {
      if(g_variant_is_container(value)) {
        g_variant_get_child(value, 0, "s", &tmpstr);
	g_strlcpy(ti->artist, tmpstr, STRLEN);
//mpris2_debug("artist_cont:%s\n",ti->artist);
      }
      else {
        g_variant_get(value, "s", &tmpstr);
	g_strlcpy(ti->artist, tmpstr, STRLEN);
//mpris2_debug("artist:%s\n",ti->artist);
      }
    }
  } /* while */

//mpris2_debug("trying to free answer");
  if (answer != NULL) g_variant_unref(answer);
//mpris2_debug("trying to free value");
  if (value != NULL) g_variant_unref(value);
//mpris2_debug("trying to free tmp");
  if (tmp != NULL) g_variant_unref(tmp);
//mpris2_debug("title: %s\talbum: %s\tartist: %s\n",ti->track,ti->album,ti->artist);
  
  goto cleanup;	 // if playing, just get out (ignore other ones which could be paused)
    } /* if */


  } /* for */
  
 cleanup:
  g_strfreev(name_list);
//mpris2_debug("trying to free proxy");
  if (proxy != NULL) g_object_unref(proxy);
//mpris2_debug("trying to free proxy2");
  if (proxy2 != NULL) g_object_unref(proxy2);
//mpris2_debug("trying to free connection");
  if (connection != NULL) g_object_unref(connection);
}
