#ifndef CRYPTOGRAPHY_H
#define CRYPTOGRAPHY_H

#include <adwaita.h>

#include <gpgme.h>
#include <stdbool.h>

typedef enum {
    ENCRYPT = 1 << 0,
    DECRYPT = 1 << 1,
    SIGN = 1 << 2,
    VERIFY = 1 << 3
} cryptography_flags;

// Keys
gpgme_key_t key_get(const char *fingerprint);
gpgme_key_t key_search(const char *userid);
bool key_generate(const char *userid, const char *sign_algorithm,
                  const char *encrypt_algorithm,
                  const unsigned long int expire);
bool key_import(const char *path);
bool key_export(const char *path, const char *fingerprint);
bool key_expire(const char *fingerprint, const unsigned long int expire);
bool key_remove(const char *fingerprint);

/* Operations */
char *process_text(const char *text, cryptography_flags flags, gpgme_key_t key,
                   gpgme_sig_mode_t signature_mode, gchar ** signer);
bool process_file(const char *input_path, const char *output_path,
                  cryptography_flags flags, gpgme_key_t key, gchar ** signer);

#endif                          // CRYPTOGRAPHY_H
