#include "filerow.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "config.h"

/**
 * This structure handles data of a file row.
 */
struct _LockFileRow {
    AdwActionRow parent;

    GFile *file;

    GtkButton *remove_button;
};

G_DEFINE_TYPE(LockFileRow, lock_file_row, ADW_TYPE_ACTION_ROW);

void lock_file_row_remove(GtkButton * self, LockFileRow * row);

/**
 * This function initializes a LockFileRow.
 *
 * @param row Row to be initialized
 */
void lock_file_row_init(LockFileRow *row)
{
    gtk_widget_init_template(GTK_WIDGET(row));

    g_signal_connect(row->remove_button, "clicked",
                     G_CALLBACK(lock_file_row_remove), row);
}

/**
 * This function initializes a LockFileRow class.
 *
 * @param class Row class to be initialized
 */
static void lock_file_row_class_init(LockFileRowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                                UI_RESOURCE("filerow.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockFileRow,
                                         remove_button);
}

/**
 * This function creates a new LockFileRow.
 *
 * @param file File
 *
 * @return LockFileRow
 */
LockFileRow *lock_file_row_new(GFile *file)
{
    LockFileRow *row = g_object_new(LOCK_TYPE_FILE_ROW, NULL);

    /* TODO: implement g_object_class_install_property() */
    row->file = file;
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row),
                                  g_file_get_basename(row->file));

    return row;
}

/**
 * This function removes the row.
 *
 * @param self Gtk.Button::clicked
 * @param row Gtk.Button::clicked
 */
void lock_file_row_remove(GtkButton *self, LockFileRow *row)
{
    (void)self;
    (void)row;

    gtk_list_box_remove(GTK_LIST_BOX(gtk_widget_get_parent(GTK_WIDGET(row))),
                        GTK_WIDGET(row));
}

/**
 * This function returns the file of the row.
 *
 * @return GFile
 */
GFile *lock_file_row_get_file(LockFileRow *row)
{
    return row->file;
}
