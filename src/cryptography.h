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

typedef enum {
    IMPORT = 1 << 0,
    EXPORT = 1 << 1,
    REMOVE = 1 << 2,
    EXPIRE = 1 << 3
} key_flags;

// Keys
gpgme_key_t key_get(const char *fingerprint);
gpgme_key_t key_search(const char *userid);
bool key_generate(const char *userid, const char *sign_algorithm,
                  const char *encrypt_algorithm, unsigned long expiry);
bool key_manage(const char *path, const char *fingerprint,
                const unsigned long expires, key_flags flags);

/* Operations */
char *process_text(const char *text, cryptography_flags flags, gpgme_key_t key,
                   gpgme_sig_mode_t signature_mode, gchar ** signer);
bool process_file(const char *input_path, const char *output_path,
                  cryptography_flags flags, gpgme_key_t key, gchar ** signer);

#endif                          // CRYPTOGRAPHY_H
