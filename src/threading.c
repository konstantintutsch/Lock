#include "threading.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "selectiondialog.h"
#include "managementdialog.h"
#include "keyrow.h"

#include <string.h>

/**
 * This function creates threads a function.
 *
 * @param id ID of the thread
 * @param purpose Purpose of the thread to create. Used in error messages
 * @param function Function to thread
 * @param data Data to pass to the function
 *
 * @return Success
 */
bool thread_create(const gchar *id, const gchar *purpose, gpointer function,
                   gpointer data)
{
    GError *error = NULL;

    g_thread_try_new(id, function, data, &error);

    if (error == NULL)
        return true;

    g_warning("%s: %s", purpose, error->message);

    /* Cleanup */
    g_error_free(error);
    error = NULL;

    return false;
}

/**
 * This function creates a new thread for the encryption of the text view of a LockWindow.
 *
 * @param self LockSelectionDialog::entered
 * @param fingerprint LockSelectionDialog::entered
 * @param window LockSelectionDialog::entered
 */
void thread_encrypt_text(LockSelectionDialog *self, const char *fingerprint,
                         LockWindow *window)
{
    (void)self;

    lock_window_set_fingerprint(window, fingerprint);

    bool success = thread_create("encrypt_text",
                                 "Failed to create text encryption thread",
                                 lock_window_encrypt_text, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the encryption of the input file of a LockWindow.
 *
 * @param self LockSelectionDialog::entered
 * @param fingerprint LockSelectionDialog::entered
 * @param window LockSelectionDialog::entered
 */
void thread_encrypt_file(LockSelectionDialog *self, const char *fingerprint,
                         LockWindow *window)
{
    (void)self;

    lock_window_set_fingerprint(window, fingerprint);
    lock_window_file_select_output_directory_dialog_present(window);

    bool success = thread_create("encrypt_file",
                                 "Failed to create file encryption thread",
                                 lock_window_encrypt_file, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the decryption of the text view of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
void thread_decrypt_text(GSimpleAction *self, GVariant *parameter,
                         LockWindow *window)
{
    (void)self;
    (void)parameter;

    bool success = thread_create("decrypt_text",
                                 "Failed to create text decryption thread",
                                 lock_window_decrypt_text, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the decryption of the input file of a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void thread_decrypt_file(GtkButton *self, LockWindow *window)
{
    (void)self;

    lock_window_file_select_output_directory_dialog_present(window);

    bool success = thread_create("decrypt_file",
                                 "Failed to create file decryption thread",
                                 lock_window_decrypt_file, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the signing of the text view of a LockWindow.
 *
 * @param self LockSelectionDialog::entered
 * @param fingerprint LockSelectionDialog::entered
 * @param window LockSelectionDialog::entered
 */
void thread_sign_text(LockSelectionDialog *self, const char *fingerprint,
                      LockWindow *window)
{
    (void)self;

    lock_window_set_fingerprint(window, fingerprint);

    bool success = thread_create("sign_text",
                                 "Failed to create text signing thread",
                                 lock_window_sign_text, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the signing of the input file of a LockWindow.
 *
 * @param self LockSelectionDialog::entered
 * @param fingerprint LockSelectionDialog::entered
 * @param window LockSelectionDialog::entered
 */
void thread_sign_file(LockSelectionDialog *self, const char *fingerprint,
                      LockWindow *window)
{
    (void)self;

    lock_window_set_fingerprint(window, fingerprint);
    lock_window_file_select_output_directory_dialog_present(window);

    bool success = thread_create("sign_file",
                                 "Failed to create file signing thread",
                                 lock_window_sign_file, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the verification of the text view of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
void thread_verify_text(GSimpleAction *self, GVariant *parameter,
                        LockWindow *window)
{
    (void)self;
    (void)parameter;

    bool success = thread_create("verify_text",
                                 "Failed to create text verification thread",
                                 lock_window_verify_text, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the verification of the input file of a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void thread_verify_file(GtkButton *self, LockWindow *window)
{
    (void)self;

    lock_window_file_select_output_directory_dialog_present(window);

    bool success = thread_create("verify_file",
                                 "Failed to create file verification thread",
                                 lock_window_verify_file, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the import of a file as a key of a LockManagementDialog.
 *
 * @param dialog Dialog to import the key in
 */
void thread_import_key(LockManagementDialog *dialog)
{
    bool success = thread_create("import_key",
                                 "Failed to create key import thread",
                                 lock_management_dialog_import, dialog);

    if (success)
        lock_management_dialog_show_spinner(dialog, true);
}

/**
 * This function creates a new thread for the generation of a new key pair in a LockManagementDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void thread_generate_key(GtkButton *self, LockManagementDialog *dialog)
{
    (void)self;

    if (!lock_management_dialog_generate_ready(dialog))
        return;

    bool success = thread_create("generate_key",
                                 "Failed to create key pair generation thread",
                                 lock_management_dialog_generate, dialog);

    if (success)
        lock_management_dialog_show_spinner(dialog, true);
}

/**
 * This function creates a new thread for the export of a key as a file in a LockKeyRow.
 *
 * @param row Row to export the key of
 */
void thread_export_key(LockKeyRow *row)
{
    thread_create("export_key",
                  "Failed to create key export thread",
                  lock_key_row_export, row);
}

/**
 * This function creates a new thread for the removal of a key in a LockKeyRow.
 *
 * @param row Row to remove the key of
 */
void thread_remove_key(LockKeyRow *row)
{
    thread_create("remove_key",
                  "Failed to create key removal thread",
                  lock_key_row_remove, row);
}
