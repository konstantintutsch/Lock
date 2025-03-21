#include "window.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "application.h"
#include "filerow.h"
#include "selectiondialog.h"
#include "managementdialog.h"
#include "config.h"

#include <gpgme.h>
#include "cryptography.h"
#include "threading.h"
#include "text.h"

#define ACTION_MODE_TEXT 0
#define ACTION_MODE_FILE 1

/**
 * This structure handles data of a window.
 */
struct _LockWindow {
    AdwApplicationWindow parent;

    AdwToastOverlay *toast_overlay;

    AdwSpinner *processing_spinner;
    AdwViewStack *stack;
    unsigned int action_mode;

    gchar *fingerprint; /**< Stores the selected fingerprint for an encryption or signing process. */
    gpgme_sig_mode_t signature_mode; /**< Stores the selected signature mode for a signing process. */
    gchar *signer; /**< Stores the signer of the verified signature from a verification process. */

    /* Text */
    AdwViewStackPage *text_page;
    AdwSplitButton *text_button;
    gboolean text_success;
    GtkTextBuffer *text_queue; /**< Text from the last cryptography operation on text */
    GtkTextView *text_view;

    /* File */
    AdwViewStackPage *file_page;
    GtkBox *file_box;
    AdwStatusPage *file_status;
    GtkListBox *file_list;
    AdwButtonRow *file_open_button;
    AdwButtonRow *file_clear_button;

    GFile *file_output_directory;
    enum dialog_status file_output_status;
    gboolean file_success; /**< Success of the last cryptography operation on files */

    GtkBox *file_process_box;
    GtkButton *file_encrypt_button;
    GtkButton *file_decrypt_button;
    GtkButton *file_sign_button;
    GtkButton *file_verify_button;
};

G_DEFINE_TYPE(LockWindow, lock_window, ADW_TYPE_APPLICATION_WINDOW);

/* UI */
static void lock_window_stack_page_on_changed(AdwViewStack * self,
                                              GParamSpec * pspec,
                                              LockWindow * window);
void lock_window_lock_file_processing(LockWindow * window, gboolean lock);
static void lock_window_on_file_selection(GtkListBox * self,
                                          LockWindow * window);

// Encryption
gboolean lock_window_encrypt_text_on_completed(LockWindow * window);
gboolean lock_window_encrypt_file_on_completed(LockWindow * window);

// Decryption
gboolean lock_window_decrypt_text_on_completed(LockWindow * window);
gboolean lock_window_decrypt_file_on_completed(LockWindow * window);

// Signing
gboolean lock_window_sign_text_on_completed(LockWindow * window);
gboolean lock_window_sign_file_on_completed(LockWindow * window);

// Verification
gboolean lock_window_verify_text_on_completed(LockWindow * window);
gboolean lock_window_verify_file_on_completed(LockWindow * window);

/* Key management */
static void lock_window_management_dialog(GSimpleAction * action,
                                          GVariant * parameter,
                                          LockWindow * window);

/* Text */
static void lock_window_text_view_copy(AdwSplitButton * self,
                                       LockWindow * window);
static void lock_window_text_view_copy_action(GSimpleAction * action,
                                              GVariant * parameter,
                                              LockWindow * window);

static void lock_window_text_view_paste(GSimpleAction * action,
                                        GVariant * parameter,
                                        LockWindow * window);
static void lock_window_text_view_paste_finish(GObject * object,
                                               GAsyncResult * result,
                                               gpointer data);

static void lock_window_text_queue_set_text(LockWindow * window,
                                            const char *text);

/* File */
gboolean
lock_window_on_file_drop(GtkDropTarget * target,
                         const GValue * value, double x, double y,
                         gpointer data);
static void lock_window_file_open_dialog_finish(GObject * source_object,
                                                GAsyncResult * res,
                                                gpointer data);
static void lock_window_file_open_dialog_present(AdwButtonRow * self,
                                                 LockWindow * window);
void lock_window_file_list_clear(AdwButtonRow * self, LockWindow * window);

/* Encryption */
void lock_window_encrypt_text_dialog(GSimpleAction * self, GVariant * parameter,
                                     LockWindow * window);
void lock_window_encrypt_file_dialog(GtkButton * self, LockWindow * window);

/* Signing */
void lock_window_sign_text_dialog(GSimpleAction * self, GVariant * parameter,
                                  LockWindow * window);
void lock_window_sign_file_dialog(GtkButton * self, LockWindow * window);

/**
 * This function initializes a LockWindow.
 *
 * @param window Window to be initialized
 */
static void lock_window_init(LockWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));

    window->fingerprint = malloc((0 + 1) * sizeof(char));

    /* UI */
    g_signal_connect(window->stack, "notify::visible-child",
                     G_CALLBACK(lock_window_stack_page_on_changed), window);
    lock_window_stack_page_on_changed(window->stack, NULL, window);

    lock_window_cryptography_processing(window, false);

    /* Key management */

    g_autoptr(GSimpleAction) manage_keys_action =
        g_simple_action_new("manage_keys", NULL);
    g_signal_connect(manage_keys_action, "activate",
                     G_CALLBACK(lock_window_management_dialog), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(manage_keys_action));

    /* Text */
    g_signal_connect(window->text_button, "clicked",
                     G_CALLBACK(lock_window_text_view_copy), window);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(window->text_view),
                             _("Enter text …"), -1);

    window->text_queue = gtk_text_buffer_new(NULL);
    lock_window_text_queue_set_text(window, "");

    g_autoptr(GSimpleAction) copy_text_action =
        g_simple_action_new("copy_text", NULL);
    g_signal_connect(copy_text_action, "activate",
                     G_CALLBACK(lock_window_text_view_copy_action), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(copy_text_action));

    g_autoptr(GSimpleAction) paste_text_action =
        g_simple_action_new("paste_text", NULL);
    g_signal_connect(paste_text_action, "activate",
                     G_CALLBACK(lock_window_text_view_paste), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(paste_text_action));

    // Encrypt
    g_autoptr(GSimpleAction) encrypt_text_action =
        g_simple_action_new("encrypt_text", NULL);
    g_signal_connect(encrypt_text_action, "activate",
                     G_CALLBACK(lock_window_encrypt_text_dialog), window);
    g_action_map_add_action(G_ACTION_MAP(window),
                            G_ACTION(encrypt_text_action));

    // Decrypt
    g_autoptr(GSimpleAction) decrypt_text_action =
        g_simple_action_new("decrypt_text", NULL);
    g_signal_connect(decrypt_text_action, "activate",
                     G_CALLBACK(thread_decrypt_text), window);
    g_action_map_add_action(G_ACTION_MAP(window),
                            G_ACTION(decrypt_text_action));

    // Sign
    g_autoptr(GSimpleAction) sign_text_action =
        g_simple_action_new("sign_text", NULL);
    g_signal_connect(sign_text_action, "activate",
                     G_CALLBACK(lock_window_sign_text_dialog), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(sign_text_action));

    g_autoptr(GSimpleAction) sign_text_clear_action =
        g_simple_action_new("sign_text_clear", NULL);
    g_signal_connect(sign_text_clear_action, "activate",
                     G_CALLBACK(lock_window_sign_text_dialog), window);
    g_action_map_add_action(G_ACTION_MAP(window),
                            G_ACTION(sign_text_clear_action));

    g_autoptr(GSimpleAction) sign_text_detach_action =
        g_simple_action_new("sign_text_detach", NULL);
    g_signal_connect(sign_text_detach_action, "activate",
                     G_CALLBACK(lock_window_sign_text_dialog), window);
    g_action_map_add_action(G_ACTION_MAP(window),
                            G_ACTION(sign_text_detach_action));

    // Verify
    g_autoptr(GSimpleAction) verify_text_action =
        g_simple_action_new("verify_text", NULL);
    g_signal_connect(verify_text_action, "activate",
                     G_CALLBACK(thread_verify_text), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(verify_text_action));

    /* File */
    GtkDropTarget *file_target =
        gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
    gtk_drop_target_set_gtypes(file_target, (GType[1]) {
                               G_TYPE_FILE}, 1);

    g_signal_connect(file_target, "drop", G_CALLBACK(lock_window_on_file_drop),
                     window);
    gtk_widget_add_controller(GTK_WIDGET(window->file_box),
                              GTK_EVENT_CONTROLLER(file_target));

    window->file_output_status = SELECTING;

    g_signal_connect(window->file_list, "selected-rows-changed",
                     G_CALLBACK(lock_window_on_file_selection), window);
    lock_window_on_file_selection(NULL, window);

    g_signal_connect(window->file_open_button, "activated",
                     G_CALLBACK(lock_window_file_open_dialog_present), window);
    g_signal_connect(window->file_clear_button, "activated",
                     G_CALLBACK(lock_window_file_list_clear), window);

    // Encrypt
    g_signal_connect(window->file_encrypt_button, "clicked",
                     G_CALLBACK(lock_window_encrypt_file_dialog), window);
    // Decrypt
    g_signal_connect(window->file_decrypt_button, "clicked",
                     G_CALLBACK(thread_decrypt_file), window);
    // Sign
    g_signal_connect(window->file_sign_button, "clicked",
                     G_CALLBACK(lock_window_sign_file_dialog), window);
    // Verify
    g_signal_connect(window->file_verify_button, "clicked",
                     G_CALLBACK(thread_verify_file), window);
}

/**
 * This function initializes a LockWindow class.
 *
 * @param class Window class to be initialized
 */
static void lock_window_class_init(LockWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                                UI_RESOURCE("window.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         processing_spinner);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         toast_overlay);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         stack);

    /* Text */
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_page);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         text_view);

    /* File */
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_page);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_box);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_status);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_list);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_open_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_clear_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_process_box);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_encrypt_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_decrypt_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_sign_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockWindow,
                                         file_verify_button);
}

/**
 * This function creates a new LockWindow.
 *
 * @param app Application to create the new window for
 *
 * @return LockWindow
 */
LockWindow *lock_window_new(LockApplication *app)
{
    return g_object_new(LOCK_TYPE_WINDOW, "application", app, NULL);
}

/**
 * This function opens a LockWindow.
 *
 * @param window Window to be opened
 * @param file File to be processed with the window
 */

void lock_window_open(LockWindow *window, GFile *file)
{
    (void)window;
    (void)file;
}

/**** UI ****/

/**
 * This functions updates the UI to indicate whether any cryptography operations are currently processing.
 *
 * @param window Window to update the UI of
 * @param processing Whether an operation is currently being executed
 */
void lock_window_cryptography_processing(LockWindow *window,
                                         gboolean processing)
{
    lock_window_lock_file_processing(window, processing);
    gtk_widget_set_visible(GTK_WIDGET(window->processing_spinner), processing);
}

/**
 * This function updates the UI on a change of a page of the stack page of a LockWindow.
 *
 * @param self https://docs.gtk.org/gobject/signal.Object.notify.html
 * @param pspec https://docs.gtk.org/gobject/signal.Object.notify.html
 * @param window https://docs.gtk.org/gobject/signal.Object.notify.html
 */
static void lock_window_stack_page_on_changed(AdwViewStack *self,
                                              GParamSpec *pspec,
                                              LockWindow *window)
{
    (void)pspec;

    GtkWidget *visible_child = adw_view_stack_get_visible_child(self);
    AdwViewStackPage *visible_page =
        adw_view_stack_get_page(self, visible_child);
    adw_view_stack_page_set_needs_attention(visible_page, false);

    if (visible_page == window->text_page) {
        window->action_mode = ACTION_MODE_TEXT;

        gtk_widget_set_visible(GTK_WIDGET(window->text_button), true);
    } else if (visible_page == window->file_page) {
        window->action_mode = ACTION_MODE_FILE;

        gtk_widget_set_visible(GTK_WIDGET(window->text_button), false);
    }

    /* Cleanup */
    visible_child = NULL;
    visible_page = NULL;
}

/**
 * This functions updates the UI to lock or unlock file processing options.
 *
 * @param window Window to lock the file processing of
 * @param lock Lock or unlock file processing
 */
void lock_window_lock_file_processing(LockWindow *window, gboolean lock)
{
    GtkWidget *process_widget =
        gtk_widget_get_first_child(GTK_WIDGET(window->file_process_box));
    while (process_widget != NULL) {
        gtk_widget_set_sensitive(process_widget, !lock);

        process_widget = gtk_widget_get_next_sibling(process_widget);
    }

    /* Cleanup */
    g_free(process_widget);
    process_widget = NULL;
}

/**
 * This function updates the UI on a selection of an input or output file in a LockWindow.
 *
 * @param self GtkListBox::selected-rows-changed
 * @param window GtkListBox::selected-rows-changed
 */
static void lock_window_on_file_selection(GtkListBox *self, LockWindow *window)
{
    (void)self;

    gboolean ready =
        (gtk_list_box_get_row_at_index(window->file_list, 0) != NULL);

    gtk_widget_set_visible(GTK_WIDGET(window->file_status), !ready);
    gtk_widget_set_visible(GTK_WIDGET(window->file_list), ready);

    lock_window_lock_file_processing(window, !ready);
}

/**** Key management ****/

/**
 * This function initializes the UI required for managing keys.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
static void lock_window_management_dialog(GSimpleAction *action,
                                          GVariant *parameter,
                                          LockWindow *window)
{
    (void)action;
    (void)parameter;

    LockManagementDialog *dialog = lock_management_dialog_new(window);

    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

/**** Text ****/

/**
 * This function gets the text from the text view of a LockWindow.
 *
 * @param window Window to get the text from
 *
 * @return Text. Freeing required
 */
gchar *lock_window_text_view_get_text(LockWindow *window)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(window->text_view);

    GtkTextIter start_iter;
    GtkTextIter end_iter;
    gtk_text_buffer_get_start_iter(buffer, &start_iter);
    gtk_text_buffer_get_end_iter(buffer, &end_iter);

    return gtk_text_buffer_get_text(buffer, &start_iter, &end_iter, true);
}

/**
 * This functions sets the text of the text view of a LockWindow.
 *
 * @param window Window to set the text in
 * @param text Text to overwrite the text views buffer with
 */
static void lock_window_text_view_set_text(LockWindow *window, const char *text)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(window->text_view);

    gtk_text_buffer_set_text(buffer, text, -1);
}

/**
 * This function copies text from the text view of a LockWindow.
 *
 * @param self https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/signal.SplitButton.clicked.html
 * @param window https://gnome.pages.gitlab.gnome.org/libadwaita/doc/1-latest/signal.SplitButton.clicked.html
 */
static void lock_window_text_view_copy(AdwSplitButton *self, LockWindow *window)
{
    (void)self;

    GdkClipboard *active_clipboard =
        gdk_display_get_clipboard(gdk_display_get_default());

    AdwToast *toast = adw_toast_new(_("Text copied"));
    adw_toast_set_timeout(toast, 2);

    gchar *text = lock_window_text_view_get_text(window);
    gdk_clipboard_set_text(active_clipboard, text);

    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Cleanup */
    g_free(text);
    text = NULL;
}

/**
 * This function wraps text copying from the text view to the simple action API.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
static void lock_window_text_view_copy_action(GSimpleAction *action,
                                              GVariant *parameter,
                                              LockWindow *window)
{
    (void)action;
    (void)parameter;

    lock_window_text_view_copy(NULL, window);
}

/**
 * This function overwrites the text view of a LockWindow with the contents of the system clipboard.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
static void lock_window_text_view_paste(GSimpleAction *action,
                                        GVariant *parameter, LockWindow *window)
{
    (void)action;
    (void)parameter;

    GdkClipboard *active_clipboard =
        gdk_display_get_clipboard(gdk_display_get_default());

    gdk_clipboard_read_text_async(active_clipboard, NULL,
                                  lock_window_text_view_paste_finish, window);
}

/**
 * This function finished the overwriting of the text view of a LockWindow.
 *
 * @param object https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param result https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param data https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 */
static void lock_window_text_view_paste_finish(GObject *object,
                                               GAsyncResult *result,
                                               gpointer data)
{
    GdkClipboard *clipboard = GDK_CLIPBOARD(object);
    LockWindow *window = LOCK_WINDOW(data);

    GError *error = NULL;

    gchar *text = gdk_clipboard_read_text_finish(clipboard, result, &error);

    if (error != NULL) {
        g_warning(_("Could not read text from clipboard: %s"), error->message);

        /* Cleanup */
        g_error_free(error);
        error = NULL;

        return;
    }

    AdwToast *toast = adw_toast_new(_("Text pasted"));
    adw_toast_set_timeout(toast, 2);

    lock_window_text_view_set_text(window, text);

    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Text */
    g_free(text);
    text = NULL;
}

/**
 * This function gets the text from the text queue of a LockWindow.
 *
 * @param window Window to get the text of the queue from
 *
 * @return Text. Freeing required
 */
gchar *lock_window_text_queue_get_text(LockWindow *window)
{
    GtkTextIter start_iter;
    GtkTextIter end_iter;
    gtk_text_buffer_get_start_iter(window->text_queue, &start_iter);
    gtk_text_buffer_get_end_iter(window->text_queue, &end_iter);

    return gtk_text_buffer_get_text(window->text_queue, &start_iter, &end_iter,
                                    true);
}

/**
 * This functions sets the text of the text queue of a LockWindow.
 *
 * @param window Window to set the text in
 * @param text Text to overwrite the text queue buffer with
 */
static void lock_window_text_queue_set_text(LockWindow *window,
                                            const char *text)
{
    gtk_text_buffer_set_text(window->text_queue, text, -1);
}

/**** File ****/

/**
 * This function opens a file in a LockWindow
 *
 * @param window Window to open the file in
 * @param file File to open
 */
void lock_window_file_open(LockWindow *window, GFile *file)
{
    gtk_list_box_append(window->file_list, GTK_WIDGET(lock_file_row_new(file)));
    gtk_list_box_select_row(window->file_list,
                            gtk_list_box_get_row_at_index(window->file_list,
                                                          0));

    GtkWidget *visible_child = adw_view_stack_get_visible_child(window->stack);
    AdwViewStackPage *visible_page =
        adw_view_stack_get_page(window->stack, visible_child);

    if (visible_page != window->file_page)
        adw_view_stack_page_set_needs_attention(window->file_page, true);

    /* Cleanup */
    visible_child = NULL;
    visible_page = NULL;
}

/**
 * This function opens a dropped file.
 *
 * @param target Gtk.DropTarget::drop
 * @param value Gtk.DropTarget::drop
 * @param x Gtk.DropTarget::drop
 * @param y Gtk.DropTarget::drop
 * @param data Gtk.DropTarget::drop
 *
 * @return Gtk.DropTarget::drop
 */
gboolean
lock_window_on_file_drop(GtkDropTarget *target,
                         const GValue *value, double x, double y, gpointer data)
{
    (void)target;
    (void)x;
    (void)y;

    if (!G_VALUE_HOLDS(value, G_TYPE_FILE))
        return false;

    LockWindow *window = LOCK_WINDOW(data);

    lock_window_file_open(window, g_value_get_object(value));

    /* Cleanup */
    window = NULL;

    return true;
}

/**
 * This function opens the input file of a LockWindow from a file dialog.
 *
 * @param object https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param result https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param user_data https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 */
static void lock_window_file_open_dialog_finish(GObject *source_object,
                                                GAsyncResult *res,
                                                gpointer data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    LockWindow *window = LOCK_WINDOW(data);

    window->file_output_status = SELECTING;

    GListModel *files = gtk_file_dialog_open_multiple_finish(dialog, res, NULL);
    if (files == NULL)
        goto cleanup;

    GFile *file;
    int file_item = 0;
    while ((file = g_list_model_get_item(files, file_item))) {
        lock_window_file_open(window, g_file_dup(file));
        file_item++;
    }

    g_free(file);
    file = NULL;

    g_free(files);
    files = NULL;

 cleanup:
    g_object_unref(dialog);
    dialog = NULL;

    window = NULL;
}

/**
 * This function opens an open file dialog for a LockWindow.
 *
 * @param self Adw.ButtonRow::activate
 * @param window Adw.ButtonRow::activate
 */
static void
lock_window_file_open_dialog_present(AdwButtonRow *self, LockWindow *window)
{
    (void)self;

    GtkFileDialog *dialog = gtk_file_dialog_new();
    GCancellable *cancel = g_cancellable_new();

    gtk_file_dialog_open_multiple(dialog, GTK_WINDOW(window),
                                  cancel, lock_window_file_open_dialog_finish,
                                  window);
}

/**
 * This function opens the selected output directory of a LockWindow.
 *
 * @param object https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param result https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param user_data https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 */
void lock_window_file_select_output_directory(GObject *source_object,
                                              GAsyncResult *res, gpointer data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    LockWindow *window = LOCK_WINDOW(data);

    window->file_output_directory =
        gtk_file_dialog_select_folder_finish(dialog, res, NULL);
    window->file_output_status =
        (window->file_output_directory == NULL) ? ABORTED : SELECTED;

    /* Cleanup */
    g_object_unref(dialog);
    dialog = NULL;

    window = NULL;
}

/**
 * This function opens an output folder selection dialog for a LockWindow.
 *
 * @param window Window to present the file dialog on
 */
void lock_window_file_select_output_directory_dialog_present(LockWindow *window)
{
    GtkFileDialog *dialog = gtk_file_dialog_new();
    GCancellable *cancel = g_cancellable_new();

    gtk_file_dialog_set_title(dialog, _("Save files"));
    gtk_file_dialog_set_accept_label(dialog, C_("Save files", "Save"));

    window->file_output_status = SELECTING;
    gtk_file_dialog_select_folder(dialog, GTK_WINDOW(window), cancel,
                                  lock_window_file_select_output_directory,
                                  window);
}

/**
 * This function closes all opened files
 *
 * @param self Adw.ButtonRow::activate
 * @param window Adw.ButtonRow::activate
 */
void lock_window_file_list_clear(AdwButtonRow *self, LockWindow *window)
{
    (void)self;

    gtk_list_box_remove_all(window->file_list);
}

/****** Cryptography ******/

/**
 * This function overwrites the key fingerprint of a LockWindow.
 *
 * @param window Window to overwrite the key fingerprint of
 * @param fingerprint Fingerprint to overwrite with
 */
void lock_window_set_fingerprint(LockWindow *window, const char *fingerprint)
{
    window->fingerprint =
        realloc(window->fingerprint, (strlen(fingerprint) + 1) * sizeof(char));
    strcpy(window->fingerprint, fingerprint);
}

/**** Encryption ****/

/**
 * This function handles user input to select the target key for a text encryption process of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
void lock_window_encrypt_text_dialog(GSimpleAction *self, GVariant *parameter,
                                     LockWindow *window)
{
    (void)self;
    (void)parameter;

    LockSelectionDialog *dialog = lock_selection_dialog_new(true);

    g_signal_connect(dialog, "entered", G_CALLBACK(thread_encrypt_text),
                     window);

    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

/**
 * This function handles user input to select the target key for a file encryption process of a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void lock_window_encrypt_file_dialog(GtkButton *self, LockWindow *window)
{
    (void)self;

    LockSelectionDialog *dialog = lock_selection_dialog_new(true);

    g_signal_connect(dialog, "entered", G_CALLBACK(thread_encrypt_file),
                     window);

    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

/**
 * This function encrypts text from the text view of a LockWindow.
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 */
void lock_window_encrypt_text(LockWindow *window)
{
    gchar *plain = lock_window_text_view_get_text(window);

    gpgme_key_t key = key_get(window->fingerprint);

    gchar *armor = text_encrypt(plain, key);
    if (armor == NULL) {
        window->text_success = false;
    } else {
        window->text_success = true;
        lock_window_text_queue_set_text(window, armor);
    }

    /* Cleanup */
    g_free(plain);
    plain = NULL;

    g_free(armor);
    armor = NULL;

    gpgme_key_release(key);

    /* UI */
    g_idle_add((GSourceFunc) lock_window_encrypt_text_on_completed, window);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for text encryption and is supposed to be called via g_idle_add().
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_window_encrypt_text_on_completed(LockWindow *window)
{
    AdwToast *toast;

    if (!window->text_success) {
        toast = adw_toast_new(_("Encryption failed"));
    } else {
        toast = adw_toast_new(_("Text encrypted"));

        gchar *armor = lock_window_text_queue_get_text(window);
        lock_window_text_view_set_text(window, armor);

        /* Cleanup */
        g_free(armor);
        armor = NULL;
    }

    adw_toast_set_timeout(toast, 3);

    lock_window_cryptography_processing(window, false);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**
 * This function encrypts the file of a LockWindow.
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 */
void lock_window_encrypt_file(LockWindow *window)
{
    gpgme_key_t key = key_get(window->fingerprint);

    while (window->file_output_status == SELECTING)
        sleep(1);

    if (window->file_output_status == ABORTED) {
        lock_window_cryptography_processing(window, false);
        g_thread_exit(0);
    }

    GtkWidget *row = gtk_widget_get_first_child(GTK_WIDGET(window->file_list));
    GFile *file = NULL;

    window->file_success = true;
    while (row != NULL && window->file_success) {
        file = lock_file_row_get_file(LOCK_FILE_ROW(row));
        window->file_success =
            file_encrypt(g_file_get_path(file),
                         g_strdup_printf("%s/%s.pgp",
                                         g_file_get_path
                                         (window->file_output_directory),
                                         g_file_get_basename(file)), key);

        row = gtk_widget_get_next_sibling(row);
    }

    g_free(file);
    file = NULL;

    gpgme_key_release(key);

    /* UI */
    g_idle_add((GSourceFunc) lock_window_encrypt_file_on_completed, window);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for file encryption and is supposed to be called via g_idle_add().
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_window_encrypt_file_on_completed(LockWindow *window)
{
    AdwToast *toast;

    if (!window->file_success) {
        toast = adw_toast_new(_("Encryption failed"));
    } else {
        toast = adw_toast_new(_("Files encrypted"));
    }

    adw_toast_set_timeout(toast, 3);

    lock_window_cryptography_processing(window, false);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**** Decryption ****/

/**
 * This function decrypts text from the text view of a LockWindow.
 *
 * @param window https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_window_decrypt_text(LockWindow *window)
{
    gchar *armor = lock_window_text_view_get_text(window);

    gchar *plain = text_decrypt(armor);
    if (plain == NULL) {
        window->text_success = false;
    } else {
        window->text_success = true;
        lock_window_text_queue_set_text(window, plain);
    }

    /* Cleanup */
    g_free(armor);
    armor = NULL;

    g_free(plain);
    plain = NULL;

    /* UI */
    g_idle_add((GSourceFunc) lock_window_decrypt_text_on_completed, window);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for text decryption and is supposed to be called via g_idle_add().
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_window_decrypt_text_on_completed(LockWindow *window)
{
    AdwToast *toast;

    if (!window->text_success) {
        toast = adw_toast_new(_("Decryption failed"));
    } else {
        toast = adw_toast_new(_("Text decrypted"));

        gchar *plain = lock_window_text_queue_get_text(window);
        lock_window_text_view_set_text(window, plain);

        /* Cleanup */
        g_free(plain);
        plain = NULL;
    }

    adw_toast_set_timeout(toast, 3);

    lock_window_cryptography_processing(window, false);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**
 * This function decrypts the input file of a LockWindow.
 *
 * @param window https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_window_decrypt_file(LockWindow *window)
{
    while (window->file_output_status == SELECTING)
        sleep(1);

    if (window->file_output_status == ABORTED) {
        lock_window_cryptography_processing(window, false);
        g_thread_exit(0);
    }

    GtkWidget *row = gtk_widget_get_first_child(GTK_WIDGET(window->file_list));
    GFile *file = NULL;
    gchar *output_path = NULL;

    window->file_success = true;
    while (row != NULL && window->file_success) {
        file = lock_file_row_get_file(LOCK_FILE_ROW(row));
        output_path =
            g_strdup_printf("%s/%s",
                            g_file_get_path(window->file_output_directory),
                            g_file_get_basename(file));

        output_path = remove_extension(output_path, "pgp");
        output_path = remove_extension(output_path, "gpg");

        window->file_success = file_decrypt(g_file_get_path(file), output_path);

        row = gtk_widget_get_next_sibling(row);
    }

    g_free(file);
    file = NULL;

    g_free(output_path);
    output_path = NULL;

    /* UI */
    g_idle_add((GSourceFunc) lock_window_decrypt_file_on_completed, window);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for file decryption and is supposed to be called via g_idle_add().
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_window_decrypt_file_on_completed(LockWindow *window)
{
    AdwToast *toast;

    if (!window->file_success) {
        toast = adw_toast_new(_("Decryption failed"));
    } else {
        toast = adw_toast_new(_("Files decrypted"));
    }

    adw_toast_set_timeout(toast, 3);

    lock_window_cryptography_processing(window, false);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**** Signing ****/

/**
 * This function handles user input to select the source key for a text signing process of a LockWindow.
 *
 * @param self https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param window https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
void lock_window_sign_text_dialog(GSimpleAction *self, GVariant *parameter,
                                  LockWindow *window)
{
    (void)self;
    (void)parameter;

    if (strcmp(g_action_get_name(G_ACTION(self)), "sign_text") == 0) {
        window->signature_mode = GPGME_SIG_MODE_NORMAL;
    } else if (strcmp(g_action_get_name(G_ACTION(self)), "sign_text_clear") ==
               0) {
        window->signature_mode = GPGME_SIG_MODE_CLEAR;
    } else if (strcmp(g_action_get_name(G_ACTION(self)), "sign_text_detach") ==
               0) {
        window->signature_mode = GPGME_SIG_MODE_DETACH;
    }

    LockSelectionDialog *dialog = lock_selection_dialog_new(false);

    g_signal_connect(dialog, "entered", G_CALLBACK(thread_sign_text), window);

    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

/**
 * This function handles user input to select the target key for a file encryption process of a LockWindow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param window https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
void lock_window_sign_file_dialog(GtkButton *self, LockWindow *window)
{
    (void)self;

    LockSelectionDialog *dialog = lock_selection_dialog_new(false);

    g_signal_connect(dialog, "entered", G_CALLBACK(thread_sign_file), window);

    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(window));
}

/**
 * This function signs text from the text view of a LockWindow.
 *
 * @param window https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_window_sign_text(LockWindow *window)
{
    gchar *plain = lock_window_text_view_get_text(window);

    gpgme_key_t key = key_get(window->fingerprint);

    gchar *armor = text_sign(plain, key, window->signature_mode);
    if (armor == NULL) {
        window->text_success = false;
    } else {
        window->text_success = true;
        lock_window_text_queue_set_text(window, armor);
    }

    /* Cleanup */
    g_free(plain);
    plain = NULL;

    g_free(armor);
    armor = NULL;

    gpgme_key_release(key);

    /* UI */
    g_idle_add((GSourceFunc) lock_window_sign_text_on_completed, window);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for text signing and is supposed to be called via g_idle_add().
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_window_sign_text_on_completed(LockWindow *window)
{
    AdwToast *toast;

    if (!window->text_success) {
        toast = adw_toast_new(_("Signing failed"));
    } else {
        toast = adw_toast_new(_("Text signed"));

        gchar *armor = lock_window_text_queue_get_text(window);
        lock_window_text_view_set_text(window, armor);

        /* Cleanup */
        g_free(armor);
        armor = NULL;
    }

    adw_toast_set_timeout(toast, 3);

    lock_window_cryptography_processing(window, false);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**
 * This function signs the input file of a LockWindow.
 *
 * @param window https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_window_sign_file(LockWindow *window)
{
    gpgme_key_t key = key_get(window->fingerprint);

    while (window->file_output_status == SELECTING)
        sleep(1);

    if (window->file_output_status == ABORTED) {
        lock_window_cryptography_processing(window, false);
        g_thread_exit(0);
    }

    GtkWidget *row = gtk_widget_get_first_child(GTK_WIDGET(window->file_list));
    GFile *file = NULL;

    window->file_success = true;
    while (row != NULL && window->file_success) {
        file = lock_file_row_get_file(LOCK_FILE_ROW(row));
        window->file_success =
            file_sign(g_file_get_path(file),
                      g_strdup_printf("%s/%s.pgp",
                                      g_file_get_path
                                      (window->file_output_directory),
                                      g_file_get_basename(file)), key);

        row = gtk_widget_get_next_sibling(row);
    }

    g_free(file);
    file = NULL;

    /* Cleanup */
    gpgme_key_release(key);

    /* UI */
    g_idle_add((GSourceFunc) lock_window_sign_file_on_completed, window);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for file signing and is supposed to be called via g_idle_add().
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_window_sign_file_on_completed(LockWindow *window)
{
    AdwToast *toast;

    if (!window->file_success) {
        toast = adw_toast_new(_("Signing failed"));
    } else {
        toast = adw_toast_new(_("Files signed"));
    }

    adw_toast_set_timeout(toast, 3);

    lock_window_cryptography_processing(window, false);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**** Verification ****/

/**
 * This function verifies text from the text view of a LockWindow.
 *
 * @param window https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_window_verify_text(LockWindow *window)
{
    gchar *armor = lock_window_text_view_get_text(window);

    gchar *plain = text_verify(armor, &window->signer);
    if (plain == NULL) {
        window->text_success = false;
    } else {
        window->text_success = true;
        lock_window_text_queue_set_text(window, plain);
    }

    /* Cleanup */
    g_free(armor);
    armor = NULL;

    g_free(plain);
    plain = NULL;

    /* UI */
    g_idle_add((GSourceFunc) lock_window_verify_text_on_completed, window);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for text verification and is supposed to be called via g_idle_add().
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_window_verify_text_on_completed(LockWindow *window)
{
    AdwToast *toast;

    if (!window->text_success) {
        toast = adw_toast_new(_("Signature invalid"));
    } else {
        toast =
            adw_toast_new(g_strdup_printf
                          (_("Signature valid - by %s"), window->signer));

        gchar *plain = lock_window_text_queue_get_text(window);
        lock_window_text_view_set_text(window, plain);

        /* Cleanup */
        g_free(plain);
        plain = NULL;
    }

    adw_toast_set_timeout(toast, 3);

    lock_window_cryptography_processing(window, false);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

/**
 * This function verifies the input file of a LockWindow.
 *
 * @param window https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_window_verify_file(LockWindow *window)
{
    while (window->file_output_status == SELECTING)
        sleep(1);

    if (window->file_output_status == ABORTED) {
        lock_window_cryptography_processing(window, false);
        g_thread_exit(0);
    }

    GtkWidget *row = gtk_widget_get_first_child(GTK_WIDGET(window->file_list));
    GFile *file = NULL;
    gchar *input_path = NULL;
    gchar *output_path = NULL;

    window->file_success = true;
    while (row != NULL && window->file_success) {
        file = lock_file_row_get_file(LOCK_FILE_ROW(row));
        input_path = g_file_get_path(file);
        output_path =
            g_strdup_printf("%s/%s",
                            g_file_get_path(window->file_output_directory),
                            g_file_get_basename(file));

        output_path = remove_extension(output_path, "pgp");
        output_path = remove_extension(output_path, "gpg");

        window->file_success =
            file_verify(input_path, output_path, &window->signer);
        g_message("%s: %s (%s)", input_path,
                  (window->signer != NULL) ? window->signer : "",
                  (window->file_success) ? _("valid") : _("invalid"));

        row = gtk_widget_get_next_sibling(row);
    }

    g_free(file);
    file = NULL;

    g_free(input_path);
    input_path = NULL;

    g_free(output_path);
    output_path = NULL;

    /* UI */
    g_idle_add((GSourceFunc) lock_window_verify_file_on_completed, window);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for file verification and is supposed to be called via g_idle_add().
 *
 * @param window https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_window_verify_file_on_completed(LockWindow *window)
{
    AdwToast *toast;

    if (!window->file_success) {
        toast = adw_toast_new(_("At least one invalid signature"));
    } else {
        toast = adw_toast_new(_("Signatures valid"));
    }

    adw_toast_set_timeout(toast, 3);

    lock_window_cryptography_processing(window, false);
    adw_toast_overlay_add_toast(window->toast_overlay, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}
