#include "SelfProgram.h"

#include <cstring>
#include "sha256.h"

uint8_t SelfProgram::eraseCount = 0;
bool SelfProgram::appFwFingerprintValid = false;

unsigned char SelfProgram::appFwFingerprint[32] = {0};
uint32_t SelfProgram::appFwFingerprintSalt = 0;

// Make it weak so it can be overridden
__attribute__((weak)) uint8_t SelfProgram::eraseApplicationFlash() {
    return 0;
}

void SelfProgram::readFlash(uint32_t address, uint8_t *data, uint16_t len) {
	for (uint8_t i=0; i < len; i++) {
		data[i] = readByte(address + i);
	}
}

uint8_t SelfProgram::readByte(uint32_t address) {
	uint8_t *ptr = (uint8_t*)FLASH_BASE + FLASH_APP_OFFSET + address;
	return *ptr;
}

void SelfProgram::calculateSaltedFingerprint(uint32_t salt) {
    uintptr_t start = FLASH_BASE + FLASH_APP_OFFSET;
    size_t size = applicationSize;

	//Helper to free sha context on return
    struct Context {
        mbedtls_sha256_context ctx;
        Context() {
            mbedtls_sha256_init(&ctx);
        }
        ~Context() {
            mbedtls_sha256_free(&ctx);
        }
    };
    Context context;

    if (mbedtls_sha256_starts_ret(&context.ctx, false) != 0) {
        return;
    }

    if (mbedtls_sha256_update_ret(&context.ctx, reinterpret_cast<uint8_t *>(&salt), sizeof(salt)) != 0) { // Salt
        return;
    }

    if (mbedtls_sha256_update_ret(&context.ctx, (const unsigned char *)start, size) != 0) { // Firmware
        return;
    }

    if (mbedtls_sha256_finish_ret(&context.ctx, appFwFingerprint) != 0) {
        return;
    }

    appFwFingerprintValid = true;
}

bool SelfProgram::checkUnsaltedFingerprint(const unsigned char fingerprint[32])
{
	uintptr_t start = FLASH_BASE + FLASH_APP_OFFSET;
	size_t size = applicationSize - FW_DESCRIPTOR_SIZE; // this is size of entire application flash area without the fw descriptor
	unsigned char calculatedFingerprint[32] = {0};

	if (mbedtls_sha256_ret((const unsigned char *)start, size, calculatedFingerprint, false) != 0) {
		return false;	// Couldn't check = check failed
	}

	return (memcmp(calculatedFingerprint, fingerprint, sizeof(calculatedFingerprint)) == 0);
}
