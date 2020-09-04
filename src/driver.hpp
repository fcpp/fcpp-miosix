// Copyright © 2020 Giorgio Audrito. All Rights Reserved.

/**
 * @file os.hpp
 * @brief Abstract functions defining an OS interface.
 */

#ifndef FCPP_DEPLOYMENT_OS_H_
#define FCPP_DEPLOYMENT_OS_H_

#include <vector>

#include "lib/settings.hpp"


/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {


//! @brief Type for raw messages received.
struct message_type {
    //! @brief Timestamp of message receival.
    times_t time;
    //! @brief UID of the sender device.
    device_t device;
    //! @brief An estimate of the signal power (RSSI).
    double power;
    //! @brief The message content.
    std::vector<char> content;
};


//! @brief Namespace containing OS-dependent functionalities.
namespace os {


//! @brief Access the local unique identifier.
device_t uid() {
    // TODO: hash if device_t smaller than 64 bits
    return *reinterpret_cast<unsigned long long*>(0x0FE081F0);
}


/**
 * @brief Interface for network capabilities.
 *
 * It should have the following minimal public interface:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * struct data_type;            // default-constructible type for settings
 * data_type data;              // network settings
 * network(node_t&, data_type); // constructor with node reference and settings
 * bool empty() const;          // tests whether there are incoming messages
 * message_type pop();          // returns the next incoming message in queue order
 * void push(std::vector<char>);// requires the broadcast of a given message
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * @param node_t The node type. It has the following method that can be called at any time:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
 * void receive(message_type);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~
 */
template <typename node_t>
struct network;


}


}

#endif // FCPP_DEPLOYMENT_OS_H_
