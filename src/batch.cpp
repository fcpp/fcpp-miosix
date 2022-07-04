// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

#include "simulation.hpp"


//! @brief The main function.
int main() {
    using namespace fcpp;

    // The component type (batch simulator with given options).
    using comp_t = component::batch_simulator<option::simulation>;
    // Create the plotter object.
    option::plotter_t p;
    // The list of initialisation values to be used for simulations.
    auto init_list = batch::make_tagged_tuple_sequence(
        batch::arithmetic<option::seed>(0, 999, 1),              // 1000 different random seeds
        batch::stringify<option::output>("output/batch", "txt"), // generate output file name for the run
        batch::constant<option::plotter>(&p)                     // reference to the plotter object
    );
    // Runs the given simulations.
    batch::run(comp_t{}, init_list);
    // Builds the resulting plots.
    std::cout << plot::file("batch", p.build());
    return 0;
}
