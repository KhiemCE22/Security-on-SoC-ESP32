import hashlib
import sys
import os
from ecdsa import SigningKey, NIST256p

def digest_private_key_legacy(pem_path, output_path):
    print(f"--- Processing using the old method (Hash Private Key): {pem_path} ---")

    # 1. Read the PEM file and Load the Key (Identical to the _load_ecdsa_signing_key function)
    try:
        with open(pem_path, "rb") as f:
            content = f.read()
            # The from_pem function automatically parses the content
            sk = SigningKey.from_pem(content)
    except Exception as e:
        print(f"Error reading PEM file: {e}")
        return

    # 2. Get the raw byte string of the Private Key (Scalar)
    # This is the line: repr(sk.to_string()) in the code you provided
    raw_private_key = sk.to_string()
    
    print(f"Raw Private Key size: {len(raw_private_key)} bytes")
    # Check the size (NIST P-256 must be 32 bytes)
    if len(raw_private_key) != 32:
        print("Warning: Unusual Private Key length (should typically be 32 bytes for P-256)")

    # 3. Calculate SHA-256 (Identical to the old logic)
    digest = hashlib.sha256()
    digest.update(raw_private_key)
    result = digest.digest()

    print(f"SHA-256 Digest: {result.hex()}")

    # 4. Write to a .bin file
    with open(output_path, "wb") as f:
        f.write(result)
    
    print(f"--- File created successfully: {output_path} ---")
    print("NOTE: This is the SHA-256 of the PRIVATE KEY (Old/Custom method).")

if __name__ == "__main__":
    # Change your file name here if you don't want to use the command line
    # Example: INPUT_FILE = "my_private_key.pem"
    
    if len(sys.argv) != 3:
        print("Usage: python hash_old.py <input_private.pem> <output_digest.bin>")
    else:
        digest_private_key_legacy(sys.argv[1], sys.argv[2])