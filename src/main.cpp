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
uint16_t getMaxStackUsed() {
    return MemoryProfiling::getStackSize() - MemoryProfiling::getAbsoluteFreeStack();
}

//! @brief \return the maximum heap used by the node (divided by 2 to fit in a short)
uint16_t getMaxHeapUsed() {
    return (MemoryProfiling::getHeapSize() - MemoryProfiling::getAbsoluteFreeHeap()) / 2;
}

// DEBUG STORAGE TAGS
struct nbr_counts {};
struct uid_list {};
struct elect_debug {};
struct nbr_elect {};
//! @brief Finds the minimum value, knowing an upper bound to the network diameter.
template <typename node_t, typename T>
T diameter_election_debug(node_t& node, trace_t call_point, const T& value, hops_t diameter) {
    internal::trace_call trace_caller(node.stack_trace, call_point);

    return get<0>(nbr(node, 0, make_tuple(value, hops_t{0}), [&](field<tuple<T,hops_t>> x){
        node.storage(nbr_elect{}) = x;
        tuple<T,hops_t> best = fold_hood(node, 0, [&](tuple<T,hops_t> const& a, tuple<T,hops_t> const& b){
            return get<1>(a) < diameter and a < b ? a : b;
        }, x, make_tuple(value, hops_t{0}));
        get<1>(best) += 1;
        return node.storage(elect_debug{}) = min(best, make_tuple(value, hops_t{0}));
    }));
}

//! @brief Finds the minimum UID, knowing an upper bound to the network diameter.
template <typename node_t>
inline device_t diameter_election_debug(node_t& node, trace_t call_point, int diameter) {
    return diameter_election_debug(node, call_point, node.uid, diameter);
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
//! @brief The list of positive devices in the network.
struct positives {};
//! @}

FUN(T) T fix_after(ARGS, T value, times_t t) { CODE
    return old(node, 0, value, [&](T o){
        return node.current_time() < t ? value : o;
    });
}

//! @brief Computes the maximum ever appeared in the network for a given value.
FUN(T) T max_ever(ARGS, T value) { CODE
    return nbr(node, 0, value, [&](field<T> neigh_vals){
        return max(coordination::max_hood(node, 1, neigh_vals), value);
    });
}

//! @brief Tracks the maximum consumption of memory and message resources.
FUN() void resource_tracking(ARGS) { CODE
    node.storage(max_stack{}) = max_ever(node, 0, getMaxStackUsed());
    node.storage(max_heap{}) = uint32_t{2} * max_ever(node, 1, getMaxHeapUsed());
    node.storage(max_msg{}) = max_ever(node, 2, (uint8_t)min(node.msg_size(), size_t{255}));
}

//! @brief Records the set of neighbours ever connected before RECORD_TIME.
FUN() void topology_recording(ARGS) { CODE
    using map_t = std::unordered_map<device_t,times_t>;
    field<device_t> nbr_uids = nbr_uid(node, 0);
    node.storage(uid_list{}) = nbr_uids;
    map_t nbr_counters = fix_after(node, 1, old(node, 2, map_t{}, [&](map_t n){
        fold_hood(node, 3, [&](device_t i, int){
            if (n.count(i) == 0) n[i] = 0;
            n[i] += 1;
            return 0;
        }, nbr_uids, 0);
        return n;
    }), RECORD_TIME);
    node.storage(nbr_counts{}) = nbr_counters;
    if (node.current_time() < RECORD_TIME) {
        node.storage(nbr_list{}) = {};
        for (const auto& it : nbr_counters)
            if (it.second >= nbr_counters[node.uid]/2)
                node.storage(nbr_list{}).insert(it.first);
    }
}

//! @brief Computes whether there is a node with only one connected neighbour at a given time.
FUN() void vulnerability_detection(ARGS, int diameter) { CODE
    node.storage(min_uid{}) = fix_after(node, 0, coordination::diameter_election_debug(node, 1, diameter), RECORD_TIME);
    node.storage(hop_dist{}) = fix_after(node, 2, coordination::abf_hops(node, 3, node.uid == node.storage(min_uid{})), RECORD_TIME);
    bool collect_weak = coordination::sp_collection(node, 4, node.storage(hop_dist{}), node.storage(neigh_count{}) <= 2, false, [](bool x, bool y) {
        return x or y;
    });
    node.storage(some_weak{}) = fix_after(node, 5, coordination::broadcast(node, 6, node.storage(hop_dist{}), collect_weak), RECORD_TIME);
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
    node.storage(neigh_count{}) = count_hood(node, 1);
#if CASE_STUDY == VULNERABILITY_DETECTION
    vulnerability_detection(node, 2, DIAMETER);
#elif CASE_STUDY == CONTACT_TRACING
    contact_tracing(node, 3, WINDOW_TIME, false); //TODO: change "false" with something that works
#endif
    resource_tracking(node, 4);
    topology_recording(node, 5);
}


//! @brief Main struct calling `test_program`.
MAIN(case_study,);

//! @brief FCPP setup.
DECLARE_OPTIONS(opt,
    program<main>,
    retain<metric::retain<15, 10>>, // messages are thrown away after 15/10 secs
    round_schedule<sequence::periodic_n<1, 1, 1>>, // rounds are happening every 1 secs (den, start, period)
    exports< // types that may appear in messages
        bool, hops_t, device_t, uint8_t, uint16_t, tuple<device_t, hops_t>,
        std::unordered_map<device_t, times_t>
    >,
    tuple_store< // tag/type that can appear in node.storage(tag{}) = type{}, are printed in output
        round_count,int,
        neigh_count,int,
        min_uid,    device_t,
        hop_dist,   hops_t,
        some_weak,  bool,
        max_stack,  uint16_t,
        max_heap,   uint32_t,
        max_msg,    uint8_t,
        infected,   bool,
        positives,  std::unordered_map<device_t, times_t>,
        nbr_elect,  field<tuple<device_t,hops_t>>,
        elect_debug,tuple<device_t,hops_t>,
        uid_list,   field<device_t>,
        nbr_counts, std::unordered_map<device_t, times_t>,
        nbr_list,   std::unordered_set<device_t>
    >
);

//! @brief Main function starting FCPP.
int main() {
    component::deployment<opt>::net network{common::make_tagged_tuple<hoodsize>(device_t{DEGREE})};
    network.run();
    return 0;
}
