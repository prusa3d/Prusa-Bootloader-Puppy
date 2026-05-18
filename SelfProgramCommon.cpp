#include "SelfProgram.h"

#include <cstring>
#include "sha256.h"
#include "iwdg.hpp"

uint8_t SelfProgram::eraseCount = 0;
bool SelfProgram::appFwFingerprintValid = false;

unsigned char SelfProgram::appFwFingerprint[32] = {0};
uint32_t SelfProgram::appFwFingerprintSalt = 0;

void SelfProgram::readFlash(uint32_t address, uint8_t *data, uint16_t len) {
	for (uint8_t i=0; i < len; i++) {
		data[i] = readByte(address + i);
	}
}

uint8_t SelfProgram::readByte(uint32_t address) {
	uint8_t *ptr = (uint8_t*)FLASH_BASE + FLASH_APP_OFFSET + address;
	return *ptr;
}

void SelfProgram::calculateFingerprint(const uint32_t *salt_or_null, uint32_t size, unsigned char output[32]) {
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx);

    if (salt_or_null) {
        mbedtls_sha256_update_ret(&ctx, reinterpret_cast<const uint8_t *>(salt_or_null), sizeof(*salt_or_null));
    }

    // Hash the firmware in chunks so we can kick the watchdog
    static constexpr size_t chunk = 1024;
    const unsigned char *p = (const unsigned char *)(FLASH_BASE + FLASH_APP_OFFSET);
    while (size > 0) {
        size_t n = size < chunk ? size : chunk;
        mbedtls_sha256_update_ret(&ctx, p, n);
        WatchdogReset();
        p += n;
        size -= n;
    }

    mbedtls_sha256_finish_ret(&ctx, output);
    mbedtls_sha256_free(&ctx);
}

void SelfProgram::calculateSaltedFingerprint(uint32_t salt) {
    calculateFingerprint(&salt, applicationSize, appFwFingerprint);
    appFwFingerprintValid = true;
}

bool SelfProgram::checkUnsaltedFingerprint(const unsigned char fingerprint[32])
{
	unsigned char calculatedFingerprint[32];
	calculateFingerprint(nullptr, applicationSize - FW_DESCRIPTOR_SIZE, calculatedFingerprint);
	return (memcmp(calculatedFingerprint, fingerprint, sizeof(calculatedFingerprint)) == 0);
}
