#ifndef WINDOW_H
#define WINDOW_H

#include <adwaita.h>
#include "application.h"

#define LOCK_TYPE_WINDOW (lock_window_get_type())

G_DECLARE_FINAL_TYPE(LockWindow, lock_window, LOCK, WINDOW,
                     AdwApplicationWindow);

LockWindow *lock_window_new(LockApplication * app);
void lock_window_open(LockWindow * window, GFile * file);

enum dialog_status {
    SELECTING,
    SELECTED,
    ABORTED
};

/* UI */
void lock_window_cryptography_processing(LockWindow * window,
                                         gboolean processing);

/* File */
void lock_window_file_open(LockWindow * window, GFile * file);
void lock_window_file_select_output_directory_dialog_present(LockWindow *
                                                             window);

/* Cryptography */
void lock_window_set_fingerprint(LockWindow * window, const char *fingerprint);

// Encryption
void lock_window_encrypt_text(LockWindow * window);
void lock_window_encrypt_file(LockWindow * window);

// Decryption
void lock_window_decrypt_text(LockWindow * window);
void lock_window_decrypt_file(LockWindow * window);

// Signing
void lock_window_sign_text(LockWindow * window);
void lock_window_sign_file(LockWindow * window);

// Verification
void lock_window_verify_text(LockWindow * window);
void lock_window_verify_file(LockWindow * window);

#endif                          // WINDOW_H
