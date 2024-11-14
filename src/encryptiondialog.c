#include "encryptiondialog.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "config.h"

/**
 * This structure handles data of a window.
 */
struct _LockEncryptionDialog {
    AdwDialog parent;

    GtkLabel *title_label;

    GtkEntry *entry;
    GtkButton *confirm_button;
};

G_DEFINE_TYPE(LockEncryptionDialog, lock_encryption_dialog, ADW_TYPE_DIALOG);

static void lock_encryption_dialog_set_title(LockEncryptionDialog * dialog,
                                             const gchar * title);
static void lock_encryption_dialog_set_placeholder_text(LockEncryptionDialog *
                                                        dialog,
                                                        const gchar *
                                                        placeholder_text);
static void lock_encryption_dialog_set_input_purpose(LockEncryptionDialog *
                                                     dialog,
                                                     GtkInputPurpose purpose);

static void lock_encryption_dialog_entry_confirm(GtkButton * self,
                                                 LockEncryptionDialog * dialog);

/**
 * This function initializes a LockEncryptionDialog.
 *
 * @param dialog Dialog to be initialized
 */
static void lock_encryption_dialog_init(LockEncryptionDialog *dialog)
{
    gtk_widget_init_template(GTK_WIDGET(dialog));

    g_signal_connect(dialog->confirm_button, "clicked",
                     G_CALLBACK(lock_encryption_dialog_entry_confirm), dialog);
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
                                         LockEncryptionDialog, title_label);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEncryptionDialog, entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockEncryptionDialog, confirm_button);

    g_signal_new("entered", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, 0,
                 NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1,
                 G_TYPE_STRING);
}

/**
 * This function creates a new LockEncryptionDialog.
 *
 * @param title Title of the dialog
 * @param placeholder_text Placeholder text in the entry of the dialog
 *
 * @return LockEncryptionDialog
 */
LockEncryptionDialog *lock_encryption_dialog_new(const gchar *title,
                                                 const gchar *placeholder_text,
                                                 GtkInputPurpose input_purpose)
{
    LockEncryptionDialog *dialog =
        g_object_new(LOCK_TYPE_ENCRYPTION_DIALOG, NULL);

    /* TODO: implement g_object_class_install_property() */
    lock_encryption_dialog_set_title(dialog, title);
    lock_encryption_dialog_set_placeholder_text(dialog, placeholder_text);
    lock_encryption_dialog_set_input_purpose(dialog, input_purpose);

    return dialog;
}

/**
 * This function sets the title of a LockEncryptionDialog.
 *
 * @param dialog Dialog to update the title of
 * @param title Text to use as the title
 */
static void lock_encryption_dialog_set_title(LockEncryptionDialog *dialog,
                                             const gchar *title)
{
    adw_dialog_set_title(ADW_DIALOG(dialog), title);
    gtk_label_set_label(dialog->title_label, title);
}

/**
 * This function sets the placeholder text of the entry of a LockEncryptionDialog.
 *
 * @param dialog Dialog to update the placeholder text of
 * @param placeholder_text Text to use as placeholder text of the entry
 */
static void lock_encryption_dialog_set_placeholder_text(LockEncryptionDialog
                                                        *dialog,
                                                        const gchar
                                                        *placeholder_text)
{
    gtk_entry_set_placeholder_text(dialog->entry, placeholder_text);
}

/**
 * This functions sets the input purpose of the entry of a LockEncryptionDialog.
 *
 * @param self Dialog to update the input purpose of
 * @param purpose Input purpose
 */
static void lock_encryption_dialog_set_input_purpose(LockEncryptionDialog
                                                     *dialog,
                                                     GtkInputPurpose purpose)
{
    gtk_entry_set_input_purpose(dialog->entry, purpose);
}

/**
 * This function gets text from the entry of a LockEncryptionDialog.
 *
 * @param dialog Dialog to get the text from
 *
 * @return Text
 */
const gchar *lock_encryption_dialog_get_text(LockEncryptionDialog *dialog)
{
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(dialog->entry);

    return gtk_entry_buffer_get_text(buffer);
}

/**
 * This function handles input confirmation of a LockEncryptionDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_encryption_dialog_entry_confirm(GtkButton *self,
                                                 LockEncryptionDialog *dialog)
{
    (void)self;

    const gchar *text = lock_encryption_dialog_get_text(dialog);

    if (!(strlen(text) > 0) || text == NULL)
        return;

    g_signal_emit_by_name(dialog, "entered", text);

    adw_dialog_close(ADW_DIALOG(dialog));
}
