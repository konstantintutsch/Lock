#include "managementdialog.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "keyrow.h"
#include "config.h"

#include <gpgme.h>
#include <time.h>
#include "cryptography.h"
#include "threading.h"

/**
 * This structure handles data of a key dialog.
 */
struct _LockManagementDialog {
    AdwDialog parent;

    LockWindow *window;

    AdwToastOverlay *toast_overlay;

    AdwSpinner *spinner;
    AdwNavigationView *view;

    GtkButton *refresh_button;
    GtkBox *manage_box;

    AdwStatusPage *status_page;
    GtkListBox *key_box;

    gboolean import_success;
    GtkButton *import_button;
    GListModel *import_file;

    gboolean generate_success;
    GtkButton *generate_button;
    AdwEntryRow *name_entry;
    AdwEntryRow *email_entry;
    AdwEntryRow *comment_entry;
    AdwComboRow *sign_entry;
    AdwComboRow *encrypt_entry;
    gboolean generate_expire;
    AdwActionRow *expire_entry;
    GtkButton *expire_reset_button;
    GtkButton *expire_button;
    GtkPopover *expire_popover;
    GtkCalendar *expire_calendar;
};

G_DEFINE_TYPE(LockManagementDialog, lock_management_dialog, ADW_TYPE_DIALOG);

/* UI */
gboolean lock_management_dialog_import_on_completed(LockManagementDialog * dialog);
gboolean lock_management_dialog_generate_on_completed(LockManagementDialog * dialog);

/* Import */
static void lock_management_dialog_import_file_present(GtkButton * self, LockManagementDialog * dialog);

/* Expire */
void lock_management_dialog_select_expire(GtkButton * self, LockManagementDialog * dialog);
void lock_management_dialog_select_expire_finish(GtkCalendar * self, LockManagementDialog * dialog);
void lock_management_dialog_reset_expire(GtkButton * self, LockManagementDialog * dialog);

/**
 * This function initializes a LockManagementDialog.
 *
 * @param dialog Dialog to be initialized
 */
static void lock_management_dialog_init(LockManagementDialog *dialog)
{
    gtk_widget_init_template(GTK_WIDGET(dialog));

    g_signal_connect(dialog->refresh_button, "clicked", G_CALLBACK(lock_management_dialog_refresh), dialog);
    lock_management_dialog_refresh(NULL, dialog);

    g_signal_connect(dialog->import_button, "clicked", G_CALLBACK(lock_management_dialog_import_file_present), dialog);

    lock_management_dialog_reset_expire(NULL, dialog);
    g_signal_connect(dialog->expire_reset_button, "clicked", G_CALLBACK(lock_management_dialog_reset_expire), dialog);
    g_signal_connect(dialog->expire_button, "clicked", G_CALLBACK(lock_management_dialog_select_expire), dialog);
    g_signal_connect(dialog->expire_calendar, "day-selected", G_CALLBACK(lock_management_dialog_select_expire_finish), dialog);
    g_signal_connect(dialog->generate_button, "clicked", G_CALLBACK(thread_generate_key), dialog);
}

/**
 * This function initializes a LockManagementDialog class.
 *
 * @param class Dialog class to be initialized
 */
static void lock_management_dialog_class_init(LockManagementDialogClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), UI_RESOURCE("managementdialog.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, toast_overlay);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, spinner);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, view);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, refresh_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, manage_box);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, status_page);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, key_box);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, import_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, generate_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, name_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, email_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, comment_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, sign_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, encrypt_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, expire_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, expire_reset_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, expire_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, expire_popover);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockManagementDialog, expire_calendar);
}

/**
 * This function creates a new LockManagementDialog.
 *
 * @param window Window in which the dialog is presented
 *
 * @return LockManagementDialog
 */
LockManagementDialog *lock_management_dialog_new(LockWindow *window)
{
    LockManagementDialog *dialog = g_object_new(LOCK_TYPE_MANAGEMENT_DIALOG, NULL);

    /* TODO: implement g_object_class_install_property() */
    dialog->window = window;

    return dialog;
}

/**** UI ****/

/**
 * This function updates the UI of a LockManagementDialog to indicate whether something is currently processing or loading.
 *
 * @param dialog Dialog to update the UI of
 * @param spinning Whether something is happening
 */
void lock_management_dialog_show_spinner(LockManagementDialog *dialog, gboolean spinning)
{
    gtk_widget_set_visible(GTK_WIDGET(dialog->spinner), spinning);
    gtk_widget_set_visible(GTK_WIDGET(dialog->view), !spinning);
}

/**
 * This function refreshes the key list of a LockManagementDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void lock_management_dialog_refresh(GtkButton *self, LockManagementDialog *dialog)
{
    (void)self;

    lock_management_dialog_show_spinner(dialog, true);

    gtk_list_box_remove_all(dialog->key_box);

    gpgme_ctx_t context;
    gpgme_key_t key;
    gpgme_error_t error;

    error = gpgme_new(&context);
    if (error)
        return;

    error = gpgme_op_keylist_start(context, NULL, 0);
    while (!error) {
        error = gpgme_op_keylist_next(context, &key);

        if (error)
            break;

        gtk_list_box_append(dialog->key_box, GTK_WIDGET(lock_key_row_new(dialog, key)));
    }

    /* Cleanup */
    gpgme_release(context);
    gpgme_key_release(key);

    if (gtk_list_box_get_row_at_index(dialog->key_box, 0) == NULL) {
        gtk_widget_set_visible(GTK_WIDGET(dialog->key_box), false);
        gtk_box_set_spacing(dialog->manage_box, 0);

        gtk_widget_set_visible(GTK_WIDGET(dialog->status_page), true);
    } else {
        gtk_widget_set_visible(GTK_WIDGET(dialog->key_box), true);
        gtk_box_set_spacing(dialog->manage_box, 20);

        gtk_widget_set_visible(GTK_WIDGET(dialog->status_page), false);
    }

    lock_management_dialog_show_spinner(dialog, false);
}

/**
 * This functions returns the window of a LockManagementDialog.
 *
 * @param dialog Dialog to get the window of
 *
 * @return LockWindow
 */
LockWindow *lock_management_dialog_get_window(LockManagementDialog *dialog)
{
    return dialog->window;
}

/**
 * This function adds a toast the toast_overlay of a LockManagementDialog.
 *
 * @param dialog Dialog to add the toast to
 * @param toast Toast to add to the dialog
 */
void lock_management_dialog_add_toast(LockManagementDialog *dialog, AdwToast *toast)
{
    adw_toast_overlay_add_toast(dialog->toast_overlay, toast);
}

/**** Import ****/

/**
 * This function opens the import file of a LockManagementDialog.
 *
 * @param object https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param result https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param user_data https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 */
static void lock_management_dialog_import_file_open(GObject *source_object, GAsyncResult *res, gpointer data)
{
    GtkFileDialog *file = GTK_FILE_DIALOG(source_object);
    LockManagementDialog *dialog = LOCK_MANAGEMENT_DIALOG(data);

    dialog->import_file = gtk_file_dialog_open_multiple_finish(file, res, NULL);
    if (dialog->import_file == NULL) {
        dialog = NULL;
        goto cleanup;
    }

    thread_import_key(dialog);

 cleanup:
    g_object_unref(file);
    file = NULL;
}

/**
 * This function opens an open file dialog for a LockManagementDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_management_dialog_import_file_present(GtkButton *self, LockManagementDialog *dialog)
{
    (void)self;

    GtkFileDialog *file = gtk_file_dialog_new();
    GCancellable *cancel = g_cancellable_new();

    gtk_file_dialog_open_multiple(file, GTK_WINDOW(dialog->window), cancel, lock_management_dialog_import_file_open, dialog);
}

/**
 * This function imports a key in a LockManagementDialog.
 *
 * @param dialog https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_management_dialog_import(LockManagementDialog *dialog)
{
    char *path = malloc(1 * sizeof(char));

    gboolean thread_success;
    for (guint i = 0; i < g_list_model_get_n_items(dialog->import_file); i++) {
        path = g_file_get_path(g_list_model_get_item(dialog->import_file, i));

        thread_success = key_import(path);

        if (!thread_success)
            break;
    }
    dialog->import_success = thread_success;

    /* Cleanup */
    g_free(path);
    path = NULL;

    /* UI */
    g_idle_add((GSourceFunc) lock_management_dialog_import_on_completed, dialog);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for key imports and is supposed to be called via g_idle_add().
 *
 * @param dialog https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_management_dialog_import_on_completed(LockManagementDialog *dialog)
{
    AdwToast *toast;

    if (!dialog->import_success) {
        toast = adw_toast_new(_("Import failed"));
    } else {
        toast = adw_toast_new(_("Keys imported"));
    }

    adw_toast_set_timeout(toast, 2);

    lock_management_dialog_show_spinner(dialog, false);
    adw_toast_overlay_add_toast(dialog->toast_overlay, toast);

    lock_management_dialog_refresh(NULL, dialog);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**** Generation ****/

/**
 * This function lets the user select the expire date for key pair generation.
 *
 * @param self Gtk.Button::clicked
 * @param row Gtk.Button::clicked
 */
void lock_management_dialog_select_expire(GtkButton *self, LockManagementDialog *dialog)
{
    (void)self;

    gtk_popover_popup(dialog->expire_popover);
}

/**
 * This function handles expire date selection for key pair generations.
 *
 * @param self Gtk.Calendar::day-selected
 * @param row Gtk.Calendar::day-selected
 */
void lock_management_dialog_select_expire_finish(GtkCalendar *self, LockManagementDialog *dialog)
{
    GDateTime *expire = gtk_calendar_get_date(self);

    dialog->generate_expire = true;
    adw_action_row_set_subtitle(dialog->expire_entry, g_date_time_format(expire, C_("Expire date format for key pair generation (see GLib.DateTime.format / https://docs.gtk.org/glib/method.DateTime.format.html#description)", "%B %e, %Y")));

    /* Cleanup */
    g_date_time_unref(expire);
}

/**
 * This function resets the expire date for key pair generations.
 *
 * @param self Gtk.Button::clicked
 * @param row Gtk.Button::clicked
 */
void lock_management_dialog_reset_expire(GtkButton *self, LockManagementDialog *dialog)
{
    (void)self;

    adw_action_row_set_subtitle(dialog->expire_entry, _("No expire date"));
    dialog->generate_expire = false;
}

/**
 * This function checks if sufficient information for a successful key generation has been entered.
 *
 * @param dialog Dialog to check the generation information of
 *
 * @return Success
 */
bool lock_management_dialog_generate_ready(LockManagementDialog *dialog)
{
    if (strlen(gtk_editable_get_text(GTK_EDITABLE(dialog->name_entry))) == 0) {
        adw_entry_row_grab_focus_without_selecting(dialog->name_entry);
        return false;
    }

    return true;
}

/**
 * This function generates a new key pair in a LockManagementDialog.
 *
 * @param dialog Dialog to generate the key pair in and from
 */
void lock_management_dialog_generate(LockManagementDialog *dialog)
{
    const gchar *name = gtk_editable_get_text(GTK_EDITABLE(dialog->name_entry));
    const gchar *email = gtk_editable_get_text(GTK_EDITABLE(dialog->email_entry));
    const gchar *comment = gtk_editable_get_text(GTK_EDITABLE(dialog->comment_entry));

    // Name (Comment) <Email>
    gchar *userid = g_strdup_printf("%s", name);
    if (strlen(comment) > 0)
        userid = g_strdup_printf("%s (%s)", userid, comment);
    if (strlen(email) > 0)
        userid = g_strdup_printf("%s <%s>", userid, email);

    gint sign_selected = adw_combo_row_get_selected(dialog->sign_entry);
    GtkStringList *sign_list = GTK_STRING_LIST(adw_combo_row_get_model(dialog->sign_entry));
    const gchar *sign_algorithm = gtk_string_list_get_string(sign_list, sign_selected);

    gint encrypt_selected = adw_combo_row_get_selected(dialog->encrypt_entry);
    GtkStringList *encrypt_list = GTK_STRING_LIST(adw_combo_row_get_model(dialog->encrypt_entry));
    const gchar *encrypt_algorithm = gtk_string_list_get_string(encrypt_list, encrypt_selected);

    GDateTime *expire = gtk_calendar_get_date(dialog->expire_calendar);
    GDateTime *now = g_date_time_new_now_local();
    const gint64 expire_offset = (dialog->generate_expire) ? g_date_time_to_unix(expire) - g_date_time_to_unix(now) : 0;

    dialog->generate_success = key_generate(userid, sign_algorithm, encrypt_algorithm, expire_offset);

    /* Cleanup */
    g_free(userid);
    userid = NULL;

    g_date_time_unref(expire);
    g_date_time_unref(now);

    /* UI */
    g_idle_add((GSourceFunc) lock_management_dialog_generate_on_completed, dialog);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for key pair generation and is supposed to be called via g_idle_add().
 *
 * @param dialog https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_management_dialog_generate_on_completed(LockManagementDialog *dialog)
{
    AdwToast *toast;

    if (!dialog->generate_success) {
        toast = adw_toast_new(_("Generation failed"));
    } else {
        toast = adw_toast_new(_("Key pair generated"));

        // Reset input rows
        gtk_editable_set_text(GTK_EDITABLE(dialog->name_entry), "");
        gtk_editable_set_text(GTK_EDITABLE(dialog->email_entry), "");
        gtk_editable_set_text(GTK_EDITABLE(dialog->comment_entry), "");
        adw_combo_row_set_selected(dialog->sign_entry, 0);
        adw_combo_row_set_selected(dialog->encrypt_entry, 0);
        lock_management_dialog_reset_expire(NULL, dialog);
    }

    adw_toast_set_timeout(toast, 2);

    lock_management_dialog_show_spinner(dialog, false);
    adw_toast_overlay_add_toast(dialog->toast_overlay, toast);

    lock_management_dialog_refresh(NULL, dialog);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}
