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
char *text_encrypt(const char *text, gpgme_key_t key);
char *text_decrypt(const char *text);
char *text_sign(const char *text, gpgme_key_t key,
                gpgme_sig_mode_t signature_mode);
char *text_verify(const char *text, char **signer);

bool file_encrypt(const char *input_path, const char *output_path,
                  gpgme_key_t key);
bool file_decrypt(const char *input_path, const char *output_path);
bool file_sign(const char *input_path, const char *output_path,
               gpgme_key_t key);
bool file_verify(const char *input_path, const char *output_path,
                 char **signer);

#endif                          // CRYPTOGRAPHY_H
