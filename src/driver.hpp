// Copyright © 2020 Giorgio Audrito. All Rights Reserved.

/**
 * @file driver.hpp
 * @brief Implementation of the OS interface for MIOSIX.
 */

#ifndef FCPP_MIOSIX_DRIVER_H_
#define FCPP_MIOSIX_DRIVER_H_

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <chrono>
#include <exception>
#include <random>
#include <thread>
#include <vector>

#include "lib/settings.hpp"
#include "lib/deployment/os.hpp"

#include "miosix.h"
#include "interfaces-impl/transceiver.h"

#define DBG_PRINT_SUCCESSFUL_CALLS
#define DBG_TRANSCEIVER_ACTIVITY_LED


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


//! @brief Namespace containing OS-dependent functionalities.
namespace os {

void activity();

//! @brief Access the local unique identifier.
inline device_t uid() {
    uint64_t id = *reinterpret_cast<uint64_t*>(0x0FE081F0);
#if   FCPP_DEVICE == 64
    return id;
#else
    device_t res = 0;
    for (int i=0; i<64; i+=FCPP_DEVICE)
        res ^= (id >> i) & ((1ULL << FCPP_DEVICE) - 1);
    return res;
#endif
}


/**
 * @brief Low-level interface for hardware network capabilities.
 *
 * It should have the following minimal public interface:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * struct data_type;                            // default-constructible type for settings
 * data_type data;                              // network settings
 * transceiver(data_type);                      // constructor with settings
 * bool send(device_t, std::vector<char>, int); // broadcasts a message after given attemps
 * message_type receive(int);                   // listens for messages after given failed sends
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 */
struct transceiver {
    //! @brief Default-constructible type for settings.
    struct data_type {
        //! @brief Transmission frequency in MHz.
        int frequency;
        //! @brief Transmission power in in dBm.
        int power;
        //! @brief Base time in nanoseconds for each receive call.
        long long receive_time;
        //! @brief Number of attempts after which a send is aborted.
        uint8_t send_attempts;

        //! @brief Member constructor with defaults.
        data_type(int freq = 2450, int pow = 5, long long recv = 50000000LL, uint8_t sndatt = 5) : frequency(freq), power(pow), receive_time(recv), send_attempts(sndatt) {}
    };

    //! @brief Network settings.
    data_type data;

    //! @brief Constructor with settings.
    transceiver(data_type d) : data(d), m_transceiver(miosix::Transceiver::instance()), m_timer(miosix::getTransceiverTimer()), m_start(std::chrono::high_resolution_clock::now()), m_rng(std::chrono::system_clock::now().time_since_epoch().count()) {
        miosix::TransceiverConfiguration config(
            data.frequency,
            data.power,
            true,  //CRC
            false  //strictTimeout
        );
        m_transceiver.configure(config);
        m_transceiver.turnOn();
    }

    //! @brief Broadcasts a given message.
    bool send(device_t id, std::vector<char> m, int attempt) const {
        m.resize(m.size() + sizeof(device_t));
        if (m.size() > 125) {
            printf("Send failed: message overflow (%d/125 bytes)\n", m.size());
            return true;
        }
        *reinterpret_cast<device_t*>(m.data() + m.size() - sizeof(device_t)) = id;
        try {
            if (m_transceiver.sendCca(m.data(), m.size())) {
                activity();
                #ifdef DBG_PRINT_SUCCESSFUL_CALLS
                printf("Sent %d byte packet\n", m.size());
                #endif //DBG_PRINT_SUCCESSFUL_CALLS
                return true;
            } else return attempt == data.send_attempts;
        } catch (std::exception& e) {
            printf("Send failed: %s\n", e.what());
            return attempt == data.send_attempts;
        }
    }

    //! @brief Receives the next incoming message (empty if no incoming message).
    message_type receive(int attempt) const {
        long long interval = (data.receive_time << attempt);
        if (attempt > 0) {
            std::uniform_int_distribution<long long> d(data.receive_time, interval);
            interval = d(m_rng);
        }
        interval = m_timer.ns2tick(interval);
        message_type m;
        m.content.resize(125);
        try {
            auto result = m_transceiver.recv(m.content.data(), 125, m_timer.getValue() + interval);
            if (result.error == miosix::RecvResult::OK and result.size >= (int)sizeof(device_t)) {
                m.time = (std::chrono::high_resolution_clock::now() - m_start).count() * std::chrono::high_resolution_clock::period::num * 1.0 / std::chrono::high_resolution_clock::period::den;
                m.power = result.rssi; // TODO: convert in meters
                m.device = *reinterpret_cast<device_t*>(m.content.data() + result.size - sizeof(device_t));
                m.content.resize(result.size - sizeof(device_t));
                activity();
                #ifdef DBG_PRINT_SUCCESSFUL_CALLS
                printf("Received %d byte packet from device %d at time %f\n", result.size, m.device, m.time);
                #endif //DBG_PRINT_SUCCESSFUL_CALLS
            } else {
                switch (result.error) {
                    case miosix::RecvResult::OK:       printf("Receive error: too short packet (%d/%d bytes)\n", result.size, (int)sizeof(device_t)); break;
                    case miosix::RecvResult::TOO_LONG: printf("Receive error: too long packet (%d/125 bytes)\n", result.size); break;
                    case miosix::RecvResult::CRC_FAIL: printf("Receive error: wrong CRC\n"); break;
                    case miosix::RecvResult::TIMEOUT:  break;
                }
                m.content.clear();
            }
        } catch(std::exception& e) {
            printf("Receive exception: %s\n", e.what());
            m.content.clear();
        }
        return m;
    }

  private:
    //! @brief The miosix transceiver interface.
    miosix::Transceiver& m_transceiver;
    //! @brief The miosix transceiver timer.
    miosix::HardwareTimer& m_timer;
    //! @brief The time of program start.
    std::chrono::high_resolution_clock::time_point m_start;
    //! @brief A random engine.
    mutable std::default_random_engine m_rng;
};


}


}

#endif // FCPP_MIOSIX_DRIVER_H_
