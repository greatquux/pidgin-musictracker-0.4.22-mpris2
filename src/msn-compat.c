/*
 * musictracker
 * msn-compat.c
 * retrieve track info being sent to MSN
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

#include <windows.h>
#include <musictracker.h>
#include <utils.h>

#ifndef WIN32
#include <config.h>
#else
#include <config-win32.h>
#endif

#include "gettext.h"
#define _(String) dgettext (PACKAGE, String)

// put up a hidden window called "MsnMsgrUIManager", which things know to send a WM_COPYDATA message to, 
// containing strangely formatted string, for setting the now-playing status of MSN Messenger
// this might be broken by the sending side only sending to the first window of the class "MsnMsgrUIManager"
// it finds if there are more than one (for e.g., if the real MSN Messenger is running as well :D)
//
// Useful description of the message format: http://kentie.net/article/nowplaying/index.htm
// although we rely on the fact that players generate a very limited subset of the possible
// messages to be able to parse the message back into the track details...

static struct TrackInfo msnti;

static
void process_message(wchar_t *MSNTitle)
{
  char *s = wchar_to_utf8(MSNTitle);
  static char player[STRLEN] = "";
  char enabled[STRLEN], format[STRLEN], title[STRLEN], artist[STRLEN], album[STRLEN], uuid[STRLEN];
  enabled[0] = '0';
  format[0] = 0;

  // this has to be escaped quite carefully to prevent literals being interpreted as metacharacters by the compiler or in the pcre pattern
  // so yes, four \ before a 0 is required to match a literal \0 in the regex :-)
  // and marking the regex as ungreedy is a lot easier than writing \\\\0([^\\\\0]*)\\\\0 :)
  pcre *re1 = regex("^(.*)\\\\0Music\\\\0(.*)\\\\0(.*)\\\\0(.*)\\\\0(.*)\\\\0(.*)\\\\0(.*)\\\\0", PCRE_UNGREEDY);
  pcre *re2 = regex("^(.*)\\\\0Music\\\\0(.*)\\\\0(.*)\\\\0(.*)\\\\0(.*)\\\\0(.*)\\\\0", PCRE_UNGREEDY);
  pcre *re3 = regex("^(.*)\\\\0Music\\\\0(.*)\\\\0(.*)\\\\0(.*)\\\\0(.*)\\\\0", PCRE_UNGREEDY);
  pcre *re4 = regex("^(.*)\\\\0Music\\\\0(.*)\\\\0(.*) - (.*)\\\\0$", 0);

  if (capture(re1, s, strlen(s), player, enabled, format, artist, title, album, uuid) > 0)
    {
      trace("player '%s', enabled '%s', format '%s', title '%s', artist '%s', album '%s', uuid '%s'", player, enabled, format, title, artist, album, uuid);

      msnti.player = player;
      strncpy(msnti.artist, artist, STRLEN);
      msnti.artist[STRLEN-1] = 0;
      strncpy(msnti.album, album, STRLEN);
      msnti.album[STRLEN-1] = 0;
      strncpy(msnti.track, title, STRLEN);
      msnti.track[STRLEN-1] = 0;
    }
  else if (capture(re2, s, strlen(s), player, enabled, format, title, artist, album) > 0)
    {
      trace("player '%s', enabled '%s', format '%s', title '%s', artist '%s', album '%s'", player, enabled, format, title, artist, album);

      msnti.player = player;
      strncpy(msnti.artist, artist, STRLEN);
      msnti.artist[STRLEN-1] = 0;
      strncpy(msnti.album, album, STRLEN);
      msnti.album[STRLEN-1] = 0;
      strncpy(msnti.track, title, STRLEN);
      msnti.track[STRLEN-1] = 0;
    }
  else if (capture(re3, s, strlen(s), player, enabled, format, title, artist) > 0)
    {
      // Spotify likes this format
      trace("player '%s', enabled '%s', format '%s', title '%s', artist '%s' ", player, enabled, format, title, artist);

      msnti.player = player;
      strncpy(msnti.artist, artist, STRLEN);
      msnti.artist[STRLEN-1] = 0;
      strncpy(msnti.track, title, STRLEN);
      msnti.track[STRLEN-1] = 0;
    }
  else if (capture(re4, s, strlen(s), player, enabled, artist, title) > 0)
    {
      trace("player '%s', enabled '%s', artist '%s', title '%s'", player, enabled, artist, title);

      msnti.player = player;
      strncpy(msnti.artist, artist, STRLEN);
      msnti.artist[STRLEN-1] = 0;
      msnti.album[0] = 0;
      strncpy(msnti.track, title, STRLEN);
      msnti.track[STRLEN-1] = 0;
    }
  else
    {
      msnti.status = PLAYER_STATUS_STOPPED;
    }

  pcre_free(re1);
  pcre_free(re2);
  pcre_free(re3);
  pcre_free(re4);

  //
  // Winamp seems to generate messages with enabled=1 but all the fields empty when it stops, so we need to
  // check if any fields have non-empty values as well as looking at that flag
  //
  if ((strncmp(enabled, "1", 1) == 0) &&
      ((strlen(msnti.artist) > 0) || (strlen(msnti.track) > 0) || (strlen(msnti.album) > 0)))
    {
      msnti.status = PLAYER_STATUS_PLAYING;
    }
  else
    {
      msnti.status = PLAYER_STATUS_STOPPED;
    }

  //
  // Some players have artist and title the other way around
  // (As "{0} - {1}","artist","title" and "{1} - {0}","title","artist" are equivalent,
  //  but which field is actually artist and title isn't described by the message)
  //
  // From testing, the order we expect seems to be the popular consensus, but provide
  // a configuration option to override this as I've got this wrong at least once...
  //
  gboolean swap = purple_prefs_get_bool(PREF_MSN_SWAP_ARTIST_TITLE);

  if (strcmp("ZUNE", player) == 0)
    swap = !swap;

  if (swap)
  {
    char swap[STRLEN];
    strncpy(swap, msnti.artist, STRLEN);
    swap[STRLEN-1] = 0;
    strncpy(msnti.artist, msnti.track, STRLEN);
    msnti.artist[STRLEN-1] = 0;
    strncpy(msnti.track, swap, STRLEN);
    msnti.track[STRLEN-1] = 0;

    trace("swapping order to artist '%s' and title '%s'", msnti.artist, msnti.track);
  }

  //
  // Usually we don't find out the actual player name, so report it as unknown...
  //
  if ((msnti.player == 0) || (strlen(msnti.player) == 0))
    {
      msnti.player = "Unknown";
    }

  free(s);
}

static
LRESULT CALLBACK MSNWinProc(
                            HWND hwnd,      // handle to window
                            UINT uMsg,      // message identifier
                            WPARAM wParam,  // first message parameter
                            LPARAM lParam   // second message parameter
                            )
{
  switch(uMsg) {
  case WM_COPYDATA:
    {
      wchar_t MSNTitle[2048];
      COPYDATASTRUCT *cds = (COPYDATASTRUCT *) lParam;
      CopyMemory(MSNTitle,cds->lpData,cds->cbData);
      MSNTitle[2047]=0;
      process_message(MSNTitle);
      return TRUE;
    }
  default:
    return DefWindowProc(hwnd,uMsg,wParam,lParam);
  }
}

void
get_msn_compat_info(struct TrackInfo *ti)
{
  static HWND MSNWindow = 0;

  if (!MSNWindow)
    {
      memset(&msnti, 0, sizeof(struct TrackInfo));

      WNDCLASSEX MSNClass = {sizeof(WNDCLASSEX),0,MSNWinProc,0,0,GetModuleHandle(NULL),NULL,NULL,NULL,NULL,"MsnMsgrUIManager",NULL};
      ATOM a = RegisterClassEx(&MSNClass);
      trace("RegisterClassEx returned 0x%x",a);

      MSNWindow = CreateWindowEx(0,"MsnMsgrUIManager","",
                                      0,0,0,0,0,
                                      HWND_MESSAGE,NULL,GetModuleHandle(NULL),NULL);
      trace("CreateWindowEx returned 0x%x", MSNWindow);

      // Alternatively could use setWindowLong() to override WndProc for this window ?
    }

  // did we receive a message with something useful in it?
  if (msnti.status == PLAYER_STATUS_PLAYING)
    {
      *ti = msnti;
    }
}

static
void cb_toggled(GtkToggleButton *button, gpointer data)
{
  gboolean state = gtk_toggle_button_get_active(button);
  purple_prefs_set_bool(data, state);
}

void get_msn_compat_pref(GtkBox *vbox)
{
  GtkWidget *widget, *hbox;

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  widget = gtk_check_button_new_with_label(_("Swap artist and title"));
  gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), purple_prefs_get_bool(PREF_MSN_SWAP_ARTIST_TITLE));
  g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(cb_toggled), (gpointer) PREF_MSN_SWAP_ARTIST_TITLE);
}

// XXX: cleanup needed on musictracker unload?
// XXX: we've also heard tell that HKEY_CURRENT_USER\Software\Microsoft\MediaPlayer\CurrentMetadata has been used to pass this data....
