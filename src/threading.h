#ifndef THREADING_H
#define THREADING_H

#include <adwaita.h>
#include "window.h"
#include "selectiondialog.h"
#include "managementdialog.h"
#include "keyrow.h"

/* Encrypt */
void thread_encrypt_text(LockSelectionDialog * self, const char *uid,
                         LockWindow * window);
void thread_encrypt_file(LockSelectionDialog * self, const char *uid,
                         LockWindow * window);

/* Decrypt */
void thread_decrypt_text(GSimpleAction * self, GVariant * parameter,
                         LockWindow * window);
void thread_decrypt_file(GtkButton * self, LockWindow * window);

/* Sign */
void thread_sign_text(LockSelectionDialog * self, const char *fingerprint,
                      LockWindow * window);
void thread_sign_file(LockSelectionDialog * self, const char *fingerprint,
                      LockWindow * window);

/* Verify */
void thread_verify_text(GSimpleAction * self, GVariant * parameter,
                        LockWindow * window);
void thread_verify_file(GtkButton * self, LockWindow * window);

/* Key */
void thread_import_key(LockManagementDialog * dialog);
void thread_generate_key(GtkButton * self, LockManagementDialog * dialog);
void thread_export_key(LockKeyRow * row);
void thread_remove_key(LockKeyRow * row);

#endif                          // THREADING_H
