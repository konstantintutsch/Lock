#ifndef FILE_ROW_H
#define FILE_ROW_H

#include <adwaita.h>

#define LOCK_TYPE_FILE_ROW (lock_file_row_get_type())

G_DECLARE_FINAL_TYPE(LockFileRow, lock_file_row, LOCK, FILE_ROW, AdwActionRow);

/* Row */
LockFileRow *lock_file_row_new(GFile * file);

void lock_file_row_remove(GtkButton * self, LockFileRow * row);

/* File */
GFile *lock_file_row_get_file(LockFileRow * row);

#endif                          // FILE_ROW_H
