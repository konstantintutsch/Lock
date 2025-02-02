#include "text.h"

#include <adwaita.h>

#include <string.h>

/**
 * This function removes a file extension from a file path.
 *
 * @param path Path to remove an extension from
 * @param extension Extension to remove
 *
 * @return New, extensionless path. Owned by caller
 */
gchar *remove_extension(gchar *path, gchar *extension)
{
    extension = g_strdup_printf(".%s", extension);
    size_t length = strlen(path) - strlen(extension);

    if (g_str_has_suffix(path, extension)) {
        path = g_realloc(path, length + 1);
        path[length] = '\0';
    }

    return path;
}
