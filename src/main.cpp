// Copyright © 2020 Giorgio Audrito. All Rights Reserved.

#include "miosix.h"
#include "lib/fcpp.hpp"
#include "driver.hpp"

#define DEGREE      10  // maximum degree allowed for a deployment
#define DIAMETER    10  // maximum diameter in hops for a deployment
#define RECORD_TIME 300 // time in seconds during which the network topology is recorded
#define WINDOW_TIME 60  // time in seconds during which positive node information is retained

#define VULNERABILITY_DETECTION 1111
#define CONTACT_TRACING         2222
#define NONE                    3333

#ifndef CASE_STUDY
#define CASE_STUDY NONE
#endif

using namespace miosix;
using namespace fcpp;
using namespace component::tags;

//! @brief \return the maximum stack used by the node starting from the boot
uint16_t getMaxStackUsed()
{
    return MemoryProfiling::getStackSize() - MemoryProfiling::getAbsoluteFreeStack();
}

//! @brief \return the maximum heap used by the node (divided by 2 to fit in a short)
uint16_t getMaxHeapUsed()
{
    return (MemoryProfiling::getHeapSize() - MemoryProfiling::getAbsoluteFreeHeap()) / 2;
}

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
//! @brief Maximum stack size ever experienced.
struct max_stack {};
//! @brief Maximum heap size ever experienced.
struct max_heap {};
//! @brief Maximum message size ever experienced.
struct max_msg {};
//! @brief List of neighbours encountered during RECORD_TIME.
struct nbr_list {};
//! @brief Whether the device has been infected.
struct infected {};
//! @}

//! @brief Computes the maximum ever appeared in the network for a given value.
FUN(T) T max_ever(ARGS, T value) { CODE
    return nbr(CALL, value, [&](field<T> neigh_vals){
        return max(coordination::max_hood(CALL, neigh_vals), value);
    });
}

//! @brief Tracks the maximum consumption of memory and message resources.
FUN() void resource_tracking(ARGS) { CODE
    node.storage(max_stack{}) = max_ever(CALL, getMaxStackUsed());
    node.storage(max_heap{}) = max_ever(CALL, getMaxHeapUsed());
    node.storage(max_msg{}) = max_ever(CALL, uint8_t{42}); //TODO: @Giorgio add node.msg_size() built-in to inspect it
}

//! @brief Records the set of neighbours ever connected before RECORD_TIME.
FUN() void topology_recording(ARGS) { CODE
    field<device_t> nbr_uids = nbr(CALL, node.uid); //TODO: @Giorgio add nbr_uid() built-in to get it without increasing message size
    node.storage(nbr_list{}) = old(CALL, std::unordered_set<device_t>{}, [&](std::unordered_set<device_t> n){
        if (node.current_time() < RECORD_TIME)
            fold_hood(CALL, [&](device_t i, int){
                n.insert(i);
                return 0;
            }, nbr_uids, 0);
        return n;
    });
}

//! @brief Computes whether there is a node with only one connected neighbour at a given time.
FUN() void vulnerability_detection(ARGS, int diameter) { CODE
    node.storage(min_uid{}) = coordination::diameter_election(CALL, diameter);
    node.storage(hop_dist{}) = coordination::abf_hops(CALL, node.uid == node.storage(min_uid{}));
    bool collect_weak = coordination::sp_collection(CALL, node.storage(hop_dist{}), node.storage(neigh_count{}) <= 2, false, [](bool x, bool y) {
        return x or y;
    });
    node.storage(some_weak{}) = coordination::broadcast(CALL, node.storage(hop_dist{}), collect_weak);
}

//! @brief Computes whether the current node got in contact with a positive node within a time window.
FUN() void contact_tracing(ARGS, times_t window, bool positive) { CODE
    using contact_t = std::unordered_map<device_t, times_t>;
    contact_t contacts = old(CALL, contact_t{}, [&](contact_t c){
        // discard old contacts
        for (auto it = c.begin(); it != c.end();) {
          if (node.current_time() - it->second > window)
            it = c.erase(it);
          else ++it;
        }
        // add new contacts
        field<device_t> nbr_uids = nbr(CALL, node.uid); //TODO: @Giorgio add nbr_uid() built-in to get it without increasing message size
        fold_hood(CALL, [&](device_t i, int){
            c[i] = node.current_time();
            return 0;
        }, nbr_uids, 0);
        return c;
    });
    contact_t positives = nbr(CALL, contact_t{}, [&](field<contact_t> np){
        contact_t p{};
        if (positive) p[node.uid] = node.current_time();
        fold_hood(CALL, [&](contact_t const& cs, int){
            for (auto c : cs)
                if (node.current_time() - c.second < window)
                    p[c.first] = max(p[c.first], c.second);
            return 0;
        }, np, 0);
        return p;
    });
    node.storage(infected{}) = false;
    for (auto c : positives)
        if (contacts.count(c.first))
            node.storage(infected{}) = true;
}

//! @brief Main aggregate function.
FUN() void case_study(ARGS) { CODE
    node.storage(round_count{}) = coordination::counter(CALL, hops_t{1});
    node.storage(neigh_count{}) = count_hood(CALL);
#if CASE_STUDY == VULNERABILITY_DETECTION
    vulnerability_detection(CALL, DIAMETER);
#elif CASE_STUDY == CONTACT_TRACING
    contact_tracing(CALL, WINDOW_TIME, false);
#endif
    resource_tracking(CALL);
    topology_recording(CALL);
}


//! @brief Main struct calling `test_program`.
MAIN(case_study,);

//! @brief FCPP setup.
DECLARE_OPTIONS(opt,
    program<main>,
    retain<metric::retain<15, 10>>,
    round_schedule<sequence::periodic_n<1, 1, 1>>,
    exports<
        bool, hops_t, device_t, uint8_t, uint16_t, tuple<device_t, hops_t>,
        std::unordered_set<device_t>, std::unordered_map<device_t, times_t>
    >,
    tuple_store<
        round_count,int,
        neigh_count,int,
        min_uid,    device_t,
        hop_dist,   hops_t,
        some_weak,  bool,
        max_stack,  uint16_t,
        max_heap,   uint16_t,
        max_msg,    uint8_t,
        nbr_list,   std::unordered_set<device_t>,
        infected,   bool
    >
);

//! @brief Main function starting FCPP.
int main() {
    component::deployment<opt>::net network{common::make_tagged_tuple<hoodsize>(device_t{DEGREE})};
    network.run();
    return 0;
}
