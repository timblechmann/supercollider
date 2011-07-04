//  dependency graph generator
//  Copyright (C) 2010 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef SERVER_DEPENDENCY_GRAPH_GENERATOR_HPP
#define SERVER_DEPENDENCY_GRAPH_GENERATOR_HPP

#include "node_graph.hpp"

#ifdef BOOST_HAS_RVALUE_REFS
#define MOVE(X) std::move(X)
#else
#define MOVE(X) X
#endif

namespace nova
{

/** node graph parser class
 *
 *  this class generates a dsp thread queue from the node graph hierarchy
 *
 *  the basic idea is to recusively parse the tree from the back to the front
 *
 * */
class dependency_graph_generator
{
    typedef node_graph::dsp_thread_queue dsp_thread_queue;

    typedef nova::dsp_thread_queue_item<dsp_queue_node<rt_pool_allocator<void*> >,
                                        rt_pool_allocator<void*>
                                       > thread_queue_item;

    typedef thread_queue_item::successor_list successor_container;
    typedef std::vector<server_node*, rt_pool_allocator<abstract_synth*> > sequential_child_list;
    typedef std::vector<thread_queue_item*, rt_pool_allocator<void*> > rt_item_vector;
    typedef std::size_t size_t;

public:
    dsp_thread_queue * operator()(node_graph * graph)
    {
        /* pessimize: reserve enough memory for the worst case */
        q = new node_graph::dsp_thread_queue(graph->synth_count_);

        fill_queue(*graph->root_group());
#ifndef NDEBUG
        q->validate_queue();
#endif
        return q;
    }

private:
    dsp_thread_queue * q;
    successor_container empty_successor_container;

    void fill_queue(group & root_group)
    {
        if (root_group.has_synth_children())
            fill_queue_recursive(root_group, empty_successor_container, 0);
    }

    template <typename reverse_iterator>
    static inline size_t get_previous_activation_count(reverse_iterator it, const reverse_iterator end,
                                                       size_t head_activation_limit)
    {
        reverse_iterator prev = it;
        const reverse_iterator last = iterator_sub(end, 1);

        size_t collected_satellites = 0;
        for (;;)
        {
            ++prev;
            if (prev == end)
                 // we are the first item, so we use the head activiation limit
                return head_activation_limit + collected_satellites;

            server_node const & node = *prev;
            if (node.is_synth())
                return collected_satellites + 1;
            else {
                abstract_group const & grp = static_cast<abstract_group const &>(node);

                if (grp.empty()) {
                    collected_satellites += count_satellite_predecessor_nodes(grp);
                    continue;
                }
                size_t tail_nodes = count_tail_nodes(grp);

                return tail_nodes + collected_satellites;
            }
        }
    }

    successor_container fill_queue_recursive(abstract_group & grp, successor_container const & successors,
                                             size_t activation_limit)
    {
        if (!grp.has_synth_children())
            return handle_empty_group(grp, successors, activation_limit);

        if (grp.is_parallel())
            return fill_queue_recursive(static_cast<parallel_group&>(grp), successors, activation_limit);
        else
            return fill_queue_recursive(static_cast<group&>(grp), successors, activation_limit);
    }

    successor_container fill_queue_recursive(group & g, successor_container const & successors_from_parent,
                                             size_t head_activation_limit)
    {
        assert (g.has_synth_children());

        typedef server_node_list::reverse_iterator r_iterator;

        successor_container successors(successors_from_parent);

        size_t children = g.child_count();

        sequential_child_list sequential_children;
        sequential_children.reserve(g.child_synths_);

        for (r_iterator it = g.child_nodes.rbegin(); it != g.child_nodes.rend(); ++it)
        {
            server_node & node = *it;

            if (node.is_synth())
                sequential_group_handle_synth(it, sequential_children, g, head_activation_limit, successors, children);
            else
                sequential_group_handle_group(it, node, g, children, head_activation_limit, successors);
        }
        assert(children == 0);
        return successors;
    }

    template <typename iterator>
    static iterator iterator_add(iterator it, std::ptrdiff_t count)
    {
        assert(count > 0);
        for (std::ptrdiff_t i = 0; i != count; ++i)
            ++it;
        return it;
    }

    template <typename iterator>
    static iterator iterator_sub(iterator it, std::ptrdiff_t count)
    {
        assert(count > 0);
        for (std::ptrdiff_t i = 0; i != count; ++i)
            --it;
        return it;
    }

    void sequential_group_handle_synth(server_node_list::reverse_iterator & it, sequential_child_list & sequential_children,
                                       group & g, size_t previous_activation_limit, successor_container & successors,
                                       size_t & children)
    {
        typedef server_node_list::reverse_iterator r_iterator;
        const r_iterator last_node = it;

        // we fill the child nodes in reverse order to an array
        const r_iterator first_node = find_synth_sequence_start(it, g.child_nodes, sequential_children);
        const r_iterator it_before = iterator_add(first_node, 1);

        size_t activation_limit;

        if (it_before == g.child_nodes.rend())
            // we reached the head of the group
            activation_limit = previous_activation_limit;
        else if (it_before->is_group()) {
            // we reached a child group
            activation_limit = get_previous_activation_count(it, g.child_nodes.rend(), previous_activation_limit);

        } else
            // before this node, there is a synth
            activation_limit = 1;

        if (first_node->has_satellite_predecessor())
            activation_limit += count_satellite_predecessor_nodes(*first_node);

        size_t node_count = sequential_children.size();
        sequential_child_list::reverse_iterator seq_it = sequential_children.rbegin();

        thread_queue_item * q_item;

        if (last_node->has_satellite_successor()) {
            successor_container satellites = fill_satellite_successors(last_node->satellite_successors, 1);
            successor_container combined_successors = concat_successors(satellites, successors);

            q_item = q->allocate_queue_item(queue_node(MOVE(queue_node_data(static_cast<abstract_synth*>(*seq_it++))), node_count),
                                            combined_successors, activation_limit);
        } else
            q_item = q->allocate_queue_item(queue_node(MOVE(queue_node_data(static_cast<abstract_synth*>(*seq_it++))), node_count),
                                            successors, activation_limit);

        queue_node & q_node = q_item->get_job();

        // now we can add all nodes sequentially
        for (;seq_it != sequential_children.rend(); ++seq_it)
            q_node.add_node(static_cast<abstract_synth*>(*seq_it));
        sequential_children.clear();

        /* advance successor list */
        successors = successor_container(1);
        successors[0] = q_item;

        if (first_node->has_satellite_predecessor())
            fill_satellite_predecessors(first_node->satellite_predecessors, successors);

        if (activation_limit == 0)
            q->add_initially_runnable(q_item);
        children -= node_count;
        it = first_node;
    }

    /* finds a sequence of synths, that can be combined in one queue item
     *
     * fills sequential nodes into sequential_children argument (reverse order)
     * returns iterator to the first element
     * */
    static server_node_list::reverse_iterator
    find_synth_sequence_start(server_node_list::reverse_iterator it,
                              server_node_list & child_nodes,
                              sequential_child_list & sequential_children)
    {
        sequential_children.push_back(&*it);
        server_node_list::reverse_iterator previous = it;
        ++it;

        for (;;)
        {
            if (it == child_nodes.rend())
                return previous; // we found the beginning of this group

            if (it->has_satellite_successor())
                return previous; // synths with satellite successors should be part of the previous queue item

            if (!it->is_synth())
                return previous; // we hit a child group, later we may want to add it's nodes, too?

            if (previous->has_satellite_predecessor())
                // synths with satellite predecessors are the head of this queue item
                return previous;

            // we consume this synth
            sequential_children.push_back(&*it);

            // advance
            previous = it;
            ++it;
        }
    }

    void sequential_group_handle_group(server_node_list::reverse_iterator const & it,
                                       server_node & node, group & g,
                                       size_t & children, size_t head_activation_limit,
                                       successor_container & successors)
    {
        abstract_group & grp = static_cast<abstract_group&>(node);

        size_t activation_limit = get_previous_activation_count(it, g.child_nodes.rend(), head_activation_limit)
                                  + count_satellite_predecessor_nodes(node);

        if (grp.has_synth_children()) {
            if (node.has_satellite_successor())
            {
                successor_container satellites = fill_satellite_successors(node.satellite_successors,
                                                 count_tail_nodes(grp));
                successor_container group_successors = concat_successors(satellites, successors);

                successors = fill_queue_recursive(grp, group_successors, activation_limit);
            } else
                successors = fill_queue_recursive(grp, successors, activation_limit);

            if (node.has_satellite_predecessor())
                fill_satellite_predecessors(node.satellite_predecessors, successors);
        } else
            successors = handle_empty_group(grp, successors, activation_limit);

        children -= 1;
    }

    successor_container handle_empty_group(abstract_group & grp, successor_container const & successors,
                                           size_t head_activation_limit)
    {
        successor_container ret = successors;

        if (grp.has_satellite_successor()) {
            successor_container satellites = fill_satellite_successors(grp.satellite_successors, head_activation_limit);
            ret = concat_successors(successors, satellites);
        }

        if (grp.has_satellite_predecessor())
            fill_satellite_predecessors(grp.satellite_predecessors, ret);

        return ret;
    }

    successor_container fill_queue_recursive(parallel_group & g, successor_container const & successors_from_parent,
            size_t previous_activation_limit)
    {
        assert (g.has_synth_children());
        size_t reserve_elements = g.child_count() + 16; // pessimize
        return collect_parallel_nodes(g.child_nodes, successors_from_parent, previous_activation_limit, reserve_elements);
    }

    successor_container
    fill_satellite_successors(server_node_list const & satellite_successors, size_t activation_limit)
    {
        return collect_parallel_nodes(satellite_successors, empty_successor_container, activation_limit, 16);
    }

    successor_container
    collect_parallel_nodes(server_node_list const & parallel_nodes, successor_container const & successors,
                           const size_t activation_limit, size_t elements_to_reserve)
    {
        rt_item_vector collected_nodes;
        collected_nodes.reserve(elements_to_reserve);

        for (server_node_list::const_iterator it = parallel_nodes.begin();
                it != parallel_nodes.end(); ++it)
        {
            server_node & node = const_cast<server_node &>(*it);
            const size_t this_activation_limit = activation_limit + count_satellite_predecessor_nodes(node);

            if (node.is_synth())
                parallel_nodes_handle_synth(node, successors, collected_nodes, this_activation_limit);
            else
                parallel_nodes_handle_group(node, successors, collected_nodes, this_activation_limit);
        }

        successor_container ret(collected_nodes.size(), collected_nodes.data());
        return ret;
    }

    void parallel_nodes_handle_synth(server_node & node, successor_container const & successors,
                                     rt_item_vector & collected_nodes, size_t activation_limit)
    {
        thread_queue_item * q_item;
        if (node.has_satellite_successor()) {
            successor_container satellites = fill_satellite_successors(node.satellite_successors, 1);

            q_item = q->allocate_queue_item(queue_node(MOVE(queue_node_data(static_cast<abstract_synth*>(&node)))),
                                            concat_successors(successors, satellites),
                                            activation_limit);
        } else {
            q_item = q->allocate_queue_item(queue_node(MOVE(queue_node_data(static_cast<abstract_synth*>(&node)))),
                                            successors, activation_limit);
        }

        if (activation_limit == 0)
            q->add_initially_runnable(q_item);

        if (node.has_satellite_predecessor())
        {
            successor_container node_as_successor(1);
            node_as_successor[0] = q_item;
            fill_satellite_predecessors(node.satellite_predecessors, node_as_successor);
        }

        collected_nodes.push_back(q_item);
    }

    void parallel_nodes_handle_group(server_node & node, successor_container const & successors,
                                     rt_item_vector & collected_nodes, size_t activation_limit)
    {
        abstract_group & grp = static_cast<abstract_group &>(node);
        if (!grp.has_synth_children()) {
            if (grp.has_satellite_successor()) {
                successor_container satellites = fill_satellite_successors(grp.satellite_successors,
                                                 activation_limit);

                for (size_t i = 0; i != satellites.size(); ++i)
                    collected_nodes.push_back(satellites[i]);
            }

            if (grp.has_satellite_predecessor())
                fill_satellite_predecessors(grp.satellite_predecessors, successors);
        }

        if (grp.has_satellite_successor()) {
            successor_container satellites = fill_satellite_successors(grp.satellite_successors,
                                             count_tail_nodes(grp));
            successor_container group_successors = fill_queue_recursive(grp,
                                                   concat_successors(successors, satellites),
                                                   activation_limit);

            for (size_t i = 0; i != group_successors.size(); ++i)
                collected_nodes.push_back(group_successors[i]);

            if (grp.has_satellite_predecessor())
                fill_satellite_predecessors(grp.satellite_predecessors, group_successors);

        } else {
            successor_container group_successors = fill_queue_recursive(grp, successors, activation_limit);

            for (size_t i = 0; i != group_successors.size(); ++i)
                collected_nodes.push_back(group_successors[i]);

            if (grp.has_satellite_predecessor())
                fill_satellite_predecessors(grp.satellite_predecessors, group_successors);
        }
    }

    void fill_satellite_predecessors(server_node_list const & satellite_predecessors,
                                     successor_container const & successors)
    {
        const size_t activation_limit = 0;

        for (server_node_list::const_iterator it = satellite_predecessors.begin();
                it != satellite_predecessors.end(); ++it)
        {
            server_node & node = const_cast<server_node &>(*it);

            if (node.is_synth()) {
                thread_queue_item * q_item = q->allocate_queue_item(queue_node(MOVE(queue_node_data(static_cast<abstract_synth *>(&node)))),
                                             successors, activation_limit);

                q->add_initially_runnable(q_item);
            }
            else {
                abstract_group & grp = static_cast<abstract_group &>(node);
                if (grp.has_synth_children())
                    fill_queue_recursive(grp, successors, activation_limit);
            }
        }
    }

    static successor_container concat_successors(successor_container const & lhs, successor_container const & rhs)
    {
        successor_container ret(lhs.size() + rhs.size());
        memcpy(ret.data->content,              lhs.data->content, lhs.size() * sizeof(thread_queue_item *));
        memcpy(ret.data->content + lhs.size(), rhs.data->content, rhs.size() * sizeof(thread_queue_item *));
        return ret;
    }

    /* returns the number of queue items of the tail of this abstract_group */
    static size_t count_tail_nodes(abstract_group const & grp)
    {
        size_t direct_tail_nodes = count_direct_tail_nodes(grp);
        if (direct_tail_nodes == 0)
            return count_satellite_predecessor_nodes(grp);
        else
            return direct_tail_nodes;
    }

    /* returns the number of queue items for the satellite predecessors */
    static size_t count_satellite_predecessor_nodes(server_node const & node)
    {
        size_t ret = 0;
        for (server_node_list::const_iterator it = node.satellite_predecessors.begin();
             it != node.satellite_predecessors.end(); ++it)
        {
            if (it->is_synth())
                ret += 1;
            else {
                abstract_group const & grp = static_cast<abstract_group const &>(*it);
                ret += count_tail_nodes(grp);
            }
        }

        return ret;
    }

    /* returns the number of tail nodes, ignoring satellite predecessors */
    /* @{ */
    static size_t count_direct_tail_nodes(abstract_group const & grp)
    {
        if (grp.is_parallel())
            return count_direct_tail_nodes(static_cast<parallel_group const &>(grp));
        else
            return count_direct_tail_nodes(static_cast<group const &>(grp));
    }

    static size_t count_direct_tail_nodes(group const & grp)
    {
        for (server_node_list::const_reverse_iterator it = grp.child_nodes.rbegin();
             it != grp.child_nodes.rend(); ++it)
        {
            if (it->is_synth())
                // ending with a synth
                return 1;

            abstract_group const & child_grp = static_cast<abstract_group const &>(*it);
            size_t child_grp_tail_nodes = count_tail_nodes(child_grp);
            if (child_grp_tail_nodes)
                return child_grp_tail_nodes;
        }
        return 0;
    }

    static size_t count_direct_tail_nodes(parallel_group const & grp)
    {
        size_t ret = 0;
        for (server_node_list::const_reverse_iterator it = grp.child_nodes.rbegin();
             it != grp.child_nodes.rend(); ++it)
        {
            if (it->is_synth())
               ret += 1;
            else {
                abstract_group const & child_grp = static_cast<abstract_group const &>(*it);
                ret += count_tail_nodes(child_grp);
            }
        }
        return ret;
    }
    /* @} */
};

} /* namespacen nova */

#endif /* SERVER_DEPENDENCY_GRAPH_GENERATOR_HPP */
