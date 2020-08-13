package main

import (
	"bytes"
	"crypto/aes"
	"crypto/cipher"
	"crypto/hmac"

	"crypto/sha1"
	"encoding/base64"
	"encoding/binary"
	// "encoding/json"
	"errors"
	"golang.org/x/crypto/pbkdf2"
	"log"
	"fmt"
	// "math/big"
	// "os"
)

var skip = 0

func decodeBlob(blob64 string, keyStr string) (string, error) {

	sharedKeyBytes, err := base64.StdEncoding.DecodeString(keyStr)
	if err != nil {
		return "", err
	}

	blobBytes, err := base64.StdEncoding.DecodeString(blob64)
	if err != nil {
		return "", err
	}



	iv := blobBytes[0:16]
	encryptedPart := blobBytes[16 : len(blobBytes)-20]
	ckSum := blobBytes[len(blobBytes)-20:]
	key := sha1.Sum(sharedKeyBytes)
	base_key := key[:16]
	hash := hmac.New(sha1.New, base_key)

	hash.Write([]byte("checksum"))
	checksum_key := hash.Sum(nil)
	hash.Reset()

	hash.Write([]byte("encryption"))
	encryption_key := hash.Sum(nil)
	hash.Reset()

	macHash := hmac.New(sha1.New, checksum_key)
	macHash.Write(encryptedPart)
	mac := macHash.Sum(nil)

	if !bytes.Equal(mac, ckSum) {
		log.Println("add user error, mac doesn't match")
		return "", errors.New("mac mismatch")
	}

	block, _ := aes.NewCipher(encryption_key[0:16])
	stream := cipher.NewCTR(block, iv)
	fmt.Println(encryptedPart[60])

	stream.XORKeyStream(encryptedPart, encryptedPart)
	fmt.Println(encryptedPart[235])

	return string(encryptedPart), nil
}


func blobKey(username string, secret []byte) []byte {
	data := pbkdf2.Key(secret, []byte(username), 0x100, 20, sha1.New)[0:20]

	hash := sha1.Sum(data)
	length := make([]byte, 4)
	binary.BigEndian.PutUint32(length, 20)
	return append(hash[:], length...)
}

func decodeBlobSecondary(blob64 string, username string, deviceId string) []byte {
	blob, _ := base64.StdEncoding.DecodeString(blob64)
	secret := sha1.Sum([]byte(deviceId))
	fmt.Println(blob[40])

	key := blobKey(username, secret[:])
	fmt.Println(key[19])

	data := decryptBlob(blob, key)
	return data
}

func decryptBlob(blob []byte, key []byte) []byte {
	block, _ := aes.NewCipher(key)
	bs := block.BlockSize()
	fmt.Println(bs)
	if len(blob)%bs != 0 {
		panic("Need a multiple of the blocksize")
	}

	plaintext := make([]byte, len(blob))

	plain := plaintext
	for len(blob) > 0 {
		block.Decrypt(plaintext, blob)
		plaintext = plaintext[bs:]
		blob = blob[bs:]
	}

	l := len(plain)
	for i := 0; i < l-0x10; i++ {
		plain[l-i-1] = plain[l-i-1] ^ plain[l-i-0x11]
	}
	fmt.Println(len(plain))

	return plain
}

func readInt(b *bytes.Buffer) uint32 {
	c, _ := b.ReadByte()
	skip += 1
	fmt.Println("byte", c)
	lo := uint32(c)
	fmt.Println("lo ", lo)
	if lo&0x80 == 0 {
		return lo
	}

	c2, _ := b.ReadByte()
	skip += 1
	hi := uint32(c2)
	fmt.Println("hi ", hi)
	return lo&0x7f | hi<<7
}

func readBytes(b *bytes.Buffer) []byte {
	length := readInt(b)
	fmt.Println("SIZE", length)
	data := make([]byte, length)
	b.Read(data)
	skip += int(length)
	return data
}

func main() {


	fmt.Println("hello world")
}