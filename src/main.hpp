// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

#include "lib/fcpp.hpp"

#define DEGREE       10  // maximum degree allowed for a deployment
#define DIAMETER     10  // maximum diameter in hops for a deployment
#define WINDOW_TIME  60  // time in seconds during which positive node information is retained
#define PRESS_TIME   5   // time in seconds of button press after which termination is triggered
#define ROUND_PERIOD 1   // time in seconds between transmission rounds
#define BUFFER_SIZE  40  // size in KB to be used for buffering the output

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

// PURE C++ FUNCTIONS

//! @brief The maximum stack used by the node starting from the boot
inline uint16_t usedStack();

//! @brief The maximum heap used by the node (divided by 2 to fit in a short)
inline uint16_t usedHeap();

//! @brief Whether the button is currently pressed.
inline bool buttonPressed(fcpp::device_t, fcpp::times_t);


//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

//! @brief Tags used in the node storage.
namespace tags {
    //! @brief Total round count since start.
    struct round_count {};
    //! @brief A shared global clock.
    struct global_clock {};
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
    //! @brief Percentage of transmission success for the strongest link.
    struct strongest_link {};
    //! @brief List of neighbours encountered at least 50% of the times.
    struct nbr_list {};
    //! @brief Whether the device has been infected.
    struct infected {};
    //! @brief The list of contacts met in the last period of time.
    struct contacts {};
    //! @brief The list of positive devices in the network.
    struct positives {};
}


// AGGREGATE STATUS TRACKING

//! @brief Tracks the passage of time.
FUN void time_tracking(ARGS) { CODE
    using namespace tags;
    node.storage(round_count{}) = counter(CALL, uint16_t{1});
    node.storage(global_clock{}) = shared_clock(CALL);
}
FUN_EXPORT time_tracking_t = export_list<counter_t<uint16_t>, shared_clock_t>;

//! @brief Tracks the maximum consumption of memory and message resources.
FUN void resource_tracking(ARGS) { CODE
    using namespace tags;
    node.storage(max_stack{}) = gossip_max(CALL, usedStack());
    node.storage(max_heap{}) = uint32_t{2} * gossip_max(CALL, usedHeap());
    node.storage(max_msg{}) = gossip_max(CALL, (int8_t)min(node.msg_size(), size_t{127}));
}
FUN_EXPORT resource_tracking_t = export_list<gossip_max_t<uint16_t>, gossip_max_t<uint8_t>>;

//! @brief Records the set of neighbours connected at least 50% of the time.
FUN void topology_recording(ARGS) { CODE
    node.storage(tags::nbr_list{}).clear();
    list_hood(CALL, node.storage(tags::nbr_list{}), nbr_uid(CALL), nothing);

    using map_t = std::unordered_map<device_t,times_t>;
    map_t nbr_counters = old(CALL, map_t{}, [&](map_t n){
        fold_hood(CALL, [&](device_t i, times_t t, tags::nothing){
            if (t > node.previous_time()) n[i] += 1;
            return nothing;
        }, node.message_time(), nothing);
        return n;
    });
    times_t c = 0;
    for (const auto& it : nbr_counters)
        c = max(c, it.second);
    c = c * 100 / node.storage(tags::round_count{});
    node.storage(tags::strongest_link{}) = (int8_t)round(c);
}
FUN_EXPORT topology_recording_t = export_list<std::unordered_map<device_t,times_t>>;

//! @brief Checks whether to terminate the execution.
FUN void termination_check(ARGS) { CODE
    if (round_since(CALL, not buttonPressed(node.uid, node.storage(tags::global_clock{}))) >= PRESS_TIME) node.terminate();
}
FUN_EXPORT termination_check_t = export_list<round_since_t>;


// AGGREGATE CASE STUDIES

//! @brief Computes whether there is a node with only one connected neighbour at a given time.
FUN void vulnerability_detection(ARGS, int diameter) { CODE
    using namespace tags;
    tie(node.storage(min_uid{}), node.storage(hop_dist{})) = diameter_election_distance(CALL, diameter);
    bool collect_weak = sp_collection(CALL, node.storage(hop_dist{}), count_hood(CALL) <= 2, false, [](bool x, bool y) {
        return x or y;
    });
    node.storage(some_weak{}) = broadcast(CALL, node.storage(hop_dist{}), collect_weak);
}
FUN_EXPORT vulnerability_detection_t = export_list<diameter_election_distance_t<>, sp_collection_t<hops_t, bool>, broadcast_t<hops_t, bool>>;

//! @brief Computes whether the current node got in contact with a positive node within a time window.
FUN void contact_tracing(ARGS, times_t window) { CODE
    using namespace tags;
    bool first_case = toggle_filter(CALL, buttonPressed(node.uid, node.storage(global_clock{})));
    bool positive = first_case;
    using contact_t = std::unordered_map<device_t, times_t>;
    node.storage(contacts{}) = old(CALL, contact_t{}, [&](contact_t c){
        // discard old contacts
        for (auto it = c.begin(); it != c.end();) {
          if (node.current_time() - it->second > window)
            it = c.erase(it);
          else ++it;
        }
        // add new contacts
        field<device_t> nbr_uids = nbr_uid(CALL);
        fold_hood(CALL, [&](device_t i, int){
            c[i] = node.current_time();
            return 0;
        }, nbr_uids, 0);
        return c;
    });
    node.storage(positives{}) = nbr(CALL, contact_t{}, [&](field<contact_t> np){
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
    node.storage(infected{}) = first_case ? 2 : positive ? 1 : 0;
    for (auto c : node.storage(positives{}))
        if (node.storage(contacts{}).count(c.first))
            node.storage(infected{}) = true;
}
FUN_EXPORT contact_tracing_t = export_list<toggle_filter_t, std::unordered_map<device_t, times_t>>;


// AGGREGATE MAIN

//! @brief Handle for simulation code.
FUN void simulation_handle(ARGS);

//! @brief Main aggregate function.
MAIN() {
    time_tracking(CALL);
#ifdef RUN_VULNERABILITY_DETECTION
    vulnerability_detection(CALL, DIAMETER);
#endif
#ifdef RUN_CONTACT_TRACING
    contact_tracing(CALL, WINDOW_TIME);
#endif
    resource_tracking(CALL);
    topology_recording(CALL);
    termination_check(CALL);
    simulation_handle(CALL);
}
FUN_EXPORT main_t = export_list<
#ifdef RUN_VULNERABILITY_DETECTION
    vulnerability_detection_t,
#endif
#ifdef RUN_CONTACT_TRACING
    contact_tracing_t,
#endif
    time_tracking_t, resource_tracking_t, topology_recording_t, termination_check_t
>;

} // namespace coordination


//! @brief Namespace for component options.
namespace option {

//! @brief Import tags to be used for component options.
using namespace component::tags;
//! @brief Import tags used by aggregate functions.
using namespace coordination::tags;

//! @brief Dictates that messages are thrown away after 5/1 seconds.
using retain_type = retain<metric::retain<5, 1>>;

//! @brief Dictates that rounds are happening every 1 seconds (den, start, period).
using schedule_type = round_schedule<sequence::periodic_n<1, ROUND_PERIOD, ROUND_PERIOD>>;

//! @brief Tag-type pairs that can appear in node.storage(tag{}) = type{} expressions (are all printed in output).
using store_type = tuple_store<
    round_count,    uint16_t,
    global_clock,   times_t,
    min_uid,        device_t,
    hop_dist,       hops_t,
    some_weak,      bool,
    infected,       int8_t,
    contacts,       std::unordered_map<device_t, times_t>,
    positives,      std::unordered_map<device_t, times_t>,
    max_stack,      uint16_t,
    max_heap,       uint32_t,
    max_msg,        int8_t,
    strongest_link, int8_t,
    nbr_list,       std::vector<device_t>
>;

//! @brief Main FCPP option setup.
DECLARE_OPTIONS(main,
    program<coordination::main>,
    exports<coordination::main_t>,
    retain_type,
    schedule_type,
    store_type
);

} // namespace option

} // namespace fcpp
