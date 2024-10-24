#include "threading.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "entrydialog.h"
#include "keydialog.h"
#include "keyrow.h"

#include <string.h>

#define CRYPTOGRAPHY_THREAD_WRAPPER(ID, Target, Function, Data) GError *error = NULL; \
    \
    g_thread_try_new(ID, (gpointer)Function, Data, &error); \
    \
    if (error == NULL) \
        return; \
    \
    g_warning(C_("First format specifier is a translation string marked as “Thread Error”", "Failed to create %s thread: %s"), Target, error->message); \
    \
    /* Cleanup */ \
    g_error_free(error); \
    error = NULL;

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

    CRYPTOGRAPHY_THREAD_WRAPPER("encrypt_text",
                                C_("Thread Error", "text encryption"),
                                lock_window_encrypt_text, window);

    lock_window_set_uid(window, "");
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

    CRYPTOGRAPHY_THREAD_WRAPPER("encrypt_file",
                                C_("Thread Error", "file encryption"),
                                lock_window_encrypt_file, window);

    lock_window_set_uid(window, "");
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

    CRYPTOGRAPHY_THREAD_WRAPPER("decrypt_text",
                                C_("Thread Error", "text decryption"),
                                lock_window_decrypt_text, window);
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

    CRYPTOGRAPHY_THREAD_WRAPPER("decrypt_file",
                                C_("Thread Error", "file decryption"),
                                lock_window_decrypt_file, window);
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

    CRYPTOGRAPHY_THREAD_WRAPPER("sign_text", C_("Thread Error", "text signing"),
                                lock_window_sign_text, window);
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

    CRYPTOGRAPHY_THREAD_WRAPPER("sign_file", C_("Thread Error", "file signing"),
                                lock_window_sign_file, window);
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

    CRYPTOGRAPHY_THREAD_WRAPPER("verify_text",
                                C_("Thread Error", "text verification"),
                                lock_window_verify_text, window);
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

    CRYPTOGRAPHY_THREAD_WRAPPER("verify_file",
                                C_("Thread Error", "file verification"),
                                lock_window_verify_file, window);
}

/**
 * This function creates a new thread for the import of a file as a key of a LockKeyDialog.
 *
 * @param dialog Dialog to import the key in
 */
void thread_import_key(LockKeyDialog *dialog)
{
    CRYPTOGRAPHY_THREAD_WRAPPER("import_key",
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

    CRYPTOGRAPHY_THREAD_WRAPPER("generate_key",
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
    CRYPTOGRAPHY_THREAD_WRAPPER("export_key",
                                C_("Thread Error", "key export"),
                                lock_key_row_export, row);
}

/**
 * This function creates a new thread for the backup of the keyring in a LockWindow.
 *
 * @param window Window to process the backup in
 */
void thread_backup_keyring(LockWindow *window)
{
    CRYPTOGRAPHY_THREAD_WRAPPER("backup_keyring",
                                C_("Thread Error", "keyring backup"),
                                lock_window_backup, window);
}

/**
 * This function creates a new thread for the removal of a key in a LockKeyRow.
 *
 * @param row Row to remove the key of
 */
void thread_remove_key(LockKeyRow *row)
{
    CRYPTOGRAPHY_THREAD_WRAPPER("remove_key",
                                C_("Thread Error", "key removal"),
                                lock_key_row_remove, row);
}
