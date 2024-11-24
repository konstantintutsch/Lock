#ifndef SELECTION_DIALOG_H
#define SELECTION_DIALOG_H

#include <adwaita.h>
#include "window.h"

#define LOCK_TYPE_SELECTION_DIALOG (lock_selection_dialog_get_type())

G_DECLARE_FINAL_TYPE(LockSelectionDialog, lock_selection_dialog, LOCK,
                     SELECTION_DIALOG, AdwDialog);

LockSelectionDialog *lock_selection_dialog_new(gboolean target);

const gchar *lock_selection_dialog_get_text(LockSelectionDialog * dialog);

#endif                          // SELECTION_DIALOG_H
