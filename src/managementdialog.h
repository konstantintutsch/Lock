#ifndef MANAGEMENT_DIALOG_H
#define MANAGEMENT_DIALOG_H

#include <adwaita.h>
#include "window.h"

#define LOCK_TYPE_MANAGEMENT_DIALOG (lock_management_dialog_get_type())

G_DECLARE_FINAL_TYPE(LockManagementDialog, lock_management_dialog, LOCK,
                     MANAGEMENT_DIALOG, AdwDialog);

LockManagementDialog *lock_management_dialog_new(LockWindow * window);

// UI
void lock_management_dialog_show_spinner(LockManagementDialog * dialog,
                                         gboolean spinning);

void lock_management_dialog_refresh(GtkButton * self,
                                    LockManagementDialog * dialog);

LockWindow *lock_management_dialog_get_window(LockManagementDialog * dialog);
void lock_management_dialog_add_toast(LockManagementDialog * dialog,
                                      AdwToast * toast);

// Import
void lock_management_dialog_import(LockManagementDialog * dialog);

// Generation
bool lock_management_dialog_generate_ready(LockManagementDialog * dialog);
void lock_management_dialog_generate(LockManagementDialog * dialog);

#endif                          // MANAGEMENT_DIALOG_H
