#ifndef ENCRYPTION_DIALOG_H
#define ENCRYPTION_DIALOG_H

#include <adwaita.h>
#include "window.h"

#define LOCK_TYPE_ENCRYPTION_DIALOG (lock_encryption_dialog_get_type())

G_DECLARE_FINAL_TYPE(LockEncryptionDialog, lock_encryption_dialog, LOCK,
                     ENCRYPTION_DIALOG, AdwDialog);

LockEncryptionDialog *lock_encryption_dialog_new(const gchar * title,
                                                 const gchar * placeholder_text,
                                                 GtkInputPurpose purpose);

const gchar *lock_encryption_dialog_get_text(LockEncryptionDialog * dialog);

#endif                          // ENCRYPTION_DIALOG_H
