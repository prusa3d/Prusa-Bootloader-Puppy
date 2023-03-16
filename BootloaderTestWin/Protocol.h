#pragma once

#include "rs232.h"
#include <assert.h>
#include "Crc.h"
#include <time.h>
#include "Constants.h"
#include <string>
#include <algorithm>
#include <iostream>

using std::cout;
using std::endl;

class Protocol
{
public:

    enum status_t {
        COMMAND_OK = 0x00,
        COMMAND_FAILED = 0x01,
        COMMAND_NOT_SUPPORTED = 0x02,
        INVALID_TRANSFER = 0x03,
        INVALID_CRC = 0x04,
        INVALID_ARGUMENTS = 0x05,

        // internal errors (caused by IO error etc, never actually transmitted via communication)
        WRITE_ERROR = 0x11,
        NO_RESPONSE = 0x12,
        INCOMPLETE_RESPONSE = 0x13,
        BAD_RESPONSE = 0x14,

    };

    enum commands_t
    {
        GET_PROTOCOL_VERSION = 0x00,
        SET_ADDRESS = 0x01,
        POWER_UP_DISPLAY = 0x02,
        GET_HARDWARE_INFO = 0x03,
        GET_SERIAL_NUMBER = 0x04,
        START_APPLICATION = 0x05,
        WRITE_FLASH = 0x06,
        FINALIZE_FLASH = 0x07,
        READ_FLASH = 0x08,
        GET_HARDWARE_REVISION = 0x09,
        GET_NUM_CHILDREN = 0x0a,
        SET_CHILD_SELECT = 0x0b,
        GET_MAX_PACKET_LENGTH = 0x0c,
        GET_EXTRA_INFO = 0x0d,
        GET_FINGERPRINT = 0x0e,
        RESET = 0x46,
        RESET_ADDRESS = 0x44,
        END_OF_commands_t
    };

    bool open(const char* portName)
    {
        comEnumerate();
        port = comFindPort(portName);
        assert(port > 0);
        assert(comOpen(port, 230400) == 1);
        return true;
    }
    
    bool write(const uint8_t* data, size_t len)
    {
        return comWrite(port, (const char*)data, len) == len;
    }

    status_t write_command(uint8_t cmd, uint8_t* dataout, uint8_t len, uint8_t crc_xor = 0) {
        assert(len <= MAX_REQUEST_DATA_LEN);

        discard_read_buffer();

        uint8_t buffer[MAX_PACKET_LENGTH];
        size_t pos = 0;
        buffer[pos++] = curAddr;
        buffer[pos++] = cmd;

        if (len > 0)
        {
            memcpy(&buffer[2], dataout, len);
            pos += len;
        }

        uint16_t crc = Crc16Ibm().update(curAddr).update(cmd).update(dataout, len).get();
        buffer[pos++] = (crc & 0xff) ^ crc_xor;
        buffer[pos++] = (crc >> 8) ^ crc_xor;

        if (!write(buffer, pos))
        {
            return status_t::WRITE_ERROR;
        }
        return status_t::COMMAND_OK;
    }

    void discard_read_buffer()
    {
        char buffer[8];
        while (comRead(port, buffer, sizeof(buffer)))
        {   
        }
    }

    size_t read_bytes(uint8_t* data, size_t num, uint32_t timeout)
    {
        clock_t timeout_clock = timeout * (CLOCKS_PER_SEC / 1000);
        clock_t start = clock();

        size_t read = 0;

        // read while not timeout and still have bytes to read
        while (clock() - start <= timeout_clock && read < num)
        {
            read += comRead(port, reinterpret_cast<char*>(&data[read]), num - read);
        }
        return read;
    }

    status_t read_status(uint8_t* datain, size_t datain_max_len, size_t* actualLen = nullptr)
    {
        uint8_t buffer[3];

        size_t read = read_bytes(buffer, 3, MAX_RESPONSE_TIME);
        if (read != 3)
        {
            if (read == 0)
            {
                return status_t::NO_RESPONSE;
            }
            else
            {
                return status_t::INCOMPLETE_RESPONSE;
            }
        }

        uint8_t addr = buffer[0];
        if (addr != curAddr)
        {
            return status_t::BAD_RESPONSE;
        }

        uint8_t status = buffer[1];

        uint8_t len = buffer[2];
        if (len > MAX_RESPONSE_DATA_LEN || len > datain_max_len)
        {
            return status_t::BAD_RESPONSE;
        }
        if (actualLen != nullptr)
        {
            *actualLen = len;
        }
        

        if (len > 0)
        {
            read = read_bytes(datain, len, BASE_INTER_CHARACTER + MAX_INTER_CHARACTER * len);
            if (read != len)
            {
                return status_t::INCOMPLETE_RESPONSE;
            }
        }
        
        uint8_t crc[2];
        read = read_bytes(crc, 2, BASE_INTER_CHARACTER + MAX_INTER_CHARACTER * 2);
        if (read != 2)
        {
            return status_t::INCOMPLETE_RESPONSE;
        }

        uint16_t expectedCrc = Crc16Ibm().update(buffer, 3).update(datain, len).get();
        if (((crc[1] << 8) | crc[0]) != expectedCrc)
        {
            return status_t::INVALID_CRC;
        }
        
        return (status_t)status;
    }

    status_t run_transaction(uint8_t cmd, uint8_t* wr_data, uint8_t wr_data_len, uint8_t* read_data = nullptr, size_t read_data_max = 0, size_t* read_data_len = nullptr) 
    {
        auto res = write_command(cmd, wr_data, wr_data_len);
        if (res != status_t::COMMAND_OK) return res;

        res = read_status(read_data, read_data_max, read_data_len);
        if (res != status_t::COMMAND_OK) return res;

        return status_t::COMMAND_OK;
    }


    status_t write_flash_cmd(uint32_t address, uint8_t* data, uint8_t len) 
    {
        assert(len <= MAX_FLASH_BLOCK_LENGTH);
        uint8_t dataout[MAX_REQUEST_DATA_LEN];

        dataout[0] = address >> 24;
        dataout[1] = address >> 16;
        dataout[2] = address >> 8;
        dataout[3] = address & 0xFF;
        memcpy(dataout + 4, data, len );
        return run_transaction(commands_t::WRITE_FLASH, dataout, len + 4, nullptr, 0, nullptr);   
    }


    status_t write_flash(uint8_t* data, uint32_t len, uint8_t* erase_count) 
    {
        assert(len <= MAX_FLASH_TOTAL_LENGTH);
        uint32_t offset = 0;

        while (offset < len)
        {
            uint8_t nextlen = std::min((uint32_t)MAX_FLASH_BLOCK_LENGTH, len - offset);
            uint8_t reason = -1;
            auto res = write_flash_cmd(offset, data + offset, nextlen);
            cout << "Write to offset " << offset << ", res = " << (int)res << endl;
            if (res != status_t::COMMAND_OK)
                return res;
            
            offset += nextlen;
        }

        size_t response_len = 0;
        auto res = run_transaction(commands_t::FINALIZE_FLASH, nullptr, 0, erase_count, sizeof(erase_count), &response_len);
        cout << "Finalize res = " << (int)res << ", erase count:" << (int)* erase_count << endl;
        if (res != status_t::COMMAND_OK)
        {
            return res;
        }

        if (response_len != sizeof(*erase_count))
        {
            return status_t::BAD_RESPONSE;
        }

        return status_t::COMMAND_OK;
    }

    status_t run_app()
    {
        return write_command(commands_t::START_APPLICATION, nullptr, 0);
    }


private:
    uint8_t curAddr = 0x08;
    int port = -1;

    static const uint16_t MAX_PACKET_LENGTH = 255;
    static const uint16_t MAX_REQUEST_DATA_LEN = MAX_PACKET_LENGTH - 4;
    static const uint16_t MAX_RESPONSE_DATA_LEN = MAX_PACKET_LENGTH - 5;
    static const uint16_t MAX_FLASH_BLOCK_LENGTH = MAX_REQUEST_DATA_LEN - 4; // 4B for offset
    static const uint32_t MAX_FLASH_TOTAL_LENGTH = 128*1024;
    static const uint32_t MAX_RESPONSE_TIME = 500;
    static const uint32_t BASE_INTER_CHARACTER = 100;
    static const uint32_t MAX_INTER_CHARACTER = 1;
    static const uint32_t MAX_INTER_FRAME = 1;
};

