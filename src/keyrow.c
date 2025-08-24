#include "keyrow.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "glib.h"
#include "window.h"
#include "managementdialog.h"
#include "config.h"

#include <gpgme.h>
#include "cryptography.h"
#include "threading.h"

/**
 * This structure handles data of a key row.
 */
struct _LockKeyRow {
    AdwActionRow parent;

    LockManagementDialog *dialog;

    gboolean remove_success;
    GtkButton *remove_button;

    gboolean export_success;
    GtkButton *export_button;
    GFile *export_file;

    GtkButton *expire_button;
    GtkPopover *expire_popover;
    GtkCalendar *expire_calendar;
};

G_DEFINE_TYPE(LockKeyRow, lock_key_row, ADW_TYPE_ACTION_ROW);

void lock_key_row_copy_fingerprint(AdwActionRow * self, LockKeyRow * row);

/* Export */
static void lock_key_row_export_file_present(GtkButton * self, LockKeyRow * row);
static void lock_key_row_remove_confirm(GtkButton * self, LockKeyRow * row);

gboolean lock_key_row_export_on_completed(LockKeyRow * row);
gboolean lock_key_row_remove_on_completed(LockKeyRow * row);

/* Expire */
void lock_key_row_edit_expire(GtkButton * self, LockKeyRow * row);
void lock_key_row_edit_expire_finish(GtkCalendar * self, LockKeyRow * row);

/**
 * This function initializes a LockKeyRow.
 *
 * @param row Row to be initialized
 */
static void lock_key_row_init(LockKeyRow *row)
{
    gtk_widget_init_template(GTK_WIDGET(row));

    g_signal_connect(row, "activated", G_CALLBACK(lock_key_row_copy_fingerprint), row);

    g_signal_connect(row->remove_button, "clicked", G_CALLBACK(lock_key_row_remove_confirm), row);

    g_signal_connect(row->export_button, "clicked", G_CALLBACK(lock_key_row_export_file_present), row);

    g_signal_connect(row->expire_button, "clicked", G_CALLBACK(lock_key_row_edit_expire), row);
    g_signal_connect(row->expire_calendar, "day-selected", G_CALLBACK(lock_key_row_edit_expire_finish), row);
}

/**
 * This function initializes a LockKeyRow class.
 *
 * @param class Row class to be initialized
 */
static void lock_key_row_class_init(LockKeyRowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), UI_RESOURCE("keyrow.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyRow, remove_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyRow, export_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyRow, expire_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyRow, expire_popover);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyRow, expire_calendar);
}

/**
 * This function creates a new LockKeyRow.
 *
 * @param dialog Dialog in which the row is presented
 * @param key Key related to the row
 *
 * @return LockKeyRow
 */
LockKeyRow *lock_key_row_new(LockManagementDialog *dialog, gpgme_key_t key)
{
    gchar *tooltip_text;

    if (key->subkeys->expires == 0) {
        tooltip_text = g_strdup(_("Key does not expire"));
    } else {
        gint64 expire_timestamp = (gint64) key->subkeys->expires;
        GDateTime *expire = g_date_time_new_from_unix_local(expire_timestamp);

        tooltip_text = g_date_time_format(expire, (key->expired) ? C_("Expired key tooltip (see GLib.DateTime.format / https://docs.gtk.org/glib/method.DateTime.format.html#description for date formatting)", "Expired on %B %e, %Y at %R") : C_("Expired key tooltip (see GLib.DateTime.format / https://docs.gtk.org/glib/method.DateTime.format.html#description for date formatting)", "Expires on %B %e, %Y at %R"));
    }

    /* Row */
    LockKeyRow *row = g_object_new(LOCK_TYPE_KEY_ROW, "title", key->uids->uid, "subtitle",
                                   key->fpr,
                                   "tooltip-text", tooltip_text, NULL);

    gtk_widget_set_visible(GTK_WIDGET(row->export_button), !key->expired);
    gtk_widget_set_visible(GTK_WIDGET(row->expire_button), key->expired);

    /* TODO: implement g_object_class_install_property() */
    row->dialog = dialog;
    //memcpy(row->key, key, sizeof(&key));

    return row;
}

/**
 * This function copies the fingerprint of the key of a LockKeyRow.
 *
 * @param self https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/signal.ActionRow.activated.html
 * @param row https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/signal.ActionRow.activated.html
 */
void lock_key_row_copy_fingerprint(AdwActionRow *self, LockKeyRow *row)
{
    (void)self;

    GdkClipboard *active_clipboard = gdk_display_get_clipboard(gdk_display_get_default());

    AdwToast *toast = adw_toast_new(_("Fingerprint copied"));
    adw_toast_set_timeout(toast, 2);

    const gchar *fingerprint = adw_action_row_get_subtitle(ADW_ACTION_ROW(row));
    gdk_clipboard_set_text(active_clipboard, fingerprint);

    lock_management_dialog_add_toast(row->dialog, toast);
}

/**** Export ****/

/**
 * This function opens the export file of a LockKeyRow.
 *
 * @param object https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param result https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param user_data https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 */
static void lock_key_row_export_file_save(GObject *source_object, GAsyncResult *res, gpointer data)
{
    GtkFileDialog *file = GTK_FILE_DIALOG(source_object);
    LockKeyRow *row = LOCK_KEY_ROW(data);

    row->export_file = gtk_file_dialog_save_finish(file, res, NULL);
    if (row->export_file == NULL) {
        row = NULL;
        goto cleanup;
    }

    thread_export_key(row);

 cleanup:
    g_object_unref(file);
    file = NULL;
}

/**
 * This function opens a save file dialog for a LockKeyRow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param row https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_key_row_export_file_present(GtkButton *self, LockKeyRow *row)
{
    (void)self;

    GtkFileDialog *file = gtk_file_dialog_new();
    LockWindow *window = lock_management_dialog_get_window(row->dialog);
    GCancellable *cancel = g_cancellable_new();

    gtk_file_dialog_save(file, GTK_WINDOW(window), cancel, lock_key_row_export_file_save, row);
}

/**
 * This function imports a key in a LockManagementDialog.
 *
 * @param row https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_key_row_export(LockKeyRow *row)
{
    char *path = g_file_get_path(row->export_file);
    const char *fingerprint = adw_action_row_get_subtitle(ADW_ACTION_ROW(row));

    row->export_success = key_export(path, fingerprint);

    /* Cleanup */
    g_free(path);
    path = NULL;

    /* UI */
    g_idle_add((GSourceFunc) lock_key_row_export_on_completed, row);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for key exports and is supposed to be called via g_idle_add().
 *
 * @param row https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_key_row_export_on_completed(LockKeyRow *row)
{
    AdwToast *toast;

    if (!row->export_success) {
        toast = adw_toast_new(_("Export failed"));
    } else {
        toast = adw_toast_new(_("Key exported"));
    }

    adw_toast_set_timeout(toast, 2);
    lock_management_dialog_add_toast(row->dialog, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**** Remove ****/

/**
 * This function handles user input of the confirm dialog for key removals.
 *
 * @param self https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/signal.AlertDialog.response.html
 * @param response https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/signal.AlertDialog.response.html
 * @param row https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/signal.AlertDialog.response.html
 */
static void lock_key_row_remove_confirm_on_responded(AdwAlertDialog *self, gchar *response, LockKeyRow *row)
{
    (void)self;

    if (strcmp(response, "remove") != 0)
        return;

    thread_remove_key(row);
}

/**
 * This function confirms whether a key should be removed in a LockManagementDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param row https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_key_row_remove_confirm(GtkButton *self, LockKeyRow *row)
{
    (void)self;

    LockWindow *window = lock_management_dialog_get_window(row->dialog);
    const char *uid = adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row));

    AdwAlertDialog *confirm = ADW_ALERT_DIALOG(adw_alert_dialog_new(_("Remove key?"), NULL));
    adw_alert_dialog_format_body(confirm, _("The removal of the key of %s cannot be undone!"), uid);

    adw_alert_dialog_add_responses(confirm, "cancel", _("_Cancel"), "remove", _("_Remove"), NULL);
    adw_alert_dialog_set_response_appearance(confirm, "remove", ADW_RESPONSE_DESTRUCTIVE);

    adw_alert_dialog_set_default_response(confirm, "cancel");
    adw_alert_dialog_set_close_response(confirm, "cancel");
    g_signal_connect(confirm, "response", G_CALLBACK(lock_key_row_remove_confirm_on_responded), row);

    adw_dialog_present(ADW_DIALOG(confirm), GTK_WIDGET(window));
}

/**
 * This function removes a key and its subkeys from the keyring in a LockKeyRow.
 *
 * @param row https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_key_row_remove(LockKeyRow *row)
{
    const char *fingerprint = adw_action_row_get_subtitle(ADW_ACTION_ROW(row));

    row->remove_success = key_remove(fingerprint);

    /* UI */
    g_idle_add((GSourceFunc) lock_key_row_remove_on_completed, row);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for key removals and is supposed to be called via g_idle_add().
 *
 * @param row https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_key_row_remove_on_completed(LockKeyRow *row)
{
    AdwToast *toast;

    if (!row->remove_success) {
        toast = adw_toast_new(_("Removal failed"));
    } else {
        toast = adw_toast_new(_("Key removed"));
    }

    lock_management_dialog_refresh(NULL, row->dialog);

    adw_toast_set_timeout(toast, 2);
    lock_management_dialog_add_toast(row->dialog, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**** Expire ****/

/**
 * This function lets the user update the expire time of a key.
 *
 * @param self Gtk.Button::clicked
 * @param row Gtk.Button::clicked
 */
void lock_key_row_edit_expire(GtkButton *self, LockKeyRow *row)
{
    (void)self;

    gtk_popover_popup(row->expire_popover);
}

/**
 * This function applies the user-selected expire time to a key.
 *
 * @param self Gtk.Calendar::day-selected
 * @param row Gtk.Calendar::day-selected
 */
void lock_key_row_edit_expire_finish(GtkCalendar *self, LockKeyRow *row)
{
    AdwToast *toast;

    const char *fingerprint = adw_action_row_get_subtitle(ADW_ACTION_ROW(row));

    GDateTime *expire = gtk_calendar_get_date(self);
    GDateTime *now = g_date_time_new_now_local();
    const gint64 expire_offset = g_date_time_to_unix(expire) - g_date_time_to_unix(now);

    gtk_popover_popdown(row->expire_popover);

    gboolean expire_success = (expire_offset >= 0) ? key_expire(fingerprint, expire_offset) : false;

    if (!expire_success) {
        toast = adw_toast_new(_("Renewal failed"));
    } else {
        toast = adw_toast_new(_("Expire date renewed"));

        lock_management_dialog_refresh(NULL, row->dialog);
    }

    adw_toast_set_timeout(toast, 2);
    lock_management_dialog_add_toast(row->dialog, toast);

    /* Cleanup */
    g_date_time_unref(expire);
    g_date_time_unref(now);
}
