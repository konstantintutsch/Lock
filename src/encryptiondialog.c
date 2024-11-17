#include "encryptiondialog.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "config.h"

#include <gpgme.h>

/**
 * This structure handles data of a window.
 */
struct _LockEncryptionDialog {
    AdwDialog parent;

    AdwSpinner *spinner;
    AdwToolbarView *view;

    GtkButton *refresh_button;
    GtkBox *manage_box;

    AdwStatusPage *status_page;
    GtkListBox *key_box;
};

G_DEFINE_TYPE(LockEncryptionDialog, lock_encryption_dialog, ADW_TYPE_DIALOG);

void lock_encryption_dialog_refresh(GtkButton * self,
                                    LockEncryptionDialog * dialog);

void lock_encryption_dialog_key_selected(GtkButton * self,
                                         GtkListBoxRow * row,
                                         LockEncryptionDialog * dialog);

/**
 * This function initializes a LockEncryptionDialog.
 *
 * @param dialog Dialog to be initialized
 */
static void lock_encryption_dialog_init(LockEncryptionDialog *dialog)
{
    gtk_widget_init_template(GTK_WIDGET(dialog));

    g_signal_connect(dialog->refresh_button, "clicked",
                     G_CALLBACK(lock_encryption_dialog_refresh), dialog);
    lock_encryption_dialog_refresh(NULL, dialog);

    g_signal_connect(dialog->key_box, "row-activated",
                     G_CALLBACK(lock_encryption_dialog_key_selected), dialog);
}

/**
 * This function initializes a LockEncryptionDialog class.
 *
 * @param class Dialog class to be initialized
 */
static void lock_encryption_dialog_class_init(LockEncryptionDialogClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                                UI_RESOURCE
                                                ("encryptiondialog.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEncryptionDialog, spinner);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEncryptionDialog, view);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEncryptionDialog, refresh_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEncryptionDialog, manage_box);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEncryptionDialog, status_page);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEncryptionDialog, key_box);

    g_signal_new("entered", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, 0,
                 NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1,
                 G_TYPE_STRING);
}

/**
 * This function creates a new LockEncryptionDialog.
 *
 * @return LockEncryptionDialog
 */
LockEncryptionDialog *lock_encryption_dialog_new()
{
    return g_object_new(LOCK_TYPE_ENCRYPTION_DIALOG, NULL);
}

/**
 * This function updates the UI of a LockEncryptionDialog to indicate whether something is currently loading.
 *
 * @param dialog Dialog to update the UI of
 * @param spinning Whether something is happening
 */
void lock_encryption_dialog_show_spinner(LockEncryptionDialog *dialog,
                                         gboolean spinning)
{
    gtk_widget_set_visible(GTK_WIDGET(dialog->spinner), spinning);
    gtk_widget_set_visible(GTK_WIDGET(dialog->view), !spinning);
}

/**
 * This function refreshes the key list of a LockEncryptionDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void lock_encryption_dialog_refresh(GtkButton *self,
                                    LockEncryptionDialog *dialog)
{
    (void)self;

    lock_encryption_dialog_show_spinner(dialog, true);

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

        if (!key->can_encrypt)
            continue;

        AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
        adw_preferences_row_set_use_markup(ADW_PREFERENCES_ROW(row), false);
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), true);

        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), key->uids->uid);
        adw_action_row_set_subtitle(row, key->fpr);

        gtk_list_box_append(dialog->key_box, GTK_WIDGET(row));
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

    lock_encryption_dialog_show_spinner(dialog, false);
}

/**
 * This function handles key selection of a LockEncryptionDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.ListBox.row-activated.html
 * @param row https://docs.gtk.org/gtk4/signal.ListBox.row-activated.html
 * @param dialog https://docs.gtk.org/gtk4/signal.ListBox.row-activated.html
 */
void lock_encryption_dialog_key_selected(GtkButton *self,
                                         GtkListBoxRow *row,
                                         LockEncryptionDialog *dialog)
{
    (void)self;

    const gchar *fingerprint = adw_action_row_get_subtitle(ADW_ACTION_ROW(row));
    g_signal_emit_by_name(dialog, "entered", fingerprint);

    adw_dialog_close(ADW_DIALOG(dialog));
}
