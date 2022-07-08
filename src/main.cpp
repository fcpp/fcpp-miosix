// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

#include <iostream>

#include "miosix.h"
#include "main.hpp"
#include "driver.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

// PURE C++ FUNCTIONS

//! @brief The maximum stack used by the node starting from the boot
inline uint16_t usedStack() {
    using namespace miosix;
    return MemoryProfiling::getStackSize() - MemoryProfiling::getAbsoluteFreeStack();
}

//! @brief The maximum heap used by the node (divided by 2 to fit in a short)
inline uint16_t usedHeap() {
    using namespace miosix;
    return (MemoryProfiling::getHeapSize() - MemoryProfiling::getAbsoluteFreeHeap() - BUFFER_SIZE*1024) / 2;
}

//! @brief Whether the button is currently pressed.
inline bool buttonPressed(device_t, uint16_t) {
    using namespace miosix;
    return userButton::value() == 0;
}


//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

//! @brief Handle for simulation code (empty).
FUN void simulation_handle(ARGS) {}

}

//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;

//! @brief Main FCPP option setup.
DECLARE_OPTIONS(deployment,
    main,
    plot_type<rows_type>
);

}

}


//! @brief Main function starting FCPP.
int main() {
    using namespace fcpp;

    // Type for the network object.
    using net_t = component::deployment<option::deployment>::net;
    // Create the logger object.
    option::rows_type row_store;
    // The initialisation values.
    auto init_v = common::make_tagged_tuple<option::hoodsize, option::plotter>(device_t{DEGREE}, &row_store);
    // Construct the network object.
    net_t network{init_v};
    // Run the program until exit.
    network.run();
    // Print the log until button release.
    while (true) {
        std::cout << "----" << std::endl << "log size " << row_store.byte_size() << std::endl;
        row_store.print(std::cout);
        while (not buttonPressed(0,0));
    }
    return 0;
}
