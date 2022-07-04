// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

#include "simulation.hpp"


//! @brief The main function.
int main() {
    using namespace fcpp;

    // The network object type (interactive simulator with given options).
    using net_t = component::interactive_simulator<option::simulation>::net;
    // Create the plotter object.
    option::plotter_t p;
    // The initialisation values.
    auto init_v = common::make_tagged_tuple<option::name, option::texture, option::plotter>("MIOSIX Simulation", "building.jpg", &p);
    std::cout << "/*\n"; // avoid simulation output to interfere with plotting output
    {
        // Construct the network object.
        net_t network{init_v};
        // Run the simulation until exit.
        network.run();
    }
    std::cout << "*/\n"; // avoid simulation output to interfere with plotting output
    std::cout << plot::file("simulation", p.build()); // write plots
    return 0;
}
