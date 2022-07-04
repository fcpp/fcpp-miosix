// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

#define RUN_VULNERABILITY_DETECTION
#define RUN_CONTACT_TRACING

#include "main.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {

//! @brief Number of devices in the building.
constexpr size_t device_num = 20;

//! @brief Dimensionality of the space.
constexpr size_t dim = 3;

// PURE C++ FUNCTIONS

//! @brief The maximum stack used by the node starting from the boot
inline uint16_t usedStack() {
    return 0;
}

//! @brief The maximum heap used by the node (divided by 2 to fit in a short)
inline uint16_t usedHeap() {
    return 0;
}

//! @brief Whether the button is currently pressed.
inline bool buttonPressed(device_t uid, times_t) {
    return uid == 0 and rand() % 64 == 42;
}

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

//! @brief Tags used in the node storage.
namespace tags {
    //! @brief Color representing the STUFF.
    struct col {};
    //! @brief Size of the current node (WHAT).
    struct size {};
    //! @brief Whether the device is the initiator of an infection.
    struct infector {};
    //! @brief The degree of the node.
    struct degree {};
}

//! @brief Handle for simulation code.
FUN void simulation_handle(ARGS) { CODE
    using namespace tags;
    node.storage(col{}) = color::hsva(node.storage(hop_dist{})*360/DIAMETER,1,1);
    int s = node.storage(some_weak{}) + node.storage(infected{});
    node.storage(size{}) = s == 2 ? 0.8 : s == 1 ? 0.5 : 0.3;
    // use two devices to force a correct automatic grid size
    if (node.uid < 2) {
        if (node.storage(round_count{}) == 1) {
            node.next_time(node.next_real(5*device_num));
            node.storage(size{}) = 0;
            if (node.uid == 0) node.position() = make_vec(0.9, 0.9, 0);
            if (node.uid == 1) node.position() = make_vec(23.1, 14.1, 0);
        }
        if (node.storage(round_count{}) == 2) {
            node.position() = random_rectangle_target(CALL, make_vec(7, 1, 1), make_vec(11, 5, 1));
        }
    }
    int column = constant(CALL, (int8_t)node.next_int(0, 3));
    int row = constant(CALL, (int8_t)node.next_int(0, 1));
    vec<dim> mid = constant(CALL, random_rectangle_target(CALL, make_vec(1+6*column, 1+9*row, 1), make_vec(5+6*column, 5+9*row, 1)));
    vec<dim> end = constant(CALL, random_rectangle_target(CALL, make_vec(13, 10, 1), make_vec(17, 14, 1)));
    times_t t;
    t = constant(CALL, node.next_real(5*device_num, 10*device_num));
    if (t <= node.current_time() and node.current_time() <= 15*device_num) {
        std::array<vec<dim>, 5> path = {
            make_vec(9, 5.5, 2),
            make_vec(9, 7, 2),
            make_vec(3+6*column, 7+row, 2),
            make_vec(3+6*column, 5.5+4*row, 2),
            mid
        };
        follow_path(CALL, path, 1.4, 1);
    }
    t = constant(CALL, node.next_real(15*device_num, 20*device_num));
    if (t <= node.current_time() and node.current_time() <= 25*device_num) {
        std::array<vec<dim>, 5> path = {
            make_vec(3+6*column, 5.5+4*row, 2),
            make_vec(3+6*column, 7+row, 2),
            make_vec(9, 7, 2),
            make_vec(9, 5.5, 2),
            end
        };
        follow_path(CALL, path, 1.4, 1);
    }
    t = constant(CALL, node.next_real(500-5*device_num, 500));
    if (node.current_time() > t) node.terminate();
    node.storage(infector{}) = (node.storage(infected{}) == 2);
    node.storage(infected{}) = (node.storage(infected{}) > 0);
    node.storage(degree{}) = node.size() - 1;
}
FUN_EXPORT simulation_handle_t = export_list<constant_t<vec<dim>>, constant_t<real_t>, follow_path_t>;

} // namespace coordination


//! @brief Namespace for component options.
namespace option {

//! @brief Description of the export schedule.
using export_s = sequence::periodic_n<1, 0, 1, 500>;

//! @brief Description of the sequence of node creation events.
using spawn_s = sequence::merge<
    sequence::multiple_n<2, 0>,
    sequence::multiple<
        distribution::constant_n<size_t, device_num-2>,
        distribution::interval_n<times_t, 0, 5*device_num>,
        false
    >
>;

//! @brief Description of the initial position distribution.
using rectangle_d = distribution::rect_n<1, 7, 1, 1, 11, 5, 1>;

//! @brief Additional storage tags and types.
using storage_t = tuple_store<
    col,        color,
    size,       double,
    infector,   bool,
    degree,     int
>;

//! @brief Storage tags to be logged with aggregators.
using aggregator_t = aggregators<
    min_uid,        aggregator::mean<double>,
    hop_dist,       aggregator::mean<double>,
    some_weak,      aggregator::mean<double>,
    degree,         aggregator::combine<aggregator::min<int>, aggregator::mean<int>, aggregator::max<int>>,
    infected,       aggregator::mean<double>,
    infector,       aggregator::mean<double>,
    max_msg,        aggregator::mean<double>,
    strongest_link, aggregator::mean<double>
>;

//! @brief Plotting variables over time.
template <typename... Ts>
using time_plot_t = plot::split<plot::time, plot::values<aggregator_t, common::type_sequence<>, Ts...>>;

//! @brief Overall plot description.
using plotter_t = plot::join<time_plot_t<some_weak>, time_plot_t<degree>, time_plot_t<infected, infector>, time_plot_t<max_msg, strongest_link>, time_plot_t<min_uid, hop_dist>>;

//! @brief Main FCPP option setup.
DECLARE_OPTIONS(simulation,
    main,
    exports<coordination::simulation_handle_t>,
    parallel<false>,
    synchronised<false>,
    message_size<true>,
    dimension<dim>,
    exports<vec<dim>, size_t>,
    connector<connect::radial<70, connect::fixed<12, 1, dim>>>,
    log_schedule<export_s>,
    spawn_schedule<spawn_s>,
    init<x, rectangle_d>,
    storage_t,
    aggregator_t,
    plot_type<plotter_t>,
    size_tag<size>,
    color_tag<col>
);

} // namespace option

} // namespace fcpp
