// Copyright © 2022 Giorgio Audrito. All Rights Reserved.

#include "simulation.hpp"

/**
 * @brief Namespace containing all the objects in the FCPP library.
 */
namespace fcpp {
    //! @brief Reads a storage tuple row from a string stream (empty overload).
    template <typename R>
    inline void read_row(std::stringstream&, R&, common::type_sequence<>) {}

    //! @brief Reads a storage tuple row from a string stream.
    template <typename R, typename S, typename... Ss>
    inline void read_row(std::stringstream& ss, R& row, common::type_sequence<S, Ss...>) {
        double r;
        ss >> r;
        common::get<S>(row) = r;
        read_row(ss, row, common::type_sequence<Ss...>{});
    }

    //! @brief Feeds a storage tuple row into an aggregator tuple (empty overload).
    template <typename A, typename R>
    inline void aggregate_row(A&, R const&, common::type_sequence<>) {}

    //! @brief Feeds a storage tuple row into an aggregator tuple.
    template <typename A, typename R, typename S, typename... Ss>
    inline void aggregate_row(A& aggr, R const& row, common::type_sequence<S, Ss...>) {
        common::get<S>(aggr).insert(common::get<S>(row));
        aggregate_row(aggr, row, common::type_sequence<Ss...>{});
    }

    //! @brief Retrieves the result from an aggregator tuple (empty overload).
    template <typename R, typename A>
    inline void aggregate_result(R const&, A const&, common::type_sequence<>) {}

    //! @brief Retrieves the result from an aggregator tuple.
    template <typename R, typename A, typename S, typename... Ss>
    inline void aggregate_result(R& row, A const& aggr, common::type_sequence<S, Ss...>) {
        row = common::get<S>(aggr).template result<S>();
        aggregate_result(row, aggr, common::type_sequence<Ss...>{});
    }
}


//! @brief The main function.
int main() {
    using namespace fcpp;

    // The type of logged rows.
    using row_t = common::tagged_tuple_t<
        plot::time,         times_t,
        option::min_uid,    device_t,
        option::hop_dist,   hops_t,
        option::im_weak,    bool,
        option::some_weak,  bool,
        option::infector,   bool,
        option::infected,   bool,
        option::max_stack,  uint16_t,
        option::max_heap,   uint32_t,
        option::max_msg,    uint8_t,
        option::degree,     int8_t
    >;
    // Read rows from files.
    std::vector<std::deque<row_t>> rows;
    for (int x : std::vector<int>{0,3,5,9,10,11,12,13}) {
        rows.emplace_back();
        std::ifstream in("input/node" + std::to_string(x) + ".txt");
        std::string line;
        bool first = true;
        while (std::getline(in, line)) {
            if (line[0] == '#') continue;
            if (first) {
                first = false;
                continue;
            }
            rows.back().emplace_back();
            std::stringstream ss(line);
            read_row(ss, rows.back().back(), typename row_t::tags{});
        }
    }

    // Sequence of storage tags and corresponding aggregator types.
    using aggr_t = common::tagged_tuple_t<
        option::min_uid,        aggregator::mean<double>,
        option::hop_dist,       aggregator::mean<double>,
        option::im_weak,        aggregator::mean<double>,
        option::some_weak,      aggregator::mean<double>,
        option::infected,       aggregator::mean<double>,
        option::infector,       aggregator::mean<double>,
        option::degree,         aggregator::combine<aggregator::min<int>, aggregator::mean<double>, aggregator::max<int>>
    >;
    // The plotter object.
    option::plotter_t p;
    // The row type used for plotter.
    using plot_row_t = typename component::interactive_simulator<option::simulation>::net::row_type;
    // The row used for plotter.
    plot_row_t pr;
    // Aggregate data per time.
    for (int t = 1; ; ++t) {
        aggr_t aggr;
        bool all_empty = true;
        for (auto& row : rows) {
            if (row.empty()) continue;
            all_empty = false;
            if (common::get<plot::time>(row.front()) > t) continue;
            row_t r;
            while (row.size() > 0 and common::get<plot::time>(row.front()) <= t) {
                r = std::move(row.front());
                row.pop_front();
            }
            aggregate_row(aggr, r, typename aggr_t::tags{});
        }
        if (all_empty) break;
        common::get<plot::time>(pr) = t;
        aggregate_result(pr, aggr, typename aggr_t::tags{});
        p << pr;
    }
    // Write plots.
    std::cout << plot::file("plotter", p.build());
    return 0;
}
