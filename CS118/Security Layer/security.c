#include "consts.h"
#include "io.h"
#include "libsecurity.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
1. Send over a 32 byte nonce (generate_nonce)
2. Receive the server’s public key (load_peer_public_key)
3. Send over your public key
    a. generate_private_key
    b. derive_public_key
    c. get_public_key
    d. derive_secret
    e. derive_keys
4. Receive the server’s signature of your nonce (verify, ec_peer_public_key)
    a. Terminate if the signature doesn’t match (Ctrl-C)
5. Send anything to get the IV
6. Receive the IV
7. Send anything to get the ciphertext
8. Receive the ciphertext
9. Decrypt (decrypt_cipher)

Additional rules:
- call (generate_private_key) or (load_private_key then set_private_key) BEFORE anything that needs a private key like:
derive_public_key, derive_secret, derive_keys, sign, encrypt_data, decrypt_cipher, and hmac
- ensure proper order of events (before calling something, write a check that confirms we have the data, keys, etc.
that the call expects)
*/

static int cs_type = 0;
static int handshake_status = 0;
static char* hostname = NULL;
static tlv* client_hello = NULL; //Client hello needs to be stored for client
static tlv* server_hello = NULL; //Client may need to check server_hello -> val
// append client_hello and server_hello for salt

static EVP_PKEY* my_private_key = NULL;
static uint8_t* my_public_key = NULL;
static size_t my_public_key_length = 0;

void init_sec(int type, char* host) {
    init_io();
    cs_type = type;
    hostname = host;
    
}

ssize_t input_sec(uint8_t* buf, size_t max_length) {
    
    if(max_length < 1012){return 0;} //Do nothing if it's not max_payload (okay to do per Piazza)


    uint8_t tmp[1024]; //garbage, dont care abt value, simply used to get size of tlv's
    switch (handshake_status)
    {
    case 0: //Client hasn't sent anything, server hasn't received anything. Client sebds client hello, server sends nothing
        if(cs_type == SERVER){return 0;}
        // printf("Creating client hello\n");
        fprintf(stderr, "STARTING CLIENT HELLO\n");
        tlv* ch = create_tlv(CLIENT_HELLO);

        generate_private_key();
        derive_public_key();
        my_public_key = public_key;

        my_private_key = get_private_key();

        my_public_key_length = pub_key_size;

        tlv* key = create_tlv(PUBLIC_KEY);

        add_val(key, public_key, pub_key_size);
        

        tlv* nn = create_tlv(NONCE);
        uint8_t nonce[NONCE_SIZE];
        generate_nonce(nonce, NONCE_SIZE);
        add_val(nn, nonce, NONCE_SIZE);
        add_tlv(ch, nn);
        add_tlv(ch, key);
        
        // printf("Public key length: %i\n", my_public_key_length);

        uint16_t len = serialize_tlv(buf, ch);

        client_hello = ch;
        handshake_status += 1;

        

        // print_tlv_bytes(buf, len);

        return len;

    
    case 1: //Client has sent client hello, server received client hello. Client sends nothing, server sends server hello
        if(cs_type == CLIENT){return 0;}
        fprintf(stderr, "STARTING SERVER HELLO\n");
        tlv* server_hello_tlv = create_tlv(SERVER_HELLO); //outer tlv we add nonce, certificate, public key, and signature to

        // NONCE
        tlv* nonce_tlv = create_tlv(NONCE);
        uint8_t nonce_buf[NONCE_SIZE];
        generate_nonce(nonce_buf, NONCE_SIZE);
        add_val(nonce_tlv, nonce_buf, NONCE_SIZE);
        add_tlv(server_hello_tlv, nonce_tlv);

        // CERTIFICATE
        tlv* certificate_tlv = create_tlv(CERTIFICATE);
        load_certificate("server_cert.bin");
        add_val(certificate_tlv, certificate, (uint16_t)cert_size);
        add_tlv(server_hello_tlv, certificate_tlv);
        
        // EPHEMERAL
        tlv* public_key_tlv = create_tlv(PUBLIC_KEY);
        generate_private_key();
        derive_public_key();
        my_private_key = get_private_key();
        add_val(public_key_tlv, (uint8_t*)public_key, (uint16_t)pub_key_size);
        add_tlv(server_hello_tlv, public_key_tlv);

        // SIGNATURE
        {
            tlv* signature_tlv = create_tlv(HANDSHAKE_SIGNATURE);

            // COMBINE TLV's INTO SERIALIZED BUFFER
            // ch
            uint16_t ch_ser_len = serialize_tlv(tmp, client_hello); //to get size, could maybe do client_hello->length instead but I had problems with that
            uint8_t* ch_ser = malloc(ch_ser_len);
            serialize_tlv(ch_ser, client_hello);

            // nonce
            uint16_t nonce_ser_len = serialize_tlv(tmp, nonce_tlv);
            uint8_t* nonce_ser = malloc(nonce_ser_len);
            serialize_tlv(nonce_ser, nonce_tlv);

            // cert
            uint16_t cert_ser_len = serialize_tlv(tmp, certificate_tlv);
            uint8_t* cert_ser = malloc(cert_ser_len);
            serialize_tlv(cert_ser, certificate_tlv);

            // pk
            uint16_t pk_ser_len = serialize_tlv(tmp, public_key_tlv);
            uint8_t* pk_ser = malloc(pk_ser_len);
            serialize_tlv(pk_ser, public_key_tlv);

            uint16_t ser_buf_size = ch_ser_len + nonce_ser_len + cert_ser_len + pk_ser_len;
            uint8_t* data_for_signature = malloc((size_t)ser_buf_size); //serialized buf

            size_t offset = 0;
            memcpy(data_for_signature + offset, ch_ser, ch_ser_len);
            offset += ch_ser_len;

            memcpy(data_for_signature + offset, nonce_ser, nonce_ser_len);
            offset += nonce_ser_len;

            memcpy(data_for_signature + offset, cert_ser, cert_ser_len);
            offset += cert_ser_len;

            memcpy(data_for_signature + offset, pk_ser, pk_ser_len);

            free(ch_ser);
            free(nonce_ser);
            free(cert_ser);
            free(pk_ser);
            // END COMBINE TLV's INTO SERIALIZED BUFFER

            load_private_key("server_key.bin");
            uint8_t signature[75]; //upper-bounded buffer size + a couple for metadata (72 might be correct, but with ser_buf_size being passed it probably doesnt matter)
            size_t sig_len = sign(signature, data_for_signature, (size_t)ser_buf_size);
            set_private_key(my_private_key); //reset private key per instructions
            
            add_val(signature_tlv, signature, sig_len);
            add_tlv(server_hello_tlv, signature_tlv);
            free(data_for_signature);
        }
        // END SIGNATURE
        

        uint16_t length = serialize_tlv(buf, server_hello_tlv);
        server_hello = server_hello_tlv;
        handshake_status++; // ready for finished step
        return length;
    
    case 2: //Client has received server hello, server sent server hello. Client sends finished, server sends nothing
        if(cs_type == SERVER){return 0;}

        fprintf(stderr, "STARTING FINISHED MESSAGE\n");

        // Extract certificate from server hello
        tlv* server_certificate_tlv = get_tlv(server_hello, CERTIFICATE);
        if(server_certificate_tlv == NULL) exit(1); // cert fail
        
        // Get DNS name from certificate
        tlv* dns_name_tlv = get_tlv(server_certificate_tlv, DNS_NAME);
        if(dns_name_tlv == NULL) exit(2); // dns fail
        
        // Get the signature from the certificate
        tlv* cert_signature_tlv = get_tlv(server_certificate_tlv, SIGNATURE);
        if(cert_signature_tlv == NULL) exit(3); // signature failure
        
        // Get the public key from the certificate
        tlv* cert_public_key_tlv = get_tlv(server_certificate_tlv, PUBLIC_KEY);
        if(cert_public_key_tlv == NULL) exit(1); // Cert failure
        
        // Get the server's ephemeral public key from server hello
        tlv* server_ephemeral_key_tlv = get_tlv(server_hello, PUBLIC_KEY);
        if(server_ephemeral_key_tlv == NULL) exit(4); // bad transcript
        
        // Get the handshake signature from server hello
        tlv* handshake_signature_tlv = get_tlv(server_hello, HANDSHAKE_SIGNATURE);
        if(handshake_signature_tlv == NULL) exit(4); // SH fail  
        
        // Get nonce
        tlv* nnonce_tlv = get_tlv(server_hello, NONCE);
        if(nnonce_tlv == NULL) exit(6); 

        // VERIFY CERTIFICATE SIG
        uint16_t cert_sig_size = serialize_tlv(tmp, cert_signature_tlv);
        uint8_t* cert_sig = malloc(cert_sig_size);
        serialize_tlv(cert_sig, cert_signature_tlv);
        {// print cert_sig, looks fine here?
            fprintf(stderr, "cert_sig: ");
            for (size_t i = 0; i < cert_sig_size; i++) {
                fprintf(stderr, "%02x ", cert_sig[i]);
            }
            fprintf(stderr, "\n");
            fprintf(stderr, "cert_sig_size: %u \n", cert_sig_size); //73, but signature is 70-72? Possible issue here (or just a couple more for type and length byte)
        }
        
        // instructor endorsed posts have said data is dns_name + cert pub key, so going with that
        uint16_t dns_ser_size = serialize_tlv(tmp, dns_name_tlv);
        uint8_t* dns_ser = malloc(dns_ser_size);
        serialize_tlv(dns_ser, dns_name_tlv);
        
        uint16_t cert_pubkey_ser_size = serialize_tlv(tmp, cert_public_key_tlv);
        uint8_t* cert_pubkey_ser = malloc(cert_pubkey_ser_size);
        serialize_tlv(cert_pubkey_ser, cert_public_key_tlv);
        
        size_t cert_sig_verify_size = (size_t)(dns_ser_size + cert_pubkey_ser_size);
        uint8_t* cert_sig_verify_data = malloc(cert_sig_verify_size);
        memcpy(cert_sig_verify_data, dns_ser, dns_ser_size);
        memcpy(cert_sig_verify_data + dns_ser_size, cert_pubkey_ser, cert_pubkey_ser_size);
        free(dns_ser);
        free(cert_pubkey_ser);
        {// print cert_sig_verify_data, looks fine here?
            fprintf(stderr, "cert_sig_verify_data: ");
            for (size_t i = 0; i < cert_sig_verify_size; i++) {
                fprintf(stderr, "%02x ", cert_sig_verify_data[i]);
            }
            fprintf(stderr, "\n");
            fprintf(stderr, "cert_sig_verify_size: %zu \n", cert_sig_verify_size); //105
        }
        
        load_ca_public_key("ca_public_key.bin"); // sets ec_ca_public_key
        
        // Extract the actual signature value (without TLV header)
        // TLV format: Type (1 byte) + Length (1 byte) + Value (Length bytes)
        uint8_t* actual_sig = cert_sig + 2;  // Skip the type and length bytes
        size_t actual_sig_size = cert_sig_size - 2;  // Subtract type and length bytes
        
        int rc = verify(actual_sig, actual_sig_size, cert_sig_verify_data, cert_sig_verify_size, ec_ca_public_key);
        fprintf(stderr, "CERTIFICATE VERIFICATION RETCODE %d\n", rc); //is -1, there is an error with parameters being sent (no idea how)
        if (rc == 0){exit(1);}
        else if (rc != 1) {fprintf(stderr, "ERROR IN CERTIFICATE VERIFICATION verify() CALL");}
        free(cert_sig); 
        free(cert_sig_verify_data); 
        // END VERIFY CERTIFICATE SIG

        // VERIFY DNS NAME
        // compare argv[1] to received DNS Name
        uint8_t* recvd_dns_name = dns_name_tlv->val; //possibly wrong, val may not actually be dns name
        rc = strcmp((char*)recvd_dns_name, hostname);
        fprintf(stderr, "DNS COMPARE RETCODE %d\n", rc);
        if (rc != 0){exit(2);}
        // END VERIFY DNS NAME

        // VERIFY HANDSHAKE SIG
        uint16_t handshake_sig_ser_size = serialize_tlv(tmp, handshake_signature_tlv);
        uint8_t* handshake_sig_ser = malloc(handshake_sig_ser_size);
        serialize_tlv(handshake_sig_ser, handshake_signature_tlv);
        
        // instructor endorsed: Data is client-hello + nonce + cert + server's public key
        uint16_t data_p1_size = serialize_tlv(tmp, client_hello);
        uint8_t* data_p1 = malloc(data_p1_size);
        serialize_tlv(data_p1, client_hello);


        uint16_t data_p2_size = serialize_tlv(tmp, nnonce_tlv);
        uint8_t* data_p2 = malloc(data_p2_size);
        serialize_tlv(data_p2, nnonce_tlv);


        uint16_t data_p3_size = serialize_tlv(tmp, server_certificate_tlv);
        uint8_t* data_p3 = malloc(data_p3_size);
        serialize_tlv(data_p3, server_certificate_tlv);


        uint16_t data_p4_size = serialize_tlv(tmp, server_ephemeral_key_tlv);
        uint8_t* data_p4 = malloc(data_p4_size);
        serialize_tlv(data_p4, server_ephemeral_key_tlv);


        size_t handshake_sig_verify_size = (size_t)(data_p1_size + data_p2_size + data_p3_size + data_p4_size);
        uint8_t* handshake_sig_verify_data = malloc(handshake_sig_verify_size);


        size_t offset = 0;
        memcpy(handshake_sig_verify_data + offset, data_p1, data_p1_size);
        offset += data_p1_size;
        memcpy(handshake_sig_verify_data + offset, data_p2, data_p2_size);
        offset += data_p2_size;
        memcpy(handshake_sig_verify_data + offset, data_p3, data_p3_size);
        offset += data_p3_size;
        memcpy(handshake_sig_verify_data + offset, data_p4, data_p4_size);


        load_peer_public_key(cert_public_key_tlv->val, cert_public_key_tlv->length);

        //for handshake_sig_ser and handshake_sig_ser_size: skip type and length bytes
        rc = verify(handshake_sig_ser + 2, handshake_sig_ser_size - 2, // handshake sig to verify
            handshake_sig_verify_data, handshake_sig_verify_size, // data sig was calculated on
            ec_peer_public_key); // key we use to do the verification, should be server's public key (my_public_key could be wrong)
        fprintf(stderr, "HANDSHAKE SIG RETCODE %d\n", rc);
        if (rc == 0){exit(3);}
        else if (rc != 1) {fprintf(stderr, "ERROR IN HANDSHAKE SIG VERIFICATION verify() CALL");}

        // print_tlv_bytes(buf,length);

        free(data_p1);
        free(data_p2);
        free(data_p3);
        free(data_p4);
        free(handshake_sig_verify_data);
        free(handshake_sig_ser);
        // END VERIFY HANDSHAKE SIG
        
        // MAC SECRET STUFF
        // Setup for deriving secret
        
        load_peer_public_key(server_ephemeral_key_tlv->val, server_ephemeral_key_tlv->length);

        // uint16_t peer_pk_size = serialize_tlv(tmp, server_ephemeral_key_tlv);
        // uint8_t* peer_pk = malloc(peer_pk_size);
        // serialize_tlv(peer_pk, server_ephemeral_key_tlv);
        
        // load_peer_public_key(peer_pk, (size_t)peer_pk_size); //sets ec_peer_public_key
        set_private_key(my_private_key);
        derive_secret();
        
        // Create salt to derive keys
        // uint16_t ch_ser_len = serialize_tlv(tmp, client_hello); //to get size
        // uint8_t* ch_ser = malloc(ch_ser_len);
        // serialize_tlv(ch_ser, client_hello);

        // uint16_t sh_ser_len = serialize_tlv(tmp, server_hello); 
        // uint8_t* sh_ser = malloc(sh_ser_len);
        // serialize_tlv(sh_ser, server_hello);

        // size_t salt_size = ch_ser_len + sh_ser_len;
        // uint8_t* salt = malloc(salt_size);
        // memcpy(salt, ch_ser, ch_ser_len);
        // memcpy(salt + ch_ser_len, sh_ser, sh_ser_len);
        // free(ch_ser);
        // free(sh_ser);

        uint8_t salt[1024];

        size_t salt_size = serialize_tlv(salt, client_hello);
        salt_size += serialize_tlv(salt+salt_size, server_hello);

        derive_keys(salt, salt_size);

        // Get hmac into buf
        uint8_t digest_buf[32]; // MAC length: 32 bytes per spec, might need a couple more for metadata (probably not)
        hmac(digest_buf, salt, salt_size);

        tlv* transcript_tlv = create_tlv(TRANSCRIPT);
        add_val(transcript_tlv, digest_buf, 32);

        tlv* finished_tlv = create_tlv(FINISHED);
        add_tlv(finished_tlv, transcript_tlv); //might need to add more to finished_tlv?

        // free(salt);
        uint16_t l = serialize_tlv(buf, finished_tlv);
        handshake_status++; // done all handshake steps, onto normal data sending case
        return l;
        break;
    default:
        break;
    }
    
    //NORMAL DATA SENDING

    

    size_t data_length = input_io(buf, 943);
    if(!data_length){return 0;}

    fprintf(stderr, "STARTING DATA MESSAGE\n");
    uint8_t digest[MAC_SIZE];
    uint8_t iv_cipher[IV_SIZE+1024];
    // uint8_t iv[IV_SIZE];

    // uint8_t salt[1024];

    // size_t salt_size = serialize_tlv(salt, client_hello);
    // salt_size += serialize_tlv(salt+salt_size, server_hello);

    // derive_secret();
    
    // derive_keys(salt, salt_size);
    

    size_t cipher_length = encrypt_data(iv_cipher, iv_cipher+IV_SIZE, buf, data_length);
    

    tlv* message = create_tlv(DATA);
    tlv* iv_tlv = create_tlv(IV);
    tlv* cipher_tlv = create_tlv(CIPHERTEXT);
    tlv* mac_tlv = create_tlv(MAC);
    add_val(iv_tlv, iv_cipher, IV_SIZE);
    add_val(cipher_tlv, iv_cipher+IV_SIZE, cipher_length);

    size_t hmac_length = serialize_tlv(iv_cipher, iv_tlv);
    hmac_length += serialize_tlv(iv_cipher + hmac_length, cipher_tlv); 

    hmac(digest, iv_cipher, hmac_length);

    add_val(mac_tlv, digest, MAC_SIZE);
    add_tlv(message, iv_tlv);
    add_tlv(message, cipher_tlv);
    add_tlv(message, mac_tlv);

    size_t length = serialize_tlv(buf, message);

    print_tlv_bytes(buf, length);
    
    
    
    return length;
}

void output_sec(uint8_t* buf, size_t length) {

    if(length == 0){return;} //Do nothing if given no bytes to output

    // printf("Received data\n");
    
    switch (handshake_status)
    {
    case 0: //Client has not sent anything, server has yet to receive anything. Client is expecting nothing, server is expecting client hello
        if(cs_type == CLIENT){//Client has not sent first part of handshake
            exit(6); //The client should not receive any packets before sending first part of handshake
        }else{ //Server is supposed to be receiving CLIENT HELLO
            // printf("Here\n");
            tlv* ch = deserialize_tlv(buf, length);
            if(ch == NULL){exit(6);} //This isn't a valid TLV packet: seems like this should never happen unless length is 0?
            if(ch->type != CLIENT_HELLO){exit(6);} //Expecting a client hello, exit if not
            // printf("Not null or improper tlv-type\n");
            client_hello = ch;
            handshake_status += 1;
            //print_tlv_bytes(buf, length);
            return;
        }
    /* code */
        break;
    case 1: //Client has sent hello, server has received hello. Client is expecting server hello, server is expecting nothing
        if(cs_type == SERVER){exit(6);}
        tlv* sh = deserialize_tlv(buf, length);
        if(sh == NULL){exit(6);}
        if(sh->type != SERVER_HELLO){exit(6);}
        server_hello = sh;
        handshake_status++;
        //print_tlv_bytes(buf, length);
        return;

    case 2: //Client has received server hello, server has sent server hello. Client receives nothing, server receives finish
        if(cs_type == CLIENT){exit(6);}

        

        fprintf(stderr, "RECEIVED FINISHED\n");

        print_tlv_bytes(buf, length);

        tlv* finished = deserialize_tlv(buf, length);
        if(finished == NULL || finished->type != FINISHED){exit(6);}

        tlv* transcript = get_tlv(finished, TRANSCRIPT);
        if(transcript == NULL){exit(6);}

        tlv* pub_key = get_tlv(client_hello, PUBLIC_KEY);

        load_peer_public_key(pub_key->val, pub_key->length);
        set_private_key(my_private_key);

        uint8_t salt[1024];

        size_t salt_size = serialize_tlv(salt, client_hello);
        salt_size += serialize_tlv(salt+salt_size, server_hello);
        uint8_t digest[32];

        derive_secret();
        
        derive_keys(salt, salt_size);
        hmac(digest, salt, salt_size);

        if(memcmp(digest, transcript->val, 32)){
            fprintf(stderr, "INCORRECT TRANSCRIPT\n");
            exit(4);
        }

        fprintf(stderr, "CORRECT FINISHED\n");
        handshake_status++;


        return;


        break;
    
    default:
        break;
    }

    fprintf(stderr, "STARTING DATA PROCESSING\n");
    print_tlv_bytes(buf, length);


    tlv* data = deserialize_tlv(buf, length);
    if(data == NULL || data->type != DATA){
        fprintf(stderr, "EXPECTED DATA, RECEIVED SOMETHING ELSE\n");
        exit(6);
    }

    uint8_t iv_cipher[1012];

    tlv* iv = get_tlv(data, IV);
    tlv* cipher = get_tlv(data, CIPHERTEXT);
    tlv* mac = get_tlv(data, MAC);

    if(mac == NULL || iv == NULL || cipher == NULL){
        fprintf(stderr, "MISSING PART OF MESSAGE. MAC: %s, IV: %s, CIPHER: %s\n",
        mac == NULL? "MISSING" : "PRESENT", iv == NULL? "MISSING" : "PRESENT", cipher == NULL? "MISSING" : "PRESENT"
        );
    }

    // uint8_t salt[1024];

    // size_t salt_size = serialize_tlv(salt, client_hello);
    // salt_size += serialize_tlv(salt+salt_size, server_hello);

    // derive_secret();
    
    // derive_keys(salt, salt_size);


    fprintf(stderr, "COPYING MEMORY\n");
    // memcpy(iv_cipher, iv->val, iv->length);
    // memcpy(iv_cipher+IV_SIZE, cipher->val, cipher->length);

    size_t hmac_length = serialize_tlv(iv_cipher, iv);
    hmac_length += serialize_tlv(iv_cipher+hmac_length, cipher);

    uint8_t digest[MAC_SIZE];
    fprintf(stderr, "COMPUTING HMAC\n");
    hmac(digest, iv_cipher, hmac_length);
    fprintf(stderr, "COMPUTED HMAC\n");
    if(memcmp(digest, mac->val, MAC_SIZE)){
        fprintf(stderr, "MAC MISMATCH, COMPUTED MAC:\n");
        print_hex(digest, MAC_SIZE);
        fprintf(stderr, "IV+CIPHER:\n");
        print_hex(iv_cipher, IV_SIZE+cipher->length);
        exit(4);
    }

    fprintf(stderr, "DECRYPTING\n");
    length = decrypt_cipher(buf, cipher->val, cipher->length, iv->val);



    //NORMAL DATA SENDING
    
    
    output_io(buf, length);
}
