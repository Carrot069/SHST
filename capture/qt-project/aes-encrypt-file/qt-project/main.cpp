#include <iostream>
#include <fstream>

#include "aes.hpp"

using namespace std;

const uint8_t IV_LEN = 16;
typedef uint8_t aes_iv[IV_LEN];
const int AES256_KEY_LEN = 32;
typedef uint8_t aes256_key[AES256_KEY_LEN];

int ENCRYPT_DECRYPT_FLAG = 1;
int KEY_FILE = 2;
int IV_FILE = 3;
int SRC_FILE = 4;
int DEST_FILE = 5;

// aes 256bit 加密文件, key 必须是256bit长度(256/8字节), iv 必须是随机生成的, 但可以明文被知晓, 需要16字节长度
void aesEncryptFile(aes256_key key, aes_iv iv, string inFileName, string outFileName) {
    const int blockSize = 16;

    ifstream inFile(inFileName, ios::binary | ios::in | ios::ate);
    if (inFile.is_open()) {
        int fileSize = inFile.tellg();
        unsigned char padSize = blockSize - (fileSize % blockSize);
        if (padSize == 0)
            padSize = blockSize;
        int bufSize = fileSize + padSize;

        char* fileBuf = new char[bufSize];
        memset(fileBuf, padSize, bufSize);

        inFile.seekg(0, ios::beg);
        inFile.read(fileBuf, fileSize);

        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, key, iv);
        AES_CBC_encrypt_buffer(&ctx, reinterpret_cast<uint8_t*>(fileBuf), bufSize);

        ofstream outFile(outFileName, ios::binary | ios::out);
        if (outFile.is_open()) {
            outFile.write(fileBuf, bufSize);
            outFile.write(reinterpret_cast<char*>(iv), IV_LEN);
        } else {
            cout << "Unable open the file: " << outFileName << endl;
        }

        delete []fileBuf;
        inFile.close();
    } else {
        cout << "Unable open the file: " << inFileName << endl;
    }
};

void aesDecryptFile(aes256_key key, string inFileName, string outFileName) {
    ifstream inFile(inFileName, ios::binary | ios::in | ios::ate);
    if (inFile.is_open()) {
        int fileSize = inFile.tellg();
        int bufSize = fileSize - IV_LEN;

        char* fileBuf = new char[bufSize];
        memset(fileBuf, 0, bufSize);

        inFile.seekg(0, ios::beg);
        inFile.read(fileBuf, bufSize);

        aes_iv iv;
        inFile.read(reinterpret_cast<char*>(&iv[0]), IV_LEN);

        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, key, iv);
        AES_CBC_decrypt_buffer(&ctx, reinterpret_cast<uint8_t*>(fileBuf), bufSize);

        unsigned char padSize = fileBuf[bufSize-1];

        ofstream outFile(outFileName, ios::binary | ios::out);
        if (outFile.is_open()) {
            outFile.write(fileBuf, bufSize - padSize);
        } else {
            cout << "Unable open the file: " << outFileName << endl;
        }

        delete []fileBuf;
        inFile.close();
    } else {
        cout << "Unable open the file: " << inFileName << endl;
    }
};

// 命令行参数: 32字节长度的key文件完整路径 16字节长度的iv文件完整路径(只有加密时需要) 被加密文件完整路径 加密后文件的完整路径
int main(int argc, char* argv[]) {
    if (argc <= 1) {
        cout << "Usage: \n";
        cout << "\t0 = encrypt, 1 = decrypt\n";
        cout << "\tkey file\n";
        cout << "\tiv file (encrypt only)\n";
        cout << "\tsrc file\n";
        cout << "\tdest file\n";
    }
    if (argc < ENCRYPT_DECRYPT_FLAG+1) {
        cout << "error: no encrypt or decrypt flag" << endl;
        return 0;
    }
    if (argc < KEY_FILE+1) {
        cout << "error: no key file" << endl;
        return 0;
    }
    if (strcmp(argv[ENCRYPT_DECRYPT_FLAG], "0") == 0) {
        if (argc < IV_FILE+1) {
            cout << "error: no iv file" << endl;
            return 0;
        }
        if (argc < SRC_FILE+1) {
            cout << "error: no src file" << endl;
            return 0;
        }
        if (argc < DEST_FILE+1) {
            cout << "error: no dest file" << endl;
            return 0;
        }
    } else {
        SRC_FILE--;
        if (argc < SRC_FILE+1) {
            cout << "error: no src file" << endl;
            return 0;
        }
        DEST_FILE--;
        if (argc < DEST_FILE+1) {
            cout << "error: no dest file" << endl;
            return 0;
        }
    }

    ifstream keyFile(argv[KEY_FILE], ios::in | ios::binary);
    aes256_key key;
    memset(key, 0, AES256_KEY_LEN);
    if (keyFile.is_open()) {
        keyFile.read(reinterpret_cast<char*>(key), AES256_KEY_LEN);
        cout << "key:" << endl;
        for (int i = 0; i < AES256_KEY_LEN; i++) {
            cout << hex << static_cast<int>(key[i]) << ",";
        }
        cout << endl;
    } else {
        cout << "Unable to open the file: " << argv[KEY_FILE];
        keyFile.close();
        return 0;
    }
    keyFile.close();

    if (strcmp(argv[ENCRYPT_DECRYPT_FLAG], "0") == 0) {
        ifstream ivFile(argv[IV_FILE], ios::in | ios::binary);
        aes_iv iv;
        memset(iv, 0, IV_LEN);
        if (ivFile.is_open()) {
            ivFile.read(reinterpret_cast<char*>(iv), IV_LEN);
            cout << endl << "iv:" << endl;
            for (int i = 0; i < IV_LEN; i++) {
                cout << hex << static_cast<int>(iv[i]) << ",";
            }
            cout << endl;
        } else {
            cout << "Unable to open the file: " << argv[IV_FILE];
            ivFile.close();
            return 0;
        }
        ivFile.close();
        aesEncryptFile(key, iv, argv[SRC_FILE], argv[DEST_FILE]);
    } else {
        aesDecryptFile(key, argv[SRC_FILE], argv[DEST_FILE]);
    }

    return 0;
}
