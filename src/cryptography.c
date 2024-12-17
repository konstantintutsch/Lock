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

/**** General ****/

/**
 * This function initializes a GnuPG Made Easy context.
 *
 * @param context GnuPG Made Easy context to be initialized
 *
 * @return Success
 */
bool context_initialize(gpgme_ctx_t *context)
{
    gpgme_error_t error;

    error = gpgme_new(context);
    if (error) {
        g_warning(_("Failed to create new GPGME context: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_set_protocol(*context, GPGME_PROTOCOL_OpenPGP);
    if (error) {
        g_warning(_("Failed to set protocol of GPGME context to OpenPGP: %s"),
                  gpgme_strerror(error));

        gpgme_release(*context);
        goto cleanup;
    }

    error = gpgme_set_pinentry_mode(*context, GPGME_PINENTRY_MODE_ASK);
    if (error) {
        g_warning(_("Failed to set pinentry mode of GPGME context to ask: %s"),
                  gpgme_strerror(error));

        gpgme_release(*context);
        // goto cleanup;
        // Necessary when more steps are added behind this call.
    }

 cleanup:
    return (error) ? false : true;
}

/**
 * This function extracts data as text from GnuPG Made Easy data.
 *
 * @param data gpgme_data_t to extract text from
 *
 * @return Text. Owned by caller
 */
char *text_extract(gpgme_data_t data)
{
    size_t length;
    char *buffer = gpgme_data_release_and_get_mem(data, &length);

    char *string = malloc(length + 1);
    memcpy(string, buffer, length);
    string[length] = '\0';

    gpgme_free(buffer);
    buffer = NULL;

    return string;
}

/**
 * This function extracts raw data from GnuPG Made Easy data.
 *
 * @param data gpgme_data_t to extract from
 * @param path Path to write extracted data to
 *
 * @return Success
 */
bool raw_extract(gpgme_data_t data, const char *path)
{
    bool status = true;

    size_t length;
    void *buffer = gpgme_data_release_and_get_mem(data, &length);

    void *raw = malloc(length);
    memcpy(raw, buffer, length);

    gpgme_free(buffer);
    buffer = NULL;

    FILE *file;

    file = fopen(path, "w");
    if (file == NULL) {
        g_warning(_("Failed to open file for writing: %s"), strerror(errno));

        status = false;
        goto cleanup;
    }

    size_t written = fwrite(raw, length, 1, file);
    if (written < 1) {
        g_warning(_("Failed to write to file: %s"), strerror(errno));

        status = false;
        // goto cleanup_file;
        // Necessary when more steps are added behind this call.
    }
//cleanup_file:
    fclose(file);

 cleanup:
    free(raw);
    raw = NULL;

    return status;
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
    gpgme_error_t error;

    gpgme_key_t key;

    if (!context_initialize(&context))
        return NULL;

    error = gpgme_get_key(context, fingerprint, &key, 0);
    if (error) {
        g_warning(_("Failed to obtain key by fingerprint: %s"),
                  gpgme_strerror(error));

        // goto cleanup;
        // Necessary when more steps are added behind this call.
    }
//cleanup:
    gpgme_release(context);

    return (error) ? NULL : key;
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
    gpgme_error_t error;

    gpgme_key_t key;

    if (!context_initialize(&context))
        return NULL;

    error = gpgme_op_keylist_start(context, userid, 0);
    while (!error) {
        error = gpgme_op_keylist_next(context, &key);

        if (error)
            break;

        if (strstr(key->uids->uid, userid) != NULL)
            break;
    }
    if (error) {
        g_warning("%s: %s", _("Failed to find key matching User ID"),
                  gpgme_strerror(error));

        gpgme_key_release(key);

        // goto cleanup;
        // Necessary when more steps are added behind this call.
    }
//cleanup:
    gpgme_release(context);

    return (error) ? NULL : key;
}

/**
 * This function generates a new GPG keypair.
 *
 * @param userid User ID of the new keypair
 * @param sign_algorithm Algorithm of the signing key of the new keypair
 * @param encrypt_algorithm Algorithm of the encryption subkey of the new keypair
 * @param expire Expiry date in seconds from now of the new key pair
 *
 * @return Success
 */
bool key_generate(const char *userid, const char *sign_algorithm,
                  const char *encrypt_algorithm, const unsigned long int expire)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    if (!context_initialize(&context))
        return false;

    unsigned int flags = 0;
    if (expire == 0)
        flags = GPGME_CREATE_NOEXPIRE;

    error =
        gpgme_op_createkey(context, userid, sign_algorithm, 0, expire, NULL,
                           GPGME_CREATE_SIGN | flags);
    if (error) {
        g_warning(_("Failed to generate new GPG key for signing: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    gpgme_key_t key = key_search(userid);

    error =
        gpgme_op_createsubkey(context, key, encrypt_algorithm, 0, expire,
                              GPGME_CREATE_ENCR | flags);
    if (error) {
        g_warning(_("Failed to generate new GPG subkey for encryption: %s"),
                  gpgme_strerror(error));

        error =
            gpgme_op_delete_ext(context, key,
                                GPGME_DELETE_ALLOW_SECRET | GPGME_DELETE_FORCE);

        if (error) {
            g_warning(_("Failed to delete unfinished, generated key: %s"),
                      gpgme_strerror(error));

            // goto cleanup_key;
            // Necessary when more steps are added behind this call.
        }
        // goto cleanup_key;
        // Necessary when more steps are added behind this call.
    }
//cleanup_key:
    gpgme_key_release(key);

 cleanup:
    gpgme_release(context);

    return (error) ? false : true;
}

/**
 * This function imports keys from a file.
 *
 * @param path Path of the file to import
 *
 * @return Success
 */
bool key_import(const char *path)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t key_data;

    if (!context_initialize(&context))
        return false;

    error = gpgme_data_new_from_file(&key_data, path, 1);
    if (error) {
        g_warning(_("Failed to load GPGME key data from file: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_op_import(context, key_data);
    if (error) {
        g_warning(_("Failed to import GPG key from file: %s"),
                  gpgme_strerror(error));

        // goto cleanup_key_data;
        // Necessary when more steps are added behind this call.
    }
    //cleanup_key_data:
    gpgme_data_release(key_data);

 cleanup:
    gpgme_release(context);

    return (error) ? false : true;
}

/**
 * This function exports a key to a file.
 *
 * @param path Path of the file to export to
 * @param fingerprint Fingerprint of the key to export
 *
 * @return Success
 */
bool key_export(const char *path, const char *fingerprint)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t key_data;

    if (!context_initialize(&context))
        return false;

    gpgme_set_armor(context, 1);

    error = gpgme_data_new(&key_data);
    if (error) {
        g_warning(_("Failed to create GPGME key data in memory: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_op_export(context, fingerprint, 0, key_data);
    if (error) {
        g_warning(_("Failed to export GPG key(s) to file: %s"),
                  gpgme_strerror(error));

        goto cleanup_key_data;
    }

    if (raw_extract(key_data, path)) {
        goto cleanup;
    }
    // else
    // {
    //     goto cleanup_key_data;
    // }
    //
    // Necessary when more steps are added behind this call.

 cleanup_key_data:
    gpgme_data_release(key_data);

 cleanup:
    gpgme_release(context);

    return (error) ? false : true;
}

/**
 * This function updates the expiry date of a key.
 *
 * @param fingerprint Fingerprint of the key to edit
 * @param expire Time in seconds from now to set the expiry date of the key to
 *
 * @return Success
 */
bool key_expire(const char *fingerprint, const unsigned long int expire)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_key_t key;

    if (!context_initialize(&context))
        return false;

    error = gpgme_get_key(context, fingerprint, &key, 0);
    if (error) {
        g_warning(_("Failed to get GPG key for expiry date update: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_op_setexpire(context, key, expire, NULL, 0);
    if (error) {
        g_warning(_("Failed to update the expiry date of a key: %s"),
                  gpgme_strerror(error));

        // goto cleanup_key;
        // Necessary when more steps are added behind this call.
    }
    //cleanup_key:
    gpgme_key_release(key);

 cleanup:
    gpgme_release(context);

    return (error) ? false : true;
}

/**
 * This function removes a key.
 *
 * @param fingerprint Fingerprint of the key to remove
 *
 * @return Success
 */
bool key_remove(const char *fingerprint)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_key_t key;

    if (!context_initialize(&context))
        return false;

    error = gpgme_get_key(context, fingerprint, &key, 0);
    if (error) {
        g_warning(_("Failed to get GPG key for removal: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_op_delete(context, key, 1);
    if (error) {
        g_warning(_("Failed to remove GPG key: %s"), gpgme_strerror(error));

        // goto cleanup_key;
        // Necessary when more steps are added behind this call.
    }
    //cleanup_key:
    gpgme_key_release(key);

 cleanup:
    gpgme_release(context);

    return (error) ? false : true;

}

/**** Operations ****/

/**
 * This function encrypts text.
 *
 * @param text Text to encrypt
 * @param key Key to encrypt text with
 *
 * @return Encrypted text in an OpenPGP ASCII armor. Owned by caller
 */
char *text_encrypt(const char *text, gpgme_key_t key)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t input;
    gpgme_data_t output;

    if (!context_initialize(&context))
        return NULL;

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&input, text, strlen(text), 1);
    if (error) {
        g_warning(_("Failed to create new GPGME input data from string: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_data_new(&output);
    if (error) {
        g_warning(_("Failed to create new GPGME output data in memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_op_encrypt(context, (gpgme_key_t[]) {
                             key, NULL}
                             , GPGME_ENCRYPT_ALWAYS_TRUST, input, output);
    if (error) {
        g_warning(_("Failed to encrypt GPGME data from memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        // goto cleanup_input;
        // Necessary when more steps are added behind this call.
    }

 cleanup_input:
    gpgme_data_release(input);

 cleanup:
    gpgme_release(context);

    return (error) ? NULL : text_extract(output);
}

/**
 * This function decrypts text.
 *
 * @param text Text to decrypt
 *
 * @return Decrypted text. Owned by caller
 */
char *text_decrypt(const char *text)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t input;
    gpgme_data_t output;

    if (!context_initialize(&context))
        return NULL;

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&input, text, strlen(text), 1);
    if (error) {
        g_warning(_("Failed to create new GPGME input data from string: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_data_new(&output);
    if (error) {
        g_warning(_("Failed to create new GPGME output data in memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_op_decrypt(context, input, output);
    if (error) {
        g_warning(_("Failed to decrypt GPGME data from memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        // goto cleanup_input;
        // Necessary when more steps are added behind this call.
    }

 cleanup_input:
    gpgme_data_release(input);

 cleanup:
    gpgme_release(context);

    return (error) ? NULL : text_extract(output);
}

/**
 * This function signs text.
 *
 * @param text Text to sign
 * @param key Key to sign text with
 * @param signature_mode Signature mode
 *
 * @return Signed text in an OpenPGP ASCII armor. Owned by caller
 */
char *text_sign(const char *text, gpgme_key_t key,
                gpgme_sig_mode_t signature_mode)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t input;
    gpgme_data_t output;

    if (!context_initialize(&context))
        return NULL;

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&input, text, strlen(text), 1);
    if (error) {
        g_warning(_("Failed to create new GPGME input data from string: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_data_new(&output);
    if (error) {
        g_warning(_("Failed to create new GPGME output data in memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_signers_add(context, key);
    if (error) {
        g_warning(_("Failed to add signing key to GPGME context: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_op_sign(context, input, output, signature_mode);
    if (error) {
        g_warning(_("Failed to sign GPGME data from memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        // goto cleanup_input;
        // Necessary when more steps are added behind this call.
    }

 cleanup_input:
    gpgme_data_release(input);

 cleanup:
    gpgme_release(context);

    return (error) ? NULL : text_extract(output);
}

/**
 * This function verifies text signatures.
 *
 * @param text Signature text to verify
 * @param signer Write location for signer from signature
 *
 * @return Extracted text from signature. Owned by caller
 */
char *text_verify(const char *text, char **signer)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t input;
    gpgme_data_t output;

    if (!context_initialize(&context))
        return NULL;

    gpgme_set_armor(context, 1);

    error = gpgme_data_new_from_mem(&input, text, strlen(text), 1);
    if (error) {
        g_warning(_("Failed to create new GPGME input data from string: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_data_new(&output);
    if (error) {
        g_warning(_("Failed to create new GPGME output data in memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_op_verify(context, input, NULL, output);
    if (error) {
        g_warning("%s: %s", _("Failed to verify GPGME data from memory"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    gpgme_verify_result_t verify_result = gpgme_op_verify_result(context);

    if (!(verify_result->signatures->summary & GPGME_SIGSUM_VALID)) {
        gpgme_data_release(output);
        goto cleanup_input;
    }

    *signer =
        g_strdup((verify_result->signatures->key !=
                  NULL) ? verify_result->signatures->key->uids->
                 uid : verify_result->signatures->fpr);

 cleanup_input:
    gpgme_data_release(input);

 cleanup:
    gpgme_release(context);

    return (error) ? NULL : text_extract(output);
}

/**
 * This function encrypts a file.
 *
 * @param input_path Path of the file to encrypt
 * @param output_path Path to write the encrypted file to
 * @param key Key to encrypt for
 *
 * @return Success
 */
bool file_encrypt(const char *input_path, const char *output_path,
                  gpgme_key_t key)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t input;
    gpgme_data_t output;

    if (!context_initialize(&context))
        return false;

    /* Overwriting */
    FILE *file = fopen(output_path, "r");
    if (file != NULL) {
        fclose(file);
        remove(output_path);
        g_message(_("Removed %s to prepare overwriting"), output_path);
    }
    // TODO: Do not copy file data to memory once GPGME supports this behavior
    error = gpgme_data_new_from_file(&input, input_path, 1);
    if (error) {
        g_warning(_("Failed to create new GPGME input data from file: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }
    // TODO: Set input file name once GPGME 1.24.0 is released: gpgme_op_encrypt will be able to read input data directly from files
    // error = gpgme_data_set_file_name(input, input_path);
    // if (error) {
    //     g_warning(_("Failed to set file name of GPGME input data: %s"),
    //               gpgme_strerror(error));
    //
    //     gpgme_data_release(output);
    //     goto cleanup_input;
    // }

    error = gpgme_data_new(&output);
    if (error) {
        g_warning(_("Failed to create new GPGME output data in memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_data_set_file_name(output, output_path);
    if (error) {
        g_warning(_("Failed to set file name of GPGME output data: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_op_encrypt(context, (gpgme_key_t[]) {
                             key, NULL}
                             , GPGME_ENCRYPT_ALWAYS_TRUST, input, output);
    if (error) {
        g_warning(_("Failed to encrypt GPGME data from file: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        // goto cleanup_input;
        // Necessary when more steps are added behind this call.
    }

 cleanup_input:
    gpgme_data_release(input);

 cleanup:
    gpgme_release(context);

    return (error) ? false : true;
}

/**
 * This function decrypts a file.
 *
 * @param input_path Path of the file to decrypt
 * @param output_path Path to write the decrypted file to
 *
 * @return Success
 */
bool file_decrypt(const char *input_path, const char *output_path)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t input;
    gpgme_data_t output;

    if (!context_initialize(&context))
        return false;

    // TODO: Add once GPGME 1.24.0 is released: gpgme_op_decrypt will support writing directly files and therefore fail if the file already exists
    /* Overwriting */
    // FILE *file = fopen(output_path, "r");
    // if (file != NULL) {
    //     fclose(file);
    //     remove(output_path);
    //     g_message(_("Removed %s to prepare overwriting"), output_path);
    // }

    // TODO: Do not copy file data to memory once GPGME supports this behavior
    error = gpgme_data_new_from_file(&input, input_path, 1);
    if (error) {
        g_warning(_("Failed to create new GPGME input data from file: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_data_set_file_name(input, input_path);
    if (error) {
        g_warning(_("Failed to set file name of GPGME input data: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_data_new(&output);
    if (error) {
        g_warning(_("Failed to create new GPGME output data in memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }
    // TODO: Add once GPGME 1.24.0 is released: gpgme_op_decrypt will be able to write output data directly to files
    // error = gpgme_data_set_file_name(output, output_path);
    // if (error) {
    //     g_warning(_("Failed to set file name of GPGME output data: %s"),
    //               gpgme_strerror(error));
    //
    //     gpgme_data_release(output);
    //     goto cleanup_input;
    // }

    error = gpgme_op_decrypt(context, input, output);
    if (error) {
        g_warning(_("Failed to decrypt GPGME data from file: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }
    // TODO: Do not manually write to files once GPGME 1.24.0 is released: gpgme_op_decrypt will be able to write output data directly to files
    if (!raw_extract(output, output_path)) {
        gpgme_data_release(output);
        // goto cleanup_input;
        // Necessary when more steps are added behind this call.
    }

 cleanup_input:
    gpgme_data_release(input);

 cleanup:
    gpgme_release(context);

    return (error) ? false : true;
}

/**
 * This function signs a file.
 *
 * @param input_path Path of the file to sign
 * @param output_path Path to write the signed file to
 * @param key Key to sign with
 *
 * @return Success
 */
bool file_sign(const char *input_path, const char *output_path, gpgme_key_t key)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t input;
    gpgme_data_t output;

    if (!context_initialize(&context))
        return false;

    /* Overwriting */
    FILE *file = fopen(output_path, "r");
    if (file != NULL) {
        fclose(file);
        remove(output_path);
        g_message(_("Removed %s to prepare overwriting"), output_path);
    }
    // TODO: Do not copy file data to memory once GPGME supports this behavior
    error = gpgme_data_new_from_file(&input, input_path, 1);
    if (error) {
        g_warning(_("Failed to create new GPGME input data from file: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }
    // TODO: Set input file name once GPGME 1.24.0 is released: gpgme_op_sign will be able to read input data directly from files
    // error = gpgme_data_set_file_name(input, input_path);
    // if (error) {
    //     g_warning(_("Failed to set file name of GPGME input data: %s"),
    //               gpgme_strerror(error));
    //
    //     gpgme_data_release(output);
    //     goto cleanup_input;
    // }

    error = gpgme_data_new(&output);
    if (error) {
        g_warning(_("Failed to create new GPGME output data in memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_data_set_file_name(output, output_path);
    if (error) {
        g_warning(_("Failed to set file name of GPGME output data: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_signers_add(context, key);
    if (error) {
        g_warning(_("Failed to add signing key to GPGME context: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_op_sign(context, input, output, GPGME_SIG_MODE_NORMAL);
    if (error) {
        g_warning(_("Failed to sign GPGME data from file: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        // goto cleanup_input;
        // Necessary when more steps are added behind this call.
    }

 cleanup_input:
    gpgme_data_release(input);

 cleanup:
    gpgme_release(context);

    return (error) ? false : true;
}

/**
 * This function verifies a file.
 *
 * @param input_path Path of the file to verifies
 * @param output_path Path to write the file extracted from the signature to
 * @param signer Write location for signer from signature
 *
 * @return Success
 */
bool file_verify(const char *input_path, const char *output_path, char **signer)
{
    gpgme_ctx_t context;
    gpgme_error_t error;

    gpgme_data_t input;
    gpgme_data_t output;

    if (!context_initialize(&context))
        return false;

    // TODO: Add once GPGME 1.24.0 is released: gpgme_op_verify will support writing directly files and therefore fail if the file already exists
    /* Overwriting */
    // FILE *file = fopen(output_path, "r");
    // if (file != NULL) {
    //     fclose(file);
    //     remove(output_path);
    //     g_message(_("Removed %s to prepare overwriting"), output_path);
    // }

    // TODO: Do not copy file data to memory once GPGME supports this behavior
    error = gpgme_data_new_from_file(&input, input_path, 1);
    if (error) {
        g_warning(_("Failed to create new GPGME input data from file: %s"),
                  gpgme_strerror(error));

        goto cleanup;
    }

    error = gpgme_data_set_file_name(input, input_path);
    if (error) {
        g_warning(_("Failed to set file name of GPGME input data: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    error = gpgme_data_new(&output);
    if (error) {
        g_warning(_("Failed to create new GPGME output data in memory: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }
    // TODO: Add once GPGME 1.24.0 is released: gpgme_op_verify will be able to write output data directly to files
    // error = gpgme_data_set_file_name(output, output_path);
    // if (error) {
    //     g_warning(_("Failed to set file name of GPGME output data: %s"),
    //               gpgme_strerror(error));
    //
    //     gpgme_data_release(output);
    //     goto cleanup_input;
    // }

    error = gpgme_op_verify(context, input, NULL, output);
    if (error) {
        g_warning(_("Failed to verify GPGME data from file: %s"),
                  gpgme_strerror(error));

        gpgme_data_release(output);
        goto cleanup_input;
    }

    gpgme_verify_result_t verify_result = gpgme_op_verify_result(context);

    if (!(verify_result->signatures->summary & GPGME_SIGSUM_VALID)) {
        gpgme_data_release(output);
        goto cleanup_input;
    }

    *signer =
        g_strdup((verify_result->signatures->key !=
                  NULL) ? verify_result->signatures->key->uids->
                 uid : verify_result->signatures->fpr);

    // TODO: Do not manually write to files once GPGME 1.24.0 is released: gpgme_op_verify will be able to write output data directly to files
    if (!raw_extract(output, output_path)) {
        gpgme_data_release(output);
        // goto cleanup_input;
        // Necessary when more steps are added behind this call.
    }

 cleanup_input:
    gpgme_data_release(input);

 cleanup:
    gpgme_release(context);

    return (error) ? false : true;
}
