// Copyright © 2020 Giorgio Audrito. All Rights Reserved.

#include "miosix.h"
#include "lib/fcpp.hpp"
#include "driver.hpp"

#define DIAMETER  10 // maximum diameter in hops for a deployment

using namespace miosix;
using namespace fcpp;
using namespace component::tags;

//! @brief Storage tags
//! @{
//! @brief Total round count since start.
struct round_count {};
//! @brief Current count of neighbours.
struct neigh_count {};
//! @brief Minimum UID in the network.
struct min_uid {};
//! @brief Distance in hops to the device with minimum UID.
struct hop_dist {};
//! @brief Whether some device in the network has only one neighbour.
struct some_weak {};
//! @}


//! @brief Main aggregate function.
FUN() void test_program(ARGS, int diameter) { CODE
    node.storage(round_count{}) = coordination::counter(CALL, hops_t{1});
    node.storage(neigh_count{}) = count_hood(CALL);
    node.storage(min_uid{}) = coordination::diameter_election(CALL, diameter);
    node.storage(hop_dist{}) = coordination::abf_hops(CALL, node.uid == node.storage(min_uid{}));
    bool collect_weak = coordination::sp_collection(CALL, node.storage(hop_dist{}), node.storage(neigh_count{}) <= 2, false, [](bool x, bool y) {
        return x or y;
    });
    node.storage(some_weak{}) = coordination::broadcast(CALL, node.storage(hop_dist{}), collect_weak);
}


//! @brief Main struct calling `test_program`.
MAIN(test_program,,DIAMETER);

//! @brief FCPP setup.
DECLARE_OPTIONS(opt,
    program<main>,
    round_schedule<sequence::periodic_n<1, 1, 1>>,
    exports<
        bool, hops_t, device_t, tuple<device_t, hops_t>
    >,
    tuple_store<
        round_count,int,
        neigh_count,int,
        min_uid,    device_t,
        hop_dist,   hops_t,
        some_weak,  bool
    >
);

//! @brief Main function starting FCPP.
int main() {
    component::deployment<opt>::net network{common::make_tagged_tuple<>()};
    network.run();
    return 0;
}
