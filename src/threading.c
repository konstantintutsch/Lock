#include "threading.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "entrydialog.h"
#include "keydialog.h"
#include "keyrow.h"

#include <string.h>

/**
 * This function creates threads a function.
 *
 * @param id ID of the thread
 * @param purpose Purpose of the thread to create (displayed in the error message)
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

    g_warning(C_
              ("First format specifier is a translation string marked as “Thread Error”",
               "Failed to create %s thread: %s"), purpose, error->message);

    /* Cleanup */
    g_error_free(error);
    error = NULL;

    return false;
}

/**
 * This function creates a new thread for the encryption of the text view of a LockWindow.
 *
 * @param self LockEntryDialog::entered
 * @param email LockEntryDialog::entered
 * @param window LockEntryDialog::entered
 */
void thread_encrypt_text(LockEntryDialog *self, const char *uid,
                         LockWindow *window)
{
    (void)self;

    lock_window_set_uid(window, uid);

    bool success = thread_create("encrypt_text",
                                 C_("Thread Error", "text encryption"),
                                 lock_window_encrypt_text, window);

    lock_window_set_uid(window, "");

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the encryption of the input file of a LockWindow.
 *
 * @param self LockEntryDialog::entered
 * @param email LockEntryDialog::entered
 * @param window LockEntryDialog::entered
 */
void thread_encrypt_file(LockEntryDialog *self, const char *uid,
                         LockWindow *window)
{
    (void)self;

    lock_window_set_uid(window, uid);

    bool success = thread_create("encrypt_file",
                                 C_("Thread Error", "file encryption"),
                                 lock_window_encrypt_file, window);

    lock_window_set_uid(window, "");

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
                                 C_("Thread Error", "text decryption"),
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

    bool success = thread_create("decrypt_file",
                                 C_("Thread Error", "file decryption"),
                                 lock_window_decrypt_file, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the signing of the text view of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
void thread_sign_text(GSimpleAction *self, GVariant *parameter,
                      LockWindow *window)
{
    (void)self;
    (void)parameter;

    bool success =
        thread_create("sign_text", C_("Thread Error", "text signing"),
                      lock_window_sign_text, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the signing of the input file of a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void thread_sign_file(GtkButton *self, LockWindow *window)
{
    (void)self;

    bool success =
        thread_create("sign_file", C_("Thread Error", "file signing"),
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
                                 C_("Thread Error", "text verification"),
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

    bool success = thread_create("verify_file",
                                 C_("Thread Error", "file verification"),
                                 lock_window_verify_file, window);

    if (success)
        lock_window_cryptography_processing(window, true);
}

/**
 * This function creates a new thread for the import of a file as a key of a LockKeyDialog.
 *
 * @param dialog Dialog to import the key in
 */
void thread_import_key(LockKeyDialog *dialog)
{
    thread_create("import_key",
                  C_("Thread Error", "key import"),
                  lock_key_dialog_import, dialog);
}

/**
 * This function creates a new thread for the generation of a new keypair in a LockKeyDialog.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param dialog https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void thread_generate_key(GtkButton *self, LockKeyDialog *dialog)
{
    (void)self;

    thread_create("generate_key",
                  C_("Thread Error", "key generation"),
                  lock_key_dialog_generate, dialog);
}

/**
 * This function creates a new thread for the export of a key as a file in a LockKeyRow.
 *
 * @param row Row to export the key of
 */
void thread_export_key(LockKeyRow *row)
{
    thread_create("export_key",
                  C_("Thread Error", "key export"), lock_key_row_export, row);
}

/**
 * This function creates a new thread for the removal of a key in a LockKeyRow.
 *
 * @param row Row to remove the key of
 */
void thread_remove_key(LockKeyRow *row)
{
    thread_create("remove_key",
                  C_("Thread Error", "key removal"), lock_key_row_remove, row);
}
