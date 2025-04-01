#pragma once

#include <openssl/evp.h>
#include <stdint.h>

// set after calling load_peer_public_key, should be used as the authority parameter to the verify function
// to check that data was signed with peer's private key
extern EVP_PKEY* ec_peer_public_key;
// set after calling load_ca_public_key, should be used as the authority parameter to the verify function
// to check that certificate is legit
extern EVP_PKEY* ec_ca_public_key;
// set after calling load_certificate, already encoded as TLV 0xA0
extern uint8_t* certificate;
// length (size) of certificate
extern size_t cert_size;
// set after calling derive_public_key
extern uint8_t* public_key;
// length (size) of public_key
extern size_t pub_key_size;

// pass in 'server_key.bin.', puts a private key somewhere we can retrive later
void load_private_key(const char* filename);

// pass in nothing, retrieve current private key
EVP_PKEY* get_private_key();

// pass in a previously known private key (learned from get_private_key) to set the private key to that value
void set_private_key(EVP_PKEY* key);

// pass in peer's public_key (after derive_public_key call) and pub_key_size, sets ec_peer_public_key
void load_peer_public_key(const uint8_t* peer_key, size_t size);

// pass in 'ca_public_key.bin.', sets ec_ca_public_key
void load_ca_public_key(const char* filename);

// pass in 'server_cert.bin.', sets certificate and cert_size
void load_certificate(const char* filename);

// pass nothing, generate privkey that can be obtained from get_private_key
void generate_private_key();

// pass nothing, using loaded private key generate public key and store in public_key
void derive_public_key();

// pass nothing, after load_peer_public_key and generate_private_key, call to obtain shared Diffie-Hellman secret
void derive_secret();

// pass salt, which is Client-Hello with the Server-Hello appended right after, also pass salt's length
// obtain the two keys: 1) info ENC for encryption 2) info MAC for authentication
void derive_keys(const uint8_t* salt, size_t size);

// pass buffer to store signature, data to calculate signature, size of data, returns size of signature
size_t sign(uint8_t* signature, const uint8_t* data, size_t size);

// pass signature to verify and its size, data that signature was calculated on and its size, and the public
// key to use when verifying (either ec_ca_public_key or ec_peer_public_key), ret 1 if valid 0 if not valid, ? if error
int verify(const uint8_t* signature, size_t sig_size, const uint8_t* data,
           size_t size, EVP_PKEY* authority);

// write size number of random bytes into buf
void generate_nonce(uint8_t* buf, size_t size); 

// call AFTER deriving encryption key, pass IV buffer, ciphertext buffer, plaintext to encrypt and plaintext size
// get back size of ciphertext, could be up to 16B greater than plaintext
size_t encrypt_data(uint8_t* iv, uint8_t* cipher, const uint8_t* data, size_t size);

// pass buffer for plaintext to be put in, ciphertext buffer and its size, and IV buffer, ret size of plaintext
size_t decrypt_cipher(uint8_t* data, const uint8_t* cipher, size_t size, const uint8_t* iv);

// using derived MAC key, pass buffer to write digest into, buffer to read data from, and size of data to read
void hmac(uint8_t* digest, const uint8_t* data, size_t size);
