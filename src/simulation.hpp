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

//! @brief The length of the main simulated time epochs.
constexpr size_t time_frame = 5*device_num;

//! @brief The time of simulation end.
constexpr size_t end_time = 5*time_frame;

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
inline bool buttonPressed(device_t uid, uint16_t t) {
    return uid == 0 and (t == 40 or t == 80 or t == 280 or t == 290);
}

//! @brief Turn on or off the red LED.
inline void setRedLed(bool) {}

//! @brief Namespace containing the libraries of coordination routines.
namespace coordination {

//! @brief Tags used in the node storage.
namespace tags {
    //! @brief Color representing the hop_dist from the min_uid.
    struct col {};
    //! @brief Size of the current node (larger if some_weak or infected).
    struct size {};
    //! @brief The logging buffer object.
    struct log_buffer {};
    //! @brief The size of the logging buffer object.
    struct log_buffer_size{};
    //! @brief The length of the logging buffer object.
    struct log_buffer_len{};
}

//! @brief Handle for simulation code.
FUN void simulation_handle(ARGS) { CODE
    using namespace tags;
    node.storage(col{}) = color::hsva(node.storage(hop_dist{})*360/DIAMETER,1,1);
    int s = node.storage(some_weak{}) + node.storage(infected{});
    node.storage(size{}) = s == 2 ? 0.8 : s == 1 ? 0.5 : 0.3;
    node.storage(log_buffer{}) << node.storage_tuple();
    node.storage(log_buffer_size{}) = node.storage(log_buffer{}).byte_size();
    node.storage(log_buffer_len{}) = node.storage(log_buffer{}).size();

    int column = constant(CALL, (int8_t)node.next_int(0, 3));
    int row = constant(CALL, (int8_t)node.next_int(0, 1));
    vec<dim> mid = constant(CALL, random_rectangle_target(CALL, make_vec(1+6*column, 1+9*row, 1), make_vec(5+6*column, 5+9*row, 1)));
    vec<dim> end = constant(CALL, random_rectangle_target(CALL, make_vec(13, 10, 1), make_vec(17, 14, 1)));
    times_t t;
    t = constant(CALL, node.next_real(time_frame, 2*time_frame));
    if (t <= node.current_time() and node.current_time() <= 3*time_frame) {
        std::array<vec<dim>, 5> path = {
            make_vec(9, 5.5, 2),
            make_vec(9, 7, 2),
            make_vec(3+6*column, 7+row, 2),
            make_vec(3+6*column, 5.5+4*row, 2),
            mid
        };
        follow_path(CALL, path, 1.4, 1);
    }
    t = constant(CALL, node.next_real(3*time_frame, 4*time_frame));
    if (t <= node.current_time() and node.current_time() <= 5*time_frame) {
        std::array<vec<dim>, 5> path = {
            make_vec(3+6*column, 5.5+4*row, 2),
            make_vec(3+6*column, 7+row, 2),
            make_vec(9, 7, 2),
            make_vec(9, 5.5, 2),
            end
        };
        follow_path(CALL, path, 1.4, 1);
    }
    t = constant(CALL, node.next_real(4*time_frame, 5*time_frame));
    if (node.current_time() > t) node.terminate();
}
FUN_EXPORT simulation_handle_t = export_list<constant_t<vec<dim>>, constant_t<real_t>, follow_path_t>;

} // namespace coordination


//! @brief Namespace for component options.
namespace option {

//! @brief Description of the export schedule.
using export_s = sequence::periodic_n<1, 0, 1, end_time>;

//! @brief Description of the sequence of node creation events.
using spawn_s = sequence::multiple<
    distribution::constant_n<size_t, device_num>,
    distribution::interval_n<times_t, 0, time_frame>,
    false
>;

//! @brief Description of the initial position distribution.
using rectangle_d = distribution::rect_n<1, 7, 1, 1, 11, 5, 1>;

//! @brief Additional storage tags and types.
using storage_t = tuple_store<
    col,                color,
    size,               double,
    log_buffer,         rows_type,
    log_buffer_size,    size_t,
    log_buffer_len,     size_t
>;

//! @brief Storage tags to be logged with aggregators.
using aggregator_t = aggregators<
    min_uid,        aggregator::mean<double>,
    hop_dist,       aggregator::mean<double>,
    im_weak,        aggregator::mean<double>,
    some_weak,      aggregator::mean<double>,
    infected,       aggregator::mean<double>,
    infector,       aggregator::mean<double>,
    degree,         aggregator::combine<aggregator::min<int>, aggregator::mean<double>, aggregator::max<int>>,
    max_msg,        aggregator::mean<double>,
    log_buffer_size,aggregator::combine<aggregator::max<int>, aggregator::mean<double>>,
    log_buffer_len, aggregator::combine<aggregator::mean<double>, aggregator::max<int>>
>;

//! @brief Plotting variables over time.
template <typename... Ts>
using time_plot_t = plot::split<plot::time, plot::values<aggregator_t, common::type_sequence<>, Ts...>>;

//! @brief Overall plot description.
using plotter_t = plot::join<time_plot_t<im_weak, some_weak>, time_plot_t<degree>, time_plot_t<infected, infector>>;

//! @brief Main FCPP option setup.
DECLARE_OPTIONS(simulation,
    main,
    exports<coordination::simulation_handle_t>,
    parallel<true>,
    synchronised<false>,
    message_size<true>,
    dimension<dim>,
    connector<connect::radial<70, connect::fixed<12, 1, dim>>>,
    log_schedule<export_s>,
    spawn_schedule<spawn_s>,
    init<x, rectangle_d>,
    storage_t,
    aggregator_t,
    plot_type<plotter_t>,
    size_tag<size>,
    color_tag<col>,
    area<0, 0, 24, 15>
);

} // namespace option

} // namespace fcpp
