#ifndef KEY_DIALOG_H
#define KEY_DIALOG_H

#include <adwaita.h>
#include "window.h"

#define LOCK_TYPE_KEY_DIALOG (lock_key_dialog_get_type())

G_DECLARE_FINAL_TYPE(LockKeyDialog, lock_key_dialog, LOCK, KEY_DIALOG,
                     AdwDialog);

LockKeyDialog *lock_key_dialog_new(LockWindow * window);

// UI
void lock_key_dialog_show_spinner(LockKeyDialog * dialog, gboolean spinning);

void lock_key_dialog_refresh(GtkButton * self, LockKeyDialog * dialog);

LockWindow *lock_key_dialog_get_window(LockKeyDialog * dialog);
void lock_key_dialog_add_toast(LockKeyDialog * dialog, AdwToast * toast);

// Import
void lock_key_dialog_import(LockKeyDialog * dialog);

// Generation
void lock_key_dialog_generate(LockKeyDialog * dialog);

#endif                          // KEY_DIALOG_H
