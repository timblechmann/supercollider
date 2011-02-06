#include <boost/test/unit_test.hpp>

#include "server/node_graph.hpp"

#include "test_synth.hpp"

using namespace nova;
using namespace std;

BOOST_AUTO_TEST_CASE( satellite_node_test_1 )
{
    rt_pool.init(1024 * 1024);

    node_graph n;

    test_synth * s1 = new test_synth(1000, 0);
    n.add_node(s1);

    test_synth * s2 = new test_synth(1001, 0);
    {
        node_position_constraint position = std::make_pair(s1, satellite_before);
        n.add_node(s2, position);
        BOOST_REQUIRE(s1->has_satellite());
        BOOST_REQUIRE(s1->has_satellite_predecessor());
    }

    BOOST_REQUIRE_EQUAL(n.synth_count(), 2u);

    test_synth * s3 = new test_synth(1002, 0);
    {
        node_position_constraint position = std::make_pair(s1, satellite_after);
        n.add_node(s3, position);
        BOOST_REQUIRE(s1->has_satellite());
        BOOST_REQUIRE(s1->has_satellite_successor());
    }

    BOOST_REQUIRE_EQUAL(n.synth_count(), 3u);

    {
        auto_ptr<node_graph::dsp_thread_queue> q = n.generate_dsp_queue();
        BOOST_REQUIRE_EQUAL(q->get_total_node_count(), 3u);
    }

    n.remove_node(s1);
    BOOST_REQUIRE_EQUAL(n.synth_count(), 0);
}

BOOST_AUTO_TEST_CASE( satellite_node_test_2 )
{
    node_graph n;

    test_synth * s1 = new test_synth(1000, 0);
    n.add_node(s1);

    test_synth * s2 = new test_synth(999, 0);
    {
        node_position_constraint position = std::make_pair(s1, before);
        n.add_node(s2, position);
    }

    test_synth * s3 = new test_synth(1001, 0);
    {
        node_position_constraint position = std::make_pair(s1, after);
        n.add_node(s3, position);
    }

    test_synth * sat1 = new test_synth(1009, 0);
    {
        node_position_constraint position = std::make_pair(s1, satellite_before);
        n.add_node(sat1, position);
    }

    test_synth * sat2 = new test_synth(1011, 0);
    {
        node_position_constraint position = std::make_pair(s1, satellite_after);
        n.add_node(sat2, position);
    }

    {
        auto_ptr<node_graph::dsp_thread_queue> q = n.generate_dsp_queue();
        BOOST_REQUIRE_EQUAL(q->get_total_node_count(), 5u);
    }

    n.remove_node(s1);
    BOOST_REQUIRE_EQUAL(n.synth_count(), 2);
}

