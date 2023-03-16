// BootloaderWindowsTest.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <assert.h>
#include "Protocol.h"
#include "Constants.h"
#include <chrono>
#include <thread>
#include <iomanip>
#include <fstream>
#include <assert.h>

using std::cout;
using std::endl;


Protocol p;

void read_state()
{
    uint8_t data[7];
    size_t read_len = 0;
    
    auto res = p.run_transaction(Protocol::commands_t::GET_HARDWARE_INFO, nullptr, 0, data, sizeof(data), &read_len);
    cout << "res:" << res << ", len=" << read_len << ", data: ";
    for (size_t i = 0; i < read_len; i++)
    {
        cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)data[i] << " ";
    }
    cout << endl;

}


void read_fingerprint()
{
    uint8_t data[32];
    size_t read_len = 0;

    auto res = p.run_transaction(Protocol::commands_t::GET_FINGERPRINT, nullptr, 0, data, sizeof(data), &read_len);
    assert(read_len == sizeof(data));
    cout << "Fingerprint:" << res << ", len=" << read_len << ", data: ";
    for (size_t i = 0; i < read_len; i++)
    {
        cout << std::hex << std::setfill('0') << std::setw(2) << (unsigned int)data[i] << " ";
    }
    cout << endl;

}


void write_fimrware()
{
    std::ifstream file("testfile.bin", std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    char *buffer = new char[size];
    assert(file.read(buffer, size));

    uint8_t erase_count = 0;
    auto res = p.write_flash(reinterpret_cast<uint8_t *>(buffer), static_cast<uint32_t>(size), &erase_count);

    cout << "Flashing done, res=" << res << endl;


}


int main()
{  
    p.open("COM4");


    read_state();

    read_fingerprint();

    write_fimrware();

    read_fingerprint();
    
    auto res = p.run_app();
    cout << "Run app, res=" << res << endl;
}