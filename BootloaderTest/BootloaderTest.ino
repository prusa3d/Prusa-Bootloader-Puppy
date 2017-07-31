#include <SoftWire.h>
#include <ArduinoUnit.h>

#include "Constants.h"
#include "Util.h"
#include "PrintingSoftWire.h"

PrintingSoftWire bus(SDA, SCL);

struct {
  // A single test is done using these values, then they are changed
  // into various variations

  // Address to use after general call reset. If 0, no reset happens.
  uint8_t resetAddr = FIRST_ADDRESS;
  // Address to use for SET_I2C_ADDRESS. If 0, no set happens.
  uint8_t setAddr = 0;
  // Address to use for all tests. Overwritten by general call reset and
  // SET_I2C_ADDRESS
  uint8_t curAddr = 0;

  bool repStartAfterWrite = false;
  bool repStartAfterRead = false;
  bool ackLastRead = false;
  bool skipWrite = false;

  // These are not changed, so set them here
  bool printRawData = false;
  bool displayAttached = false;
  // When non-zero, only write this much bytes to flash on each test, to
  // speed up testing
  uint16_t maxWriteSize = 0;
} cfg;


bool write_command(uint8_t cmd, uint8_t *dataout, uint8_t len, uint8_t crc_xor = 0) {
  auto on_failure = []() { return false; };
  assertLessOrEqual(len + 2, MAX_MSG_LEN);
  assertAck(bus.startWrite(cfg.curAddr));
  assertAck(bus.write(cmd));
  for (uint8_t i = 0; i < len; ++i)
    assertAck(bus.write(dataout[i]));

  uint8_t crc = calcCrc(cmd, dataout, len);
  assertAck(bus.write(crc ^ crc_xor));

  if (!cfg.repStartAfterWrite)
    bus.stop();
  return true;
}

bool read_status(uint8_t *status, uint8_t *datain, uint8_t expectedLen, bool skip_start = false) {
  auto on_failure = []() { return false; };
  assertLessOrEqual(expectedLen + 2, MAX_MSG_LEN);
  if (!skip_start) {
    assertAck(bus.startRead(cfg.curAddr));
  }
  assertTrue(status != nullptr);
  assertAck(bus.readThenAck(*status));

  for (uint8_t i = 0; i < expectedLen; ++i)
    assertAck(bus.readThenAck(datain[i]));

  uint8_t crc;
  if (cfg.ackLastRead)
    assertAck(bus.readThenAck(crc));
  else
    assertAck(bus.readThenNack(crc));

  assertEqual(crc, calcCrc(*status, datain, expectedLen));
  if (!cfg.repStartAfterRead)
    bus.stop();
  return true;
}

bool run_transaction(uint8_t cmd, uint8_t *dataout, size_t len, uint8_t *status, uint8_t *datain = nullptr, size_t expectedLen = 0) {
  auto on_failure = []() { return false; };
  assertTrue(write_command(cmd, dataout, len));
  assertTrue(read_status(status, datain, expectedLen));
  return true;
}

bool run_transaction_ok(uint8_t cmd, uint8_t *dataout = nullptr, size_t len = 0, uint8_t *datain = nullptr, size_t expectedLen = 0) {
  auto on_failure = []() { return false; };
  uint8_t status;
  assertTrue(run_transaction(cmd, dataout, len, &status, datain, expectedLen));
  assertOk(status);
  return true;
}

test(010_general_call_reset) {
  if (!cfg.resetAddr) {
    skip();
    return;
  }

  // Both general call commands reset the i2c address, so test both of
  // them randomly (we can't tell the difference from the outside
  // anyway).
  uint8_t cmd = random(2) ? GeneralCallCommands::RESET : GeneralCallCommands::RESET_ADDRESS;
  assertAck(bus.startWrite(GENERAL_CALL_ADDRESS));
  assertAck(bus.write(cmd));
  bus.stop();
  uint8_t oldAddr = cfg.curAddr;
  cfg.curAddr = cfg.resetAddr;
  delay(100);

  if (oldAddr && (oldAddr < FIRST_ADDRESS || oldAddr > LAST_ADDRESS)) {
    // If the old address is set, but outside the default range, check
    // that it no longer response
    assertEqual(bus.startWrite(oldAddr), SoftWire::nack);
    bus.stop();
  }

  // Check for a response on the full address range
  for (uint8_t i = FIRST_ADDRESS; i < LAST_ADDRESS; ++i) {
    assertAck(bus.startWrite(i));
    bus.stop();
  }
}

test(020_ack) {
  assertAck(bus.startWrite(cfg.curAddr));
  bus.stop();
}


test(030_protocol_version) {
  uint8_t data[2];
  assertTrue(run_transaction_ok(Commands::GET_PROTOCOL_VERSION, nullptr, 0, data, sizeof(data)));

  uint16_t version = data[0] << 8 | data[1];
  assertEqual(version, PROTOCOL_VERSION);
}

test(040_not_set_i2c_address) {
  if (!cfg.setAddr) {
    skip();
    return;
  }
  // Test that the set i2c address command does not work when a wrong
  // hardware type is given
  uint8_t type = random(HARDWARE_TYPE + 1, 256);
  uint8_t data[2] = { cfg.setAddr, type };
  write_command(Commands::SET_I2C_ADDRESS, data, sizeof(data));

  // The command should be ignored, so a read command should not be acked
  assertEqual(bus.startRead(cfg.curAddr), SoftWire::nack);
  bus.stop();

  // And neither on the new address
  assertEqual(bus.startRead(cfg.setAddr), SoftWire::nack);
  bus.stop();

  // But it should still ack writes on the old address
  assertAck(bus.startWrite(cfg.curAddr));
  bus.stop();
}

test(050_set_i2c_address) {
  if (!cfg.setAddr) {
    skip();
    return;
  }

  // Randomly use either the wildcard, or specific selector
  uint8_t type = random(2) ? 0x00 : HARDWARE_TYPE;
  uint8_t data[2] = { cfg.setAddr, type };
  write_command(Commands::SET_I2C_ADDRESS, data, sizeof(data));

  // The command should be ignored, so a read command should not be acked
  SoftWire::result_t res = bus.startRead(cfg.curAddr);
  assertTrue(res == SoftWire::ack || SoftWire::nack);

  // Update the address, use this one for now
  uint8_t oldAddr = cfg.curAddr;
  cfg.curAddr = cfg.setAddr;
  // If an ack is received on the old address, continue reading the
  // reponse there (without sending another start). If not, read the
  // response from the new address.
  bool skip_start = (res == SoftWire::ack);
  uint8_t status;
  assertTrue(read_status(&status, NULL, 0, skip_start));
  assertOk(status);

  // If the address actually changed, check that no response is received
  // from the old address
  if (oldAddr != cfg.setAddr) {
    assertEqual(bus.startRead(oldAddr), SoftWire::nack);
    bus.stop();
    assertEqual(bus.startWrite(oldAddr), SoftWire::nack);
    bus.stop();
  }

  for (uint8_t i = FIRST_ADDRESS; i < LAST_ADDRESS; ++i) {
    // Check that it no longer response to the default range, except to
    // the new address if it happens to be in that range.
    if (i == cfg.curAddr)
      assertAck(bus.startWrite(i));
    else
      assertEqual(bus.startWrite(i), SoftWire::nack);
    bus.stop();
  }

  // Check that commands work on the new address
  uint8_t protodata[2];
  assertTrue(run_transaction_ok(Commands::GET_PROTOCOL_VERSION, nullptr, 0, protodata, sizeof(protodata)));
}


test(060_reread_protocol_version) {
  uint8_t data[2];
  assertTrue(run_transaction_ok(Commands::GET_PROTOCOL_VERSION, nullptr, 0, data, sizeof(data)));

  uint16_t version = data[0] << 8 | data[1];
  assertEqual(version, PROTOCOL_VERSION);

  uint8_t status;
  assertTrue(read_status(&status, data, sizeof(data)));
  assertOk(status);
  version = data[0] << 8 | data[1];
  assertEqual(version, PROTOCOL_VERSION);
}

test(070_get_hardware_info) {
  uint8_t data[5];
  assertTrue(run_transaction_ok(Commands::GET_HARDWARE_INFO, nullptr, 0, data, sizeof(data)));

  assertEqual(data[0], HARDWARE_TYPE);
  assertEqual(data[1], HARDWARE_REVISION);
  assertEqual(data[2], BOOTLOADER_VERSION);
  uint16_t flash_size = data[3] << 8 | data[4];
  assertEqual(flash_size, AVAILABLE_FLASH_SIZE);
}

test(080_crc_error) {
  uint8_t status;
  uint8_t crc_xor = random(1, 256);
  assertTrue(write_command(Commands::GET_HARDWARE_INFO, nullptr, 0, crc_xor));
  // This expects a CRC error, so no data
  assertTrue(read_status(&status, nullptr, 0));
  assertEqual(status, Status::INVALID_CRC);
}

test(100_command_not_supported) {
  uint8_t cmd = Commands::END_OF_COMMANDS;
  auto on_failure = [&cmd]() { Serial.print("cmd: "); Serial.println(cmd); };
  while (cmd != 0) {
    uint8_t status;
    assertTrue(run_transaction(cmd, nullptr, 0, &status));
    assertEqual(status, Status::COMMAND_NOT_SUPPORTED);
    ++cmd;
  }
}

test(110_power_up_display) {
  uint8_t data[1];
  assertTrue(run_transaction_ok(Commands::POWER_UP_DISPLAY, nullptr, 0, data, sizeof(data)));
  assertEqual(data[0], DISPLAY_CONTROLLER_TYPE);

  if (cfg.displayAttached) {
    // See if the display responds to its address
    assertAck(bus.startWrite(DISPLAY_I2C_ADDRESS));
    bus.stop();
  }
}

bool verify_flash(uint8_t *data, uint16_t len, uint8_t readlen) {
  uint16_t offset = 0;
  uint8_t i = 0;
  auto on_failure = [&i, &offset]() {
    Serial.print("offset = 0x");
    Serial.println(offset, HEX);
    Serial.print("i = 0x");
    Serial.println(i, HEX);
    return false;
  };

  uint8_t datain[readlen];
  while (offset < len) {
    uint8_t nextlen = min(readlen, len - offset);
    uint8_t dataout[3] = {(uint8_t)(offset >> 8), (uint8_t)offset, nextlen};
    assertTrue(run_transaction_ok(Commands::READ_FLASH, dataout, sizeof(dataout), datain, nextlen));
    for (i = 0; i < nextlen; ++i)
      assertEqual(datain[i], data[offset + i]);
    offset += nextlen;
  }
  return true;
}

bool write_flash_cmd(uint16_t address, uint8_t *data, uint8_t len, uint8_t *status) {
  auto on_failure = []() { return false; };
  uint8_t dataout[len + 2];

  dataout[0] = address >> 8;
  dataout[1] = address;
  memcpy(dataout + 2, data, len);
  assertTrue(run_transaction(Commands::WRITE_FLASH, dataout, len + 2, status));
  return true;
}

bool write_flash(uint8_t *data, uint16_t len, uint8_t writelen) {
  uint16_t offset = 0;
  auto on_failure = [&offset]() {
    Serial.print("offset = 0x");
    Serial.println(offset, HEX);
    return false;
  };

  while (offset < len) {
    uint8_t nextlen = min(writelen, len - offset);
    uint8_t status;
    assertTrue(write_flash_cmd(offset, data + offset, nextlen, &status));
    assertOk(status);
    offset += nextlen;
  }
  assertTrue(run_transaction_ok(Commands::FINALIZE_FLASH));
  return true;
}

bool write_and_verify_flash(uint8_t *data, uint16_t len, uint8_t writelen, uint8_t readlen) {
  auto on_failure = [readlen, writelen]() {
    Serial.print("writelen = ");
    Serial.println(writelen);
    Serial.print("readlen = ");
    Serial.println(readlen);
    return false;
  };
#ifdef TIME_WRITE
  unsigned long start = millis();
#endif
  assertTrue(write_flash(data, len, writelen));
#ifdef TIME_WRITE
  unsigned long now = millis();
  Serial.print("Write took ");
  Serial.print(now - start);
  Serial.println("ms");
#endif

#ifdef TIME_WRITE
  start = millis();
#endif
  assertTrue(verify_flash(data, len, readlen));
#ifdef TIME_WRITE
  now = millis();
  Serial.print("Verify took ");
  Serial.print(now - start);
  Serial.println("ms");
#endif
  return true;
}

// Write random data to flash and read it back, using various message
// sizes
test(120_write_flash) {
  if (cfg.skipWrite) {
    skip();
    return;
  }
  uint8_t hwinfo[5];
  assertTrue(run_transaction_ok(Commands::GET_HARDWARE_INFO, nullptr, 0, hwinfo, sizeof(hwinfo)));
  uint16_t flash_size = hwinfo[3] << 8 | hwinfo[4];

  if (cfg.maxWriteSize && cfg.maxWriteSize < flash_size)
    flash_size = cfg.maxWriteSize;

  uint8_t *data = (uint8_t*)malloc(flash_size);
  auto on_failure = [data]() { free(data); };
  assertTrue(data != nullptr);
  assertMoreOrEqual(flash_size, 2U);
  // The bootloader requires a RCALL or RJMP instruction at address 0,
  // so give it one
  data[0] = 0x00;
  data[1] = 0xC0;
  for (uint16_t i = 2; i < flash_size; ++i)
    data[i] = random();

  assertTrue(write_and_verify_flash(data, flash_size, 1, 1));
  assertTrue(write_and_verify_flash(data, flash_size, 7, 16));
  assertTrue(write_and_verify_flash(data, flash_size, 16, 3));
  // Max write and read size
  uint8_t writelen = MAX_MSG_LEN - 4; // cmd, 2xaddr, crc
  uint8_t readlen = MAX_MSG_LEN - 2; // cmd, 2xaddr
  assertTrue(write_and_verify_flash(data, flash_size, writelen, readlen));
  // Random sizes
  writelen = random(1, writelen);
  readlen = random(1, readlen);
  assertTrue(write_and_verify_flash(data, flash_size, writelen, readlen));

  free(data);
}

test(130_invalid_writes) {
  uint8_t data[32];
  uint8_t status;
  // Start at 0 and skip a few bytes
  assertTrue(write_flash_cmd(0, data, 13, &status));
  assertOk(status);
  assertTrue(write_flash_cmd(17, data, 11, &status));
  assertEqual(status, Status::INVALID_ARGUMENTS);

  // Restart at 0 and then skip up to the next erase page
  assertTrue(write_flash_cmd(0, data, 13, &status));
  assertOk(status);
  assertTrue(write_flash_cmd(64, data, 11, &status));
  assertEqual(status, Status::INVALID_ARGUMENTS);
  // Packet should be ignored, so a subsequent write aligned with the
  // first write should work.
  assertTrue(write_flash_cmd(13, data, 13, &status));
  assertOk(status);

}

void runTests() {
  long seed = random();
  randomSeed(seed);

  Serial.println("****************************");
  Serial.print("Random seed: ");
  Serial.println(seed);
  Serial.print("current addr = 0x");
  Serial.println(cfg.curAddr, HEX);
  Serial.print("reset addr = 0x");
  Serial.println(cfg.resetAddr, HEX);
  if (cfg.setAddr) {
    Serial.print("setting addr = 0x");
    Serial.println(cfg.setAddr, HEX);
  }
  if (cfg.repStartAfterWrite)
    Serial.println("using repstart after write");
  if (cfg.repStartAfterRead)
    Serial.println("using repstart after read");
  if (cfg.ackLastRead)
    Serial.println("acking last read");

  bus.print = cfg.printRawData;

  Test::resetAllTests();

  // Run twice: once for setup, once to run the actual test
  Test::run();
  Test::run();

  if (Test::getCurrentFailed()) {
    Serial.println("FAILED");
    while(true) /* nothing */;
  }

  // Randomly change these values on every run
  cfg.repStartAfterWrite = random(2);
  cfg.repStartAfterRead = random(2);
  cfg.ackLastRead = random(2);
}

void runFixedTests() {
  // Do one run with the default settings, so you can easily change
  // settings in the cfg variable definition for a quick test.
  runTests();

  cfg.skipWrite = false;
  cfg.repStartAfterWrite = false;
  cfg.repStartAfterRead = false;
  cfg.ackLastRead = false;
  cfg.setAddr = 0;

  // Run the tests for all allowed I2c addresses, without changing the
  // address
  for (uint8_t i = FIRST_ADDRESS; i < LAST_ADDRESS; ++i) {
      cfg.resetAddr = i;
      runTests();
  }

  // Pick a random address for the rest of these tests (excluding
  // LAST_ADDRESS)
  cfg.resetAddr = random(FIRST_ADDRESS, LAST_ADDRESS);
  // Change address to a low address
  cfg.setAddr = 1;
  runTests();
  // Change address to the same as used for the set address command
  cfg.setAddr = cfg.curAddr;
  runTests();
  // Change address to another address inside the range
  cfg.setAddr = LAST_ADDRESS;
  runTests();
  // Change address to a higher address
  cfg.setAddr = 130;
  runTests();
  // Change to another address without general call reset in between
  cfg.resetAddr = 0;
  cfg.setAddr = 123;
  runTests();
  // And do general call resets again
  cfg.resetAddr = random(FIRST_ADDRESS, LAST_ADDRESS);

  // Test changing the address to each possible address (except address
  // 0, but including other reserved addresses for simplicity). Skip the
  // write test, since it is slow.
  cfg.skipWrite = true;
  for (uint8_t i = 1; i < 128; ++i) {
    cfg.curAddr = random(FIRST_ADDRESS, LAST_ADDRESS + 1);
    cfg.setAddr = i;
    runTests();
  }
  cfg.skipWrite = false;
}

void runRandomTest() {
  cfg.setAddr = random(1, 128);
  cfg.resetAddr = random(FIRST_ADDRESS, LAST_ADDRESS+1);
  runTests();
}

#define DDR DDRD
#define PIN PIND
#define SCL_BIT (1 << PD0)
#define SDA_BIT (1 << PD1)

void setup() {
  Serial.begin(115200);

  bus.begin();
  // Check the macros above at runtime, unfortunately these
  // digitalPinTo* macros do not work at compiletime.
  if (&DDR != portModeRegister(digitalPinToPort(bus.getScl()))
      || &DDR != portModeRegister(digitalPinToPort(bus.getSda()))
      || SCL_BIT != digitalPinToBitMask(bus.getScl())
      || SDA_BIT != digitalPinToBitMask(bus.getSda())
  ) {
    Serial.println("Incorrect pin mapping");
    while(true) /* nothing */;
  }

  // These assume that pullups are disabled, so the PORT register is not
  // touched.
  bus._setSclLow = [](const SoftWire *) { DDR |= SCL_BIT; };
  bus._setSclHigh = [](const SoftWire *) { DDR &= ~SCL_BIT; };
  bus._setSdaLow = [](const SoftWire *) { DDR |= SDA_BIT; };
  bus._setSdaHigh = [](const SoftWire *) { DDR &= ~SDA_BIT; };
  bus._readScl = [](const SoftWire *) { return (uint8_t)(PIN & SCL_BIT); };
  bus._readSda = [](const SoftWire *) { return (uint8_t)(PIN & SDA_BIT); };

  // Run as fast as possible. With zero delay, the above direct pin
  // access results in about 12us clock period at 16Mhz
  bus.setDelay_us(0);

  // Allow re-running tests
  Test::flags.keep_completed_tests = true;

  Serial.println();
  Serial.println("****************************");
  Serial.println("Starting fixed tests");

  //bus.print = true;
  //Test::min_verbosity = TEST_VERBOSITY_ALL;
  runFixedTests();

  Serial.println();
  Serial.println("****************************");
  Serial.println("****************************");
  Serial.println("Starting random tests");
  while(true)
    runRandomTest();
}

void loop() { }
