#include "application.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "config.h"

/**
 * This structure handles data of an application.
 */
struct _LockApplication {
    AdwApplication parent;
};

G_DEFINE_TYPE(LockApplication, lock_application, ADW_TYPE_APPLICATION);

void lock_application_show_shortcuts(GSimpleAction * self,
                                     GVariant * parameter,
                                     LockApplication * app);
void lock_application_show_about(GSimpleAction * self,
                                 GVariant * parameter, LockApplication * app);

/**
 * This function initializes a LockApplication.
 *
 * @param app Application to be initialized
 */
static void lock_application_init(LockApplication *app)
{
    // Register resources
    GResource *resource = g_resource_load(GRESOURCE_FILE, NULL);
    g_resources_register(resource);

    // Load styles
    GtkCssProvider *style = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(style, ROOT_RESOURCE("style.css"));

    // Add actions
    g_autoptr(GSimpleAction) about_action = g_simple_action_new("about", NULL);
    g_signal_connect(about_action, "activate",
                     G_CALLBACK(lock_application_show_about), app);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(about_action));

    /* Shortcuts */
    g_autoptr(GSimpleAction) shortcuts_action =
        g_simple_action_new("shortcuts", NULL);
    g_signal_connect(shortcuts_action, "activate",
                     G_CALLBACK(lock_application_show_shortcuts), app);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(shortcuts_action));
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.shortcuts",
                                          (const gchar *
                                           []) { "<Control>question", NULL });

    // Text
    gtk_application_set_accels_for_action(GTK_APPLICATION(app),
                                          "win.encrypt_text",
                                          (const gchar *[]) { "<Control>e",
                                          NULL
                                          });
    gtk_application_set_accels_for_action(GTK_APPLICATION(app),
                                          "win.decrypt_text",
                                          (const gchar *[]) { "<Control>d",
                                          NULL
                                          });
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "win.sign_text",
                                          (const gchar *[]) { "<Control>s",
                                          NULL
                                          });
    gtk_application_set_accels_for_action(GTK_APPLICATION(app),
                                          "win.verify_text",
                                          (const gchar *
                                           []) { "<Control>ampersand",
                                          NULL
                                          });
}

/**
 * This function activates a LockApplication.
 *
 * @param app Application to be activated
 */
static void lock_application_activate(GApplication *app)
{
    LockWindow *window;

    window = lock_window_new(LOCK_APPLICATION(app));
    gtk_window_present(GTK_WINDOW(window));
}

/**
 * This function opens a new LockApplication.
 *
 * @param self https://docs.gtk.org/gio/signal.Application.open.html
 * @param files https://docs.gtk.org/gio/signal.Application.open.html
 * @param n_files https://docs.gtk.org/gio/signal.Application.open.html
 * @param hint https://docs.gtk.org/gio/signal.Application.open.html
 */
static void lock_application_open(GApplication *self, GFile **files,
                                  int n_files, const char *hint)
{
    (void)files;
    (void)n_files;
    (void)hint;

    GList *windows;
    LockWindow *window;

    windows = gtk_application_get_windows(GTK_APPLICATION(self));
    if (!windows)
        window = lock_window_new(LOCK_APPLICATION(self));
    window = LOCK_WINDOW(windows->data);

    gtk_window_present(GTK_WINDOW(window));
}

/**
 * This function initializes a LockApplication class.
 *
 * @param class Application class to be initialized
 */
static void lock_application_class_init(LockApplicationClass *class)
{
    G_APPLICATION_CLASS(class)->activate = lock_application_activate;
    G_APPLICATION_CLASS(class)->open = lock_application_open;
}

/**
 * This function creates a new LockApplication.
 *
 * @return LockApplication
 */
LockApplication *lock_application_new()
{
    return g_object_new(LOCK_TYPE_APPLICATION, "application-id", PROJECT_ID,
                        G_APPLICATION_DEFAULT_FLAGS, G_APPLICATION_HANDLES_OPEN,
                        NULL);
}

/**
 * This function shows the shortcuts window of an application.
 *
 * @param action https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param app https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
void lock_application_show_shortcuts(GSimpleAction *self,
                                     GVariant *parameter, LockApplication *app)
{
    (void)self;
    (void)parameter;

    GtkBuilder *builder =
        gtk_builder_new_from_resource(UI_RESOURCE("shortcuts.ui"));
    GtkShortcutsWindow *window =
        GTK_SHORTCUTS_WINDOW(gtk_builder_get_object(builder, "shortcuts"));

    LockWindow *active_window =
        LOCK_WINDOW(gtk_application_get_active_window(GTK_APPLICATION(app)));
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(active_window));

    gtk_window_present(GTK_WINDOW(window));

    /* Cleanup */
    g_object_unref(builder);
}

/**
 * This function shows the about dialogue of an application.
 *
 * @param action https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param parameter https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 * @param app https://docs.gtk.org/gio/signal.SimpleAction.activate.html
 */
void lock_application_show_about(GSimpleAction *self,
                                 GVariant *parameter, LockApplication *app)
{
    (void)self;
    (void)parameter;

    LockWindow *active_window =
        LOCK_WINDOW(gtk_application_get_active_window(GTK_APPLICATION(app)));

    AdwAboutDialog *about =
        ADW_ABOUT_DIALOG(adw_about_dialog_new_from_appdata
                         (ROOT_RESOURCE(_PROJECT_ID(".metainfo.xml")),
                          PROJECT_VERSION));

    // Show version suffix
    adw_about_dialog_set_version(about, PROJECT_BUILD);

    // Details
    adw_about_dialog_set_comments(about, _("Process data with GnuPG"));
    adw_about_dialog_add_link(about,
                              C_("Button linking to translations", "Translate"),
                              "https://hosted.weblate.org/engage/Lock/");
    adw_about_dialog_add_link(about,
                              C_("Button linking to source code", "Develop"),
                              "https://github.com/konstantintutsch/Lock");

    // Credits
    const char *developers[] =
        { "Konstantin Tutsch <mail@konstantintutsch.com>", NULL };
    adw_about_dialog_set_developers(about, developers);
    const char *designers[] =
        { "GNOME Design Team https://welcome.gnome.org/team/design/",
        "Konstantin Tutsch <mail@konstantintutsch.com>", NULL
    };
    adw_about_dialog_set_designers(about, designers);
    adw_about_dialog_set_translator_credits(about, _("translator-credits"));
    const char *libraries[] = { "The GNOME Project https://www.gnome.org",
        "The GNU Privacy Guard https://gnupg.org/",
        "GnuPG Made Easy https://gnupg.org/software/gpgme/index.html", NULL
    };
    adw_about_dialog_add_acknowledgement_section(about, _("Dependencies"),
                                                 libraries);

    // Legal
    adw_about_dialog_set_copyright(about, "© 2024 Konstantin Tutsch");

    adw_dialog_present(ADW_DIALOG(about), GTK_WIDGET(active_window));
}
