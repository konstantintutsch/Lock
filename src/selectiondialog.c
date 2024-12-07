#include "selectiondialog.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "config.h"

#include <gpgme.h>

/**
 * This structure handles data of a window.
 */
struct _LockSelectionDialog {
    AdwDialog parent;

    gboolean target; /**< true: encryption-capable keys; false: signing-capable keys */

    AdwSpinner *spinner;
    AdwToolbarView *view;

    GtkButton *refresh_button;
    GtkBox *manage_box;

    AdwStatusPage *status_page;
    GtkListBox *key_box;
};

G_DEFINE_TYPE(LockSelectionDialog, lock_selection_dialog, ADW_TYPE_DIALOG);

void lock_selection_dialog_refresh(GtkButton * self,
                                   LockSelectionDialog * dialog);

void lock_selection_dialog_key_selected(GtkButton * self,
                                        GtkListBoxRow * row,
                                        LockSelectionDialog * dialog);

/**
 * This function initializes a LockSelectionDialog.
 *
 * @param dialog Dialog to be initialized
 */
static void lock_selection_dialog_init(LockSelectionDialog *dialog)
{
    gtk_widget_init_template(GTK_WIDGET(dialog));

    g_signal_connect(dialog->refresh_button, "clicked",
                     G_CALLBACK(lock_selection_dialog_refresh), dialog);

    g_signal_connect(dialog->key_box, "row-activated",
                     G_CALLBACK(lock_selection_dialog_key_selected), dialog);
}

/**
 * This function initializes a LockSelectionDialog class.
 *
 * @param class Dialog class to be initialized
 */
static void lock_selection_dialog_class_init(LockSelectionDialogClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                                UI_RESOURCE
                                                ("selectiondialog.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockSelectionDialog, spinner);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockSelectionDialog, view);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockSelectionDialog, refresh_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockSelectionDialog, manage_box);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockSelectionDialog, status_page);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class),
                                         LockSelectionDialog, key_box);

    g_signal_new("entered", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_FIRST, 0,
                 NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1,
                 G_TYPE_STRING);
}

/**
 * This function creates a new LockSelectionDialog.
 *
 * @param target Which keys to filter for. Encryption-capable keys if true, signing-capable keys if false
 *
 * @return LockSelectionDialog
 */
LockSelectionDialog *lock_selection_dialog_new(gboolean target)
{
    LockSelectionDialog *dialog =
        g_object_new(LOCK_TYPE_SELECTION_DIALOG, NULL);

    /* TODO: implement g_object_class_install_property() */
    dialog->target = target;
    lock_selection_dialog_refresh(NULL, dialog);

    return dialog;
}

/**
 * This function updates the UI of a LockSelectionDialog to indicate whether something is currently loading.
 *
 * @param dialog Dialog to update the UI of
 * @param spinning Whether something is happening
 */
void lock_selection_dialog_show_spinner(LockSelectionDialog *dialog,
                                        gboolean spinning)
{
    gtk_widget_set_visible(GTK_WIDGET(dialog->spinner), spinning);
    gtk_widget_set_visible(GTK_WIDGET(dialog->view), !spinning);
}

/**
 * This function refreshes the key list of a LockSelectionDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void lock_selection_dialog_refresh(GtkButton *self, LockSelectionDialog *dialog)
{
    (void)self;

    lock_selection_dialog_show_spinner(dialog, true);

    gtk_list_box_remove_all(dialog->key_box);

    gpgme_ctx_t context;
    gpgme_key_t key;
    gpgme_error_t error;

    error = gpgme_new(&context);
    if (error)
        return;

    error = gpgme_op_keylist_start(context, NULL, (dialog->target) ? 0 : 1);
    while (!error) {
        error = gpgme_op_keylist_next(context, &key);

        if (error)
            break;

        if (dialog->target && !key->can_encrypt) {
            continue;
        } else if (!dialog->target && !key->can_sign) {
            continue;
        }

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

    lock_selection_dialog_show_spinner(dialog, false);
}

/**
 * This function handles key selection of a LockSelectionDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.ListBox.row-activated.html
 * @param row https://docs.gtk.org/gtk4/signal.ListBox.row-activated.html
 * @param dialog https://docs.gtk.org/gtk4/signal.ListBox.row-activated.html
 */
void lock_selection_dialog_key_selected(GtkButton *self,
                                        GtkListBoxRow *row,
                                        LockSelectionDialog *dialog)
{
    (void)self;

    const gchar *fingerprint = adw_action_row_get_subtitle(ADW_ACTION_ROW(row));
    g_signal_emit_by_name(dialog, "entered", fingerprint);

    adw_dialog_close(ADW_DIALOG(dialog));
}
