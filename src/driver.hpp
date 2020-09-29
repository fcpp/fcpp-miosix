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


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


//! @brief Namespace containing OS-dependent functionalities.
namespace os {


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
 * struct data_type;                       // default-constructible type for settings
 * data_type data;                         // network settings
 * transceiver(data_type);                 // constructor with settings
 * void send(device_t, std::vector<char>); // broadcasts a given message
 * message_type receive();                 // receives the next incoming message (empty if no incoming message)
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 */
struct transceiver {
    //! @brief Default-constructible type for settings.
    struct data_type {
        //! @brief Transmission frequency in MHz.
        int frequency;
        //! @brief Transmission power in in dBm.
        int power;
        //! @brief Time in nanoseconds after which send/receive is first retried.
        long long retry_time;
        //! @brief Time in nanoseconds after which send/receive is aborted.
        long long fail_time;
        //! @brief Time in nanoseconds for each receive call.
        long long receive_time;

        //! @brief Member constructor with defaults.
        data_type(int freq = 2450, int pow = 5, long long retry = 10000000LL, long long fail = 1000000000LL, long long recv = 50000000LL) : frequency(freq), power(pow), retry_time(retry), fail_time(fail), receive_time(recv) {}
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
    void send(device_t id, std::vector<char> m) const {
        m.resize(m.size() + sizeof(device_t));
        if (m.size() > 125) {
            printf("Send failed: message overflow\n");
            return;
        }
        *reinterpret_cast<device_t*>(m.data() + m.size() - sizeof(device_t)) = id;
        for (int delay = data.retry_time; delay < data.fail_time; delay *= 2) {
            try {
                if (m_transceiver.sendCca(m.data(), m.size())) break;
                std::uniform_int_distribution<long long> d(delay, 2*delay);
                std::this_thread::sleep_for(std::chrono::nanoseconds(d(m_rng)));
            } catch (std::exception& e) {
                printf("Send failed: %s\n", e.what());
            }
        }
    }

    //! @brief Receives the next incoming message (empty if no incoming message).
    message_type receive() const {
        const long long interval = m_timer.ns2tick(data.receive_time);
        message_type m;
        m.content.resize(125);
        try {
            auto result = m_transceiver.recv(m.content.data(), 125, m_timer.getValue() + interval);
            if (result.error == miosix::RecvResult::OK and result.size >= (int)sizeof(device_t)) {
                m.time = (std::chrono::high_resolution_clock::now() - m_start).count() * std::chrono::high_resolution_clock::period::num / std::chrono::high_resolution_clock::period::den;
                m.power = result.rssi; // TODO: convert in meters
                m.device = *reinterpret_cast<device_t*>(m.content.data() + result.size - sizeof(device_t));
                m.content.resize(result.size - sizeof(device_t));
            } else {
                printf("Receive error: ");
                switch (result.error) {
                    case miosix::RecvResult::OK:       printf("too short packet\n"); break;
                    case miosix::RecvResult::TOO_LONG: printf("too long packet\n");  break;
                    case miosix::RecvResult::CRC_FAIL: printf("wrong CRC\n");        break;
                    case miosix::RecvResult::TIMEOUT:  printf("timeout\n");          break;
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
