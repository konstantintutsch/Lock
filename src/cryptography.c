#include "cryptography.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "config.h"

#include <gpgme.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define HANDLE_ERROR(Return, Error, String, Context, FreeCode) if (Error) \
    { \
        g_warning("%s: %s", String, gpgme_strerror(Error)); \
        \
        FreeCode \
        \
        gpgme_release(Context); \
        \
        return Return; \
    }

/**** Key ****/

/**
 * This function returns the key of a fingerprint.
 *
 * @param fingerprint Fingerprint of the key to get
 *
 * @return Key. Owned by caller
 */
gpgme_key_t key_get(const char *fingerprint)
{
    gpgme_ctx_t context;
    gpgme_key_t key;
    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error,
                 _("Failed to create new GPGME context"), context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 _("Failed to set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_get_key(context, fingerprint, &key, 0);
    HANDLE_ERROR(NULL, error,
                 _("Failed to obtain key by fingerprint"),
                 context, gpgme_key_release(key););

    /* Cleanup */
    gpgme_release(context);

    return key;
}

/**
 * This function returns a key with matching UID.
 *
 * @param userid UID of the key
 *
 * @return Key. Owned by caller
 */
gpgme_key_t key_search(const char *userid)
{
    gpgme_ctx_t context;
    gpgme_key_t key;
    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error,
                 _("Failed to create new GPGME context"), context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 _("Failed to set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_op_keylist_start(context, userid, 0);
    while (!error) {
        error = gpgme_op_keylist_next(context, &key);

        if (error)
            break;

        if (strstr(key->uids->uid, userid) != NULL)
            break;
    }
    HANDLE_ERROR(NULL, error,
                 _("Failed to find key matching User ID"),
                 context, gpgme_key_release(key););

    /* Cleanup */
    gpgme_release(context);

    return key;
}

/**
 * This function generates a new GPG keypair.
 *
 * @param userid User ID of the new keypair
 * @param sign_algorithm Algorithm of the signing key of the new keypair
 * @param encrypt_algorithm Algorithm of the encryption key of the new keypair
 * @param expiry Expiry in seconds of the new keypair
 */
bool key_generate(const char *userid, const char *sign_algorithm,
                  const char *encrypt_algorithm, unsigned long expiry)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(false, error,
                 _("Failed to create new GPGME context"), context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(false, error,
                 _("Failed to set protocol of GPGME context to OpenPGP"),
                 context,);

    unsigned int flags = 0;
    if (expiry == 0)
        flags = GPGME_CREATE_NOEXPIRE;

    error =
        gpgme_op_createkey(context, userid, sign_algorithm, 0, expiry, NULL,
                           GPGME_CREATE_SIGN | flags);
    HANDLE_ERROR(false, error,
                 _("Failed to generate new GPG key for signing"), context,);

    gpgme_key_t key = key_search(userid);

    error =
        gpgme_op_createsubkey(context, key, encrypt_algorithm, 0, expiry,
                              GPGME_CREATE_ENCR | flags);
    HANDLE_ERROR(false, error,
                 _("Failed to generate new GPG key for signing"), context,
                 error =
                 gpgme_op_delete_ext(context, key,
                                     GPGME_DELETE_ALLOW_SECRET |
                                     GPGME_DELETE_FORCE);
                 HANDLE_ERROR(false, error,
                              _("Failed to delete unfinished, generated key"),
                              context,); gpgme_key_release(key););

    gpgme_key_release(key);

    return true;
}

/**
 * This function manages keys.
 *
 * @param path Path of the file to import or export. Can be NULL
 * @param fingerprint Fingerprint of the key to export or remove. Can be NULL
 * @param expires expires parameter for gpgme_op_setexpire. Can be 0
 * @param flags Processing options
 *
 * @return Success
 */
bool key_manage(const char *path, const char *fingerprint,
                const unsigned long expires, key_flags flags)
{
    gpgme_ctx_t context;
    gpgme_data_t keydata;
    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(false, error,
                 _("Failed to create new GPGME context"), context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(false, error,
                 _("Failed to set protocol of GPGME context to OpenPGP"),
                 context,);

    if (flags & IMPORT) {
        error = gpgme_data_new_from_file(&keydata, path, 1);
        HANDLE_ERROR(false, error,
                     _("Failed to load GPGME key data from file"), context,
                     gpgme_data_release(keydata);
            );

        error = gpgme_op_import(context, keydata);
        HANDLE_ERROR(false, error,
                     _("Failed to import GPG key from file"),
                     context, gpgme_data_release(keydata);
            );

        /* Cleanup */
        gpgme_data_release(keydata);
    } else if (flags & EXPORT) {
        gpgme_set_armor(context, 1);

        error = gpgme_data_new(&keydata);
        HANDLE_ERROR(false, error,
                     _("Failed to create GPGME key data in memory"), context,
                     gpgme_data_release(keydata);
            );

        error = gpgme_op_export(context, fingerprint, 0, keydata);
        HANDLE_ERROR(false, error,
                     _("Failed to export GPG key(s) to file"),
                     context, gpgme_data_release(keydata);
            );

        size_t length;
        char *buffer = gpgme_data_release_and_get_mem(keydata, &length);

        char *armor = malloc(length + 1);
        memcpy(armor, buffer, length);
        armor[length] = '\0';

        FILE *file;

        file = fopen(path, "w");
        if (file == NULL) {
            g_warning(_("Failed to open export file: %s"), strerror(errno));

            /* Cleanup */
            gpgme_release(context);

            gpgme_free(buffer);
            buffer = NULL;

            free(armor);
            armor = NULL;

            return false;
        }

        fwrite(armor, length, 1, file);
        fclose(file);

        /* Cleanup */
        gpgme_free(buffer);
        buffer = NULL;

        free(armor);
        armor = NULL;
    }

    if (flags & EXPIRE) {
        gpgme_key_t key;

        error = gpgme_get_key(context, fingerprint, &key, 0);
        HANDLE_ERROR(false, error,
                     _("Failed to get GPG key for expiry editing"), context,);

        error = gpgme_op_setexpire(context, key, expires, NULL, 0);
        HANDLE_ERROR(false, error, _("Failed to update the expiry time"),
                     context, gpgme_key_release(key);
            );

        /* Cleanup */
        gpgme_key_release(key);
    }

    if (flags & REMOVE) {
        gpgme_key_t key;

        error = gpgme_get_key(context, fingerprint, &key, 0);
        HANDLE_ERROR(false, error,
                     _("Failed to get GPG key for removal"), context,);

        error = gpgme_op_delete(context, key, 1);
        HANDLE_ERROR(false, error,
                     _("Failed to remove GPG key"), context,
                     gpgme_key_release(key);
            );

        /* Cleanup */
        gpgme_key_release(key);
    }

    /* Cleanup */
    gpgme_release(context);

    return true;
}

/**** Operations ****/

/**
 * This function processes text.
 *
 * @param text Text to process
 * @param flags Processing options
 * @param key Key to encrypt for. Can be NULL
 * @param signature_mode Mode of signing operations. Can be 0
 * @param signer Write location for signer from verification. Can be NULL
 *
 * @return Processed text as an OpenPGP ASCII armor. Owned by caller
 */
char *process_text(const char *text, cryptography_flags flags, gpgme_key_t key,
                   gpgme_sig_mode_t signature_mode, gchar **signer)
{
    gpgme_ctx_t context;
    gpgme_data_t input;
    gpgme_data_t output;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(NULL, error,
                 _("Failed to create new GPGME context"), context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 _("Failed to set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_set_pinentry_mode(context, GPGME_PINENTRY_MODE_ASK);
    HANDLE_ERROR(NULL, error,
                 _("Failed to set pinentry mode of GPGME context to ask"),
                 context,);

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&input, text, strlen(text), 1);
    HANDLE_ERROR(NULL, error,
                 _("Failed to create new GPGME input data from string"),
                 context, gpgme_data_release(input););

    error = gpgme_data_new(&output);
    HANDLE_ERROR(NULL, error,
                 _("Failed to create new GPGME output data in memory"),
                 context, gpgme_data_release(input);
                 gpgme_data_release(output););

    if (flags & ENCRYPT) {
        error = gpgme_op_encrypt(context, (gpgme_key_t[]) {
                                 key, NULL}
                                 , GPGME_ENCRYPT_ALWAYS_TRUST, input, output);
        HANDLE_ERROR(NULL, error,
                     _("Failed to encrypt GPGME data from memory"), context,
                     gpgme_data_release(input);
                     gpgme_data_release(output););
    } else if (flags & DECRYPT) {
        error = gpgme_op_decrypt(context, input, output);
        HANDLE_ERROR(NULL, error,
                     _("Failed to decrypt GPGME data from memory"), context,
                     gpgme_data_release(input);
                     gpgme_data_release(output););
    }

    if (flags & SIGN) {
        error = gpgme_signers_add(context, key);
        HANDLE_ERROR(NULL, error,
                     _("Failed to add signing key to GPGME context"), context,
                     gpgme_data_release(input);
                     gpgme_data_release(output);
            );

        error = gpgme_op_sign(context, input, output, signature_mode);
        HANDLE_ERROR(NULL, error,
                     _("Failed to sign GPGME data from memory"),
                     context, gpgme_data_release(input);
                     gpgme_data_release(output););
    } else if (flags & VERIFY) {
        error = gpgme_op_verify(context, input, NULL, output);
        HANDLE_ERROR(NULL, error,
                     _("Failed to verify GPGME data from memory"), context,
                     gpgme_data_release(input);
                     gpgme_data_release(output););

        gpgme_verify_result_t verify_result = gpgme_op_verify_result(context);

        if (!(verify_result->signatures->summary & GPGME_SIGSUM_VALID)) {
            /* Cleanup */
            gpgme_release(context);
            gpgme_data_release(input);
            gpgme_data_release(output);

            return NULL;
        }

        *signer =
            g_strdup((verify_result->signatures->key !=
                      NULL) ? verify_result->signatures->key->
                     uids->uid : (strlen(verify_result->signatures->fpr) >
                                  0) ? verify_result->signatures->
                     fpr : _("Unknown"));
    }

    size_t length;
    char *buffer = gpgme_data_release_and_get_mem(output, &length);

    char *string = malloc(length + 1);
    memcpy(string, buffer, length);
    string[length] = '\0';

    /* Cleanup */
    gpgme_release(context);
    gpgme_data_release(input);

    gpgme_free(buffer);
    buffer = NULL;

    return string;
}

/**
 * This function processes a file.
 *
 * @param input_path Path to the file to process
 * @param output_path Path to write the processed file to
 * @param flags Processing options
 * @param key Key to encrypt for. Can be NULL
 * @param signer Write location for signer from verification. Can be NULL
 *
 * @return Success
 */
bool process_file(const char *input_path, const char *output_path,
                  cryptography_flags flags, gpgme_key_t key, gchar **signer)
{
    // TODO: Remove flag condition once GPGME 1.24.0 is release: gpgme_op_decrypt and gpgme_op_verify will support writing directly files
    if (flags & ENCRYPT || flags & SIGN) {
        /* Overwriting */
        FILE *file = fopen(output_path, "r");
        if (file != NULL) {
            fclose(file);
            remove(output_path);
            g_message(_("Removed %s to prepare overwriting"), output_path);
        }
    }

    gpgme_ctx_t context;
    gpgme_data_t input;
    gpgme_data_t output;

    gpgme_error_t error;

    error = gpgme_new(&context);
    HANDLE_ERROR(false, error,
                 _("Failed to create new GPGME context"), context,);

    error = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);
    HANDLE_ERROR(NULL, error,
                 _("Failed to set protocol of GPGME context to OpenPGP"),
                 context,);

    error = gpgme_set_pinentry_mode(context, GPGME_PINENTRY_MODE_ASK);
    HANDLE_ERROR(NULL, error,
                 _("Failed to set pinentry mode of GPGME context to ask"),
                 context,);

    // TODO: Do not copy file data to memory once GPGME supports this behavior
    error = gpgme_data_new_from_file(&input, input_path, 1);
    HANDLE_ERROR(false, error,
                 _("Failed to create new GPGME input data from file"), context,
                 gpgme_data_release(input););

    // TODO: Always set input file name once GPGME 1.24.0 is released: gpgme_op_encrypt and gpgme_op_sign will be able to read input data directly from files
    if (flags & DECRYPT || flags & VERIFY) {
        error = gpgme_data_set_file_name(input, input_path);
        HANDLE_ERROR(false, error,
                     _("Failed to set file name of GPGME input data"), context,
                     gpgme_data_release(input);
                     gpgme_data_release(output););
    }

    error = gpgme_data_new(&output);
    HANDLE_ERROR(false, error,
                 _("Failed to create new GPGME output data in memory"),
                 context, gpgme_data_release(input);
                 gpgme_data_release(output););

    // TODO: Always set output file name once GPGME 1.24.0 is released: gpgme_op_decrypt and gpgme_op_verify will be able to write output data directly to files
    if (flags & ENCRYPT || flags & SIGN) {
        error = gpgme_data_set_file_name(output, output_path);
        HANDLE_ERROR(false, error,
                     _("Failed to set file name of GPGME output data"),
                     context, gpgme_data_release(input);
                     gpgme_data_release(output););
    }

    if (flags & ENCRYPT) {
        error = gpgme_op_encrypt(context, (gpgme_key_t[]) {
                                 key, NULL}
                                 , GPGME_ENCRYPT_ALWAYS_TRUST, input, output);
        HANDLE_ERROR(false, error,
                     _("Failed to encrypt GPGME data from file"), context,
                     gpgme_data_release(input); gpgme_data_release(output););
    } else if (flags & DECRYPT) {
        error = gpgme_op_decrypt(context, input, output);
        HANDLE_ERROR(false, error,
                     _("Failed to decrypt GPGME data from file"), context,
                     gpgme_data_release(input); gpgme_data_release(output););
    }

    if (flags & SIGN) {
        error = gpgme_op_sign(context, input, output, GPGME_SIG_MODE_NORMAL);
        HANDLE_ERROR(false, error,
                     _("Failed to sign GPGME data from file"),
                     context, gpgme_data_release(input);
                     gpgme_data_release(output););
    } else if (flags & VERIFY) {
        error = gpgme_op_verify(context, input, NULL, output);
        HANDLE_ERROR(false, error,
                     _("Failed to verify GPGME data from file"),
                     context, gpgme_data_release(input);
                     gpgme_data_release(output););

        gpgme_verify_result_t verify_result = gpgme_op_verify_result(context);

        if (!(verify_result->signatures->summary & GPGME_SIGSUM_VALID)) {
            /* Cleanup */
            gpgme_release(context);
            gpgme_data_release(input);
            gpgme_data_release(output);

            return NULL;
        }

        *signer =
            g_strdup((verify_result->signatures->key !=
                      NULL) ? verify_result->signatures->key->
                     uids->uid : (strlen(verify_result->signatures->fpr) >
                                  0) ? verify_result->signatures->
                     fpr : _("Unknown"));
    }
    // TODO: Do not manually write to files once GPGME 1.24.0 is released: gpgme_op_decrypt and gpgme_op_verify will be able to write output data directly to files
    if (flags & DECRYPT || flags & VERIFY) {
        size_t length;
        void *buffer = gpgme_data_release_and_get_mem(output, &length);

        void *data = malloc(length + 1);
        memcpy(data, buffer, length);

        FILE *file;

        file = fopen(output_path, "w");
        if (file == NULL) {
            g_warning(_("Failed to open output file: %s"), strerror(errno));

            /* Cleanup */
            gpgme_release(context);
            gpgme_data_release(input);

            gpgme_free(buffer);
            buffer = NULL;

            free(data);
            data = NULL;

            return false;
        }

        fwrite(data, length, 1, file);
        fclose(file);

        /* Cleanup */
        gpgme_free(buffer);
        buffer = NULL;

        free(data);
        data = NULL;
    }

    /* Cleanup */
    gpgme_release(context);
    gpgme_data_release(input);
    // TODO: Remove free condition once GPGME 1.24.0 is released: gpgme_op_decrypt and gpgme_op_verify will be able to write output data directly to files
    if (flags & ENCRYPT || flags & SIGN)
        gpgme_data_release(output);

    return true;
}
