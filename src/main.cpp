// Copyright © 2020 Giorgio Audrito. All Rights Reserved.

#include <iostream>
#include "streamlogger.h"

#include "miosix.h"
#include "lib/fcpp.hpp"
#include "driver.hpp"

#define DEGREE      10  // maximum degree allowed for a deployment
#define DIAMETER    10  // maximum diameter in hops for a deployment
#define RECORD_TIME 300 // time in seconds during which the aggregate program is run and recorded
#define WINDOW_TIME 60  // time in seconds during which positive node information is retained
#define ROUND_PERIOD 1  // time in seconds between transmission rounds

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
uint16_t getMaxStackUsed() {
    return MemoryProfiling::getStackSize() - MemoryProfiling::getAbsoluteFreeStack();
}

//! @brief \return the maximum heap used by the node (divided by 2 to fit in a short)
uint16_t getMaxHeapUsed() {
    return (MemoryProfiling::getHeapSize() - MemoryProfiling::getAbsoluteFreeHeap()) / 2;
}

//! @brief Storage tags
//! @{
//! @brief Total round count since start.
struct round_count {};
//! @brief A shared global clock.
struct global_clock {};
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
//! @brief List of neighbours encountered at least 50% of the times.
struct nbr_list {};
//! @brief Whether the device has been infected.
struct infected {};
//! @brief The list of positive devices in the network.
struct positives {};
//! @}

//! @brief Does not change a value after a given time.
FUN(T) T fix_after(ARGS, T value, times_t t) { CODE
    return old(node, 0, value, [&](T o){
        return node.current_time() < t ? value : o;
    });
}

//! @brief Tracks the maximum consumption of memory and message resources.
FUN() void resource_tracking(ARGS) { CODE
    node.storage(max_stack{}) = coordination::gossip_max(node, 0, getMaxStackUsed());
    node.storage(max_heap{}) = uint32_t{2} * coordination::gossip_max(node, 1, getMaxHeapUsed());
    node.storage(max_msg{}) = coordination::gossip_max(node, 2, (int8_t)min(node.msg_size(), size_t{127}));
}

//! @brief Records the set of neighbours ever connected before RECORD_TIME.
FUN() void topology_recording(ARGS) { CODE
    using map_t = std::unordered_map<device_t,times_t>;
    field<device_t> nbr_uids = nbr_uid(node, 0);
    map_t nbr_counters = old(node, 1, map_t{}, [&](map_t n){
        fold_hood(node, 2, [&](device_t i, int){
            n[i] += 1;
            return 0;
        }, nbr_uids, 0);
        return n;
    });
    node.storage(nbr_list{}).clear();
    for (const auto& it : nbr_counters)
        if (it.second >= node.storage(round_count{})/2)
            node.storage(nbr_list{}).insert(it.first);
}

//! @brief Computes whether there is a node with only one connected neighbour at a given time.
FUN() void vulnerability_detection(ARGS, int diameter) { CODE
    tie(node.storage(min_uid{}), node.storage(hop_dist{})) = coordination::diameter_election_distance(node, 0, diameter);
    bool collect_weak = coordination::sp_collection(node, 1, node.storage(hop_dist{}), node.storage(neigh_count{}) <= 2, false, [](bool x, bool y) {
        return x or y;
    });
    node.storage(some_weak{}) = coordination::broadcast(node, 2, node.storage(hop_dist{}), collect_weak);
}

//! @brief Computes whether the current node got in contact with a positive node within a time window.
FUN() void contact_tracing(ARGS, times_t window, bool positive) { CODE
    using contact_t = std::unordered_map<device_t, times_t>;
    contact_t contacts = old(node, 0, contact_t{}, [&](contact_t c){
        // discard old contacts
        for (auto it = c.begin(); it != c.end();) {
          if (node.current_time() - it->second > window)
            it = c.erase(it);
          else ++it;
        }
        // add new contacts
        field<device_t> nbr_uids = nbr_uid(node, 1);
        fold_hood(node, 2, [&](device_t i, int){
            c[i] = node.current_time();
            return 0;
        }, nbr_uids, 0);
        return c;
    });
    node.storage(positives{}) = nbr(node, 3, contact_t{}, [&](field<contact_t> np){
        contact_t p{};
        if (positive) p[node.uid] = node.current_time();
        fold_hood(node, 4, [&](contact_t const& cs, int){
            for (auto c : cs)
                if (node.current_time() - c.second < window)
                    p[c.first] = max(p[c.first], c.second);
            return 0;
        }, np, 0);
        return p;
    });
    node.storage(infected{}) = false;
    for (auto c : node.storage(positives{}))
        if (contacts.count(c.first))
            node.storage(infected{}) = true;
}

//! @brief Main aggregate function.
FUN() void case_study(ARGS) { CODE
    node.storage(round_count{}) = coordination::counter(node, 0, hops_t{1});
    node.storage(global_clock{}) = coordination::shared_clock(node, 1);
    node.storage(neigh_count{}) = count_hood(node, 2);
#if CASE_STUDY == VULNERABILITY_DETECTION
    vulnerability_detection(node, 3, DIAMETER);
#elif CASE_STUDY == CONTACT_TRACING
    contact_tracing(node, 4, WINDOW_TIME, miosix::userButton::value() == 0);
#endif
    resource_tracking(node, 5);
    topology_recording(node, 6);
}


//! @brief Main struct calling `test_program`.
MAIN(case_study,);

//! @brief FCPP setup.
DECLARE_OPTIONS(opt,
    program<main>,
    retain<metric::retain<5, 1>>, // messages are thrown away after 5/1 secs
    round_schedule<sequence::periodic_n<1, ROUND_PERIOD, ROUND_PERIOD, RECORD_TIME>>, // rounds are happening every 1 secs (den, start, period, end)
    exports< // types that may appear in messages
        bool, hops_t, device_t, int8_t, uint16_t, times_t, tuple<device_t, hops_t>,
        std::unordered_map<device_t, times_t>
    >,
    tuple_store< // tag/type that can appear in node.storage(tag{}) = type{}, are printed in output
        round_count,    int,
        global_clock,   times_t,
        neigh_count,    int,
        min_uid,        device_t,
        hop_dist,       hops_t,
        some_weak,      bool,
        max_stack,      uint16_t,
        max_heap,       uint32_t,
        max_msg,        int8_t,
        infected,       bool,
        positives,      std::unordered_map<device_t, times_t>,
        nbr_list,       std::unordered_set<device_t>
    >
);

//! @brief Main function starting FCPP.
int main() {
    LogStreambuf log(80*1024);
    std::ostream os(&log);
    component::deployment<opt>::net network{common::make_tagged_tuple<hoodsize, output>(device_t{DEGREE}, &os)};
    network.run();

    os.flush();
    while (true) {
        while (miosix::userButton::value() == 1);
        std::cout << "----" << std::endl << "log size " << log.size() << std::endl;
        log.dump();
    }
    return 0;
}
