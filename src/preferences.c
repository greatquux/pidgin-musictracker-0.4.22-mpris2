#include "musictracker.h"
#include "account.h"
#include "utils.h"
#include <string.h>

#ifndef WIN32
#include <config.h>
#else
#include <config-win32.h>
#endif

#include "gettext.h"
#define _(String) dgettext (PACKAGE, String)

GtkWidget *format_menu;
GtkWidget *format_entry;
GtkWidget *filter_list, *filter_mask;

#if !GTK_CHECK_VERSION(2,12,0)
#define gtk_widget_set_tooltip_text(widget, tip) \
  {                                              \
  GtkTooltips *tips = gtk_tooltips_new();        \
  gtk_tooltips_set_tip(tips, widget, tip, NULL); \
  }
#endif

static
void cb_player_changed(GtkWidget *widget, gpointer data)
{
	int active = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	if (active != -1) {
		active -= 1;
		purple_prefs_set_int(PREF_PLAYER, active);
	}

	// Enable/disable properties button depending on whether player has pref_func
	if (active == -1)
		gtk_widget_set_sensitive((GtkWidget*) data, FALSE);
	else
		gtk_widget_set_sensitive((GtkWidget*) data, g_players[active].pref_func != 0);
}

static
void cb_player_properties(GtkWidget *widget, gpointer data)
{
	int active = purple_prefs_get_int(PREF_PLAYER);
	if (active != -1 && g_players[active].pref_func) {
		GtkWidget *vbox, *align;
		GtkWidget *dialog = gtk_dialog_new_with_buttons(g_players[active].name, NULL,
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_CLOSE, GTK_RESPONSE_NONE, NULL);

		vbox = gtk_vbox_new(FALSE, 5);
		align = gtk_alignment_new(0, 0, 1, 1);
		gtk_alignment_set_padding(GTK_ALIGNMENT(align), 10, 10, 10, 10);
		gtk_container_add(GTK_CONTAINER(align), vbox);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), align);

		// Ask player pref_func to add controls to dialog
		(*g_players[active].pref_func)(GTK_BOX(vbox));

		gtk_widget_show_all(dialog);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

static
void cb_format_changed(GtkWidget *widget, gpointer data)
{
	const char *type = (const char*) data;
	purple_prefs_set_string(type, gtk_entry_get_text(GTK_ENTRY(widget)));
}

static
void cb_format_clicked(GtkWidget *widget, gpointer data)
{
	format_entry = (GtkWidget*) data;
	gtk_menu_popup(GTK_MENU(format_menu), NULL, NULL, NULL, NULL,
			0, gtk_get_current_event_time());
}

static
void cb_format_menu(GtkMenuItem *item, gpointer data)
{
	const char *text = gtk_entry_get_text(GTK_ENTRY(format_entry));
	char *buf = malloc(strlen(text) + strlen((const char*) data) + 2);
	if (strlen(text) == 0)
		strcpy(buf, (const char*) data);
	else
		sprintf(buf, "%s %s", text, (const char*) data);

	gtk_entry_set_text(GTK_ENTRY(format_entry), buf);
	free(buf);
}

static
void cb_custom_edited(GtkCellRendererText *renderer, char *path, char *str, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = (GtkTreeModel*) data;
	if (gtk_tree_model_get_iter_from_string(model, &iter, path)) {
                PurpleAccount *account;
                GValue value;
                memset(&value, 0, sizeof(value));
                gtk_tree_model_get_value(model, &iter, 5, &value);
                assert(G_VALUE_HOLDS_POINTER(&value));
                account = g_value_get_pointer(&value);
                g_value_unset(&value);

		char *pref = build_pref(PREF_CUSTOM_FORMAT, purple_account_get_username(account), purple_account_get_protocol_name(account));
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 2, str, -1);
		purple_prefs_set_string(pref, str);
                g_free(pref);
	}
}

static
void cb_broken_nowlistening_toggled(GtkCellRendererToggle *cell, char *path, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model = (GtkTreeModel*) data;
  if (gtk_tree_model_get_iter_from_string(model, &iter, path))
    {
      PurpleAccount *account;
      GValue value;
      memset(&value, 0, sizeof(value));
      gtk_tree_model_get_value(model, &iter, 5, &value);
      assert(G_VALUE_HOLDS_POINTER(&value));
      account = g_value_get_pointer(&value);
      g_value_unset(&value);

      gboolean flag;
      char * pref = build_pref(PREF_BROKEN_NOWLISTENING, purple_account_get_username(account), purple_account_get_protocol_name(account));
      memset(&value, 0, sizeof(value));
      gtk_tree_model_get_value(model, &iter, 4, &value);
      assert(G_VALUE_HOLDS_BOOLEAN(&value));
      flag = !g_value_get_boolean(&value);
      g_value_unset(&value);

      gtk_list_store_set(GTK_LIST_STORE(model), &iter, 4, flag, -1);
      purple_prefs_set_bool(pref, flag);
      g_free(pref);
    }
}

static
void cb_custom_toggled(GtkCellRendererToggle *cell, char *path, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = (GtkTreeModel*) data;
	if (gtk_tree_model_get_iter_from_string(model, &iter, path)) {
                PurpleAccount *account;
                GValue value;
                memset(&value, 0, sizeof(value));
                gtk_tree_model_get_value(model, &iter, 5, &value);
                assert(G_VALUE_HOLDS_POINTER(&value));
                account = g_value_get_pointer(&value);
                g_value_unset(&value);

		gboolean flag;
		char *pref = build_pref(PREF_CUSTOM_DISABLED, purple_account_get_username(account), purple_account_get_protocol_name(account));

		memset(&value, 0, sizeof(value));
		gtk_tree_model_get_value(model, &iter, 3, &value);
		assert(G_VALUE_HOLDS_BOOLEAN(&value));
		flag = !g_value_get_boolean(&value);
		g_value_unset(&value);

		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 3, flag, -1);
                // clear status before we update the preference
                set_status(account, 0);
		purple_prefs_set_bool(pref, flag);
                g_free(pref);
	}
}

static
void cb_filter_toggled(GtkToggleButton *button, gpointer data)
{
	gboolean state = gtk_toggle_button_get_active(button);
	purple_prefs_set_bool(PREF_FILTER_ENABLE, state);
	gtk_widget_set_sensitive(filter_list, state);
	gtk_widget_set_sensitive(filter_mask, state);
}

static
void cb_filter_changed(GtkWidget *widget, gpointer data)
{
	purple_prefs_set_string(PREF_FILTER, gtk_entry_get_text(GTK_ENTRY(widget)));
}

static
void cb_filter_mask_changed(GtkWidget *widget, gpointer data)
{
	const char *text = gtk_entry_get_text(GTK_ENTRY(widget));
	if (strlen(text) == 1)
		purple_prefs_set_string(PREF_MASK, gtk_entry_get_text(GTK_ENTRY(widget)));
}

static
void cb_misc_toggled(GtkToggleButton *button, gpointer data)
{
	gboolean state = gtk_toggle_button_get_active(button);
	purple_prefs_set_bool(data, state);
}

static void
append_format_menu(const char *name, const char *format)
{
  char *buf = g_strdup_printf("%s - %s", name, format);
  GtkWidget *widget = gtk_menu_item_new_with_label(buf);
  g_free(buf);
  gtk_menu_shell_append(GTK_MENU_SHELL(format_menu), widget);
  g_signal_connect(G_OBJECT(widget), "activate", G_CALLBACK(cb_format_menu), (gpointer) format);
}

#define ADD_FORMAT_ENTRY(vbox, name, type) \
	hbox = gtk_hbox_new(FALSE, 5); \
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0); \
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(name), FALSE, FALSE, 0); \
	widget = gtk_entry_new(); \
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0); \
	gtk_entry_set_text(GTK_ENTRY(widget), purple_prefs_get_string(type)); \
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(cb_format_changed), (gpointer) type); \
	gtk_widget_set_tooltip_text(widget, _("Leave blank for an unchanged status message")); \
	widget2 = gtk_button_new_from_stock(GTK_STOCK_ADD); \
	gtk_box_pack_start(GTK_BOX(hbox), widget2, FALSE, FALSE, 0); \
	g_signal_connect(G_OBJECT(widget2), "clicked", G_CALLBACK(cb_format_clicked), (gpointer) widget);

GtkWidget* pref_frame(PurplePlugin *plugin)
{
	GtkWidget *align, *frame, *align2, *vbox2;
	GtkWidget *vbox, *hbox;
	GtkWidget *widget, *widget2;
	GtkWidget *expand;
	GtkWidget *treeview;
	GtkListStore *liststore;
	GtkCellRenderer *renderer;
	int i;

	// Root container
	vbox = gtk_vbox_new(FALSE, 10);
	align = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align), 10, 10, 10, 10);
	gtk_container_add(GTK_CONTAINER(align), vbox);

	// Player selection
	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Player:")), FALSE, FALSE, 0);
	widget = gtk_combo_box_new_text();
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	widget2 = gtk_button_new_from_stock(GTK_STOCK_PROPERTIES);
	gtk_box_pack_start(GTK_BOX(hbox), widget2, FALSE, FALSE, 0);

	gtk_combo_box_append_text(GTK_COMBO_BOX(widget), _("Auto-detect"));
	for (i=0; *g_players[i].name; ++i)
		gtk_combo_box_append_text(GTK_COMBO_BOX(widget), g_players[i].name);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(cb_player_changed), (gpointer) widget2);
	g_signal_connect(G_OBJECT(widget2), "clicked", G_CALLBACK(cb_player_properties), 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(widget), purple_prefs_get_int(PREF_PLAYER) + 1);

	// Popup menu for format
	format_menu = gtk_menu_new();
	append_format_menu(_("Artist"), "%p");
	append_format_menu(_("Album"), "%a");
	append_format_menu(_("Title"), "%t");
	append_format_menu(_("Track Duration"), "%d");
	append_format_menu(_("Elapsed Track Time"), "%c");
	append_format_menu(_("Progress Bar"), "%b");
	append_format_menu(_("Player"), "%r");
	append_format_menu(_("Music Symbol (may not display on some networks)"), "%m");
	append_format_menu(_("Status Message"), "%s");
	gtk_widget_show_all(format_menu);

	// Format selection
	frame = gtk_frame_new(_("Status Format"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	align2 = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align2), 5, 5, 5, 5);
	gtk_container_add(GTK_CONTAINER(frame), align2);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(align2), vbox2);
	ADD_FORMAT_ENTRY(vbox2, _("Playing:"), PREF_FORMAT);
	ADD_FORMAT_ENTRY(vbox2, _("Paused:"), PREF_PAUSED);
	ADD_FORMAT_ENTRY(vbox2, _("Stopped:"), PREF_OFF);

	// Protocol-specific formats
	liststore = gtk_list_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_POINTER);
	GList *accounts = purple_accounts_get_all();
	while (accounts) {
		PurpleAccount *account = (PurpleAccount*) accounts->data;
		GtkTreeIter iter;
		const char *username = purple_account_get_username(account);
                const char *protocolname = purple_account_get_protocol_name(account);

		char *buf1 = build_pref(PREF_CUSTOM_FORMAT, username, protocolname);
		char *buf2 = build_pref(PREF_CUSTOM_DISABLED, username, protocolname);
		char *buf3 = build_pref(PREF_BROKEN_NOWLISTENING, username, protocolname);
		trace("%s %s", buf1, purple_prefs_get_string(buf1));

		gtk_list_store_append(liststore, &iter);
		gtk_list_store_set(liststore, &iter, 0, username, 1, purple_account_get_protocol_name(account),
				2, purple_prefs_get_string(buf1), 3, purple_prefs_get_bool(buf2),
				4, purple_prefs_get_bool(buf3), 5, account, -1);

                g_free(buf1);
                g_free(buf2);
                g_free(buf3);
                
		accounts = accounts->next;
	}
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(liststore));
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, _("Screen Name"),
			gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, _("Protocol"),
			gtk_cell_renderer_text_new(), "text", 1, NULL);

	renderer = gtk_cell_renderer_text_new();
	g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(cb_custom_edited), (gpointer) GTK_TREE_MODEL(liststore));
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, _("Playing Status Format"),
			renderer, "text", 2, NULL);

	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(cb_custom_toggled), (gpointer) GTK_TREE_MODEL(liststore));
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, _("Disable"),
			renderer, "active", 3, NULL);


	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(G_OBJECT(renderer), "toggled", G_CALLBACK(cb_broken_nowlistening_toggled), (gpointer) GTK_TREE_MODEL(liststore));
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1, _("Don't use 'now listening'"),
			renderer, "active", 4, NULL);


	expand = gtk_expander_new(_("Customize playing status format, or disable status changing altogether for specific accounts"));
	gtk_expander_set_spacing(GTK_EXPANDER(expand), 5);
	gtk_box_pack_start(GTK_BOX(vbox), expand, TRUE, TRUE, 0);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(expand), widget);
	gtk_container_add(GTK_CONTAINER(widget), treeview);

        // Misc settings
	frame = gtk_frame_new(_("Other settings"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	align2 = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align2), 5, 5, 5, 5);
	gtk_container_add(GTK_CONTAINER(frame), align2);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(align2), vbox2);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	widget = gtk_check_button_new_with_label(_("Don't change status message when away"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), purple_prefs_get_bool(PREF_DISABLE_WHEN_AWAY));
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(cb_misc_toggled), (gpointer) PREF_DISABLE_WHEN_AWAY);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	widget = gtk_check_button_new_with_label(_("Don't change status message if protocol has 'now listening'"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), purple_prefs_get_bool(PREF_NOW_LISTENING_ONLY));
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(cb_misc_toggled), (gpointer) PREF_NOW_LISTENING_ONLY);

	// Filter
	frame = gtk_frame_new(_("Status Filter"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	align2 = gtk_alignment_new(0, 0, 1, 1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(align2), 5, 5, 5, 5);
	gtk_container_add(GTK_CONTAINER(frame), align2);
	vbox2 = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(align2), vbox2);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	widget = gtk_check_button_new_with_label(_("Enable status filter"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), purple_prefs_get_bool(PREF_FILTER_ENABLE));
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(cb_filter_toggled), 0);

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Blacklist (comma-delimited):")), FALSE, FALSE, 0);
	widget = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(widget), purple_prefs_get_string(PREF_FILTER));
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(cb_filter_changed), 0);
	filter_list = widget;

	hbox = gtk_hbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(_("Mask character:")), FALSE, FALSE, 0);
	widget = gtk_entry_new_with_max_length(1);
	gtk_entry_set_text(GTK_ENTRY(widget), purple_prefs_get_string(PREF_MASK));
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(cb_filter_mask_changed), 0);
	filter_mask = widget;

	gboolean state = purple_prefs_get_bool(PREF_FILTER_ENABLE);
	gtk_widget_set_sensitive(filter_list, state);
	gtk_widget_set_sensitive(filter_mask, state);

	gtk_widget_show_all(align);
	return align;
}

