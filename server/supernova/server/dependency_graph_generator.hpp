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
                                        rt_pool_allocator<void*> > thread_queue_item;

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
    static inline size_t get_previous_activation_count(reverse_iterator it, reverse_iterator end, int previous_activation_limit)
    {
        reverse_iterator prev = it;

        for(;;)
        {
            ++prev;
            if (prev == end)
                return previous_activation_limit; // we are the first item, so we use the previous activiation limit

            server_node & node = *prev;
            if (node.is_synth())
                return 1;
            else {
                abstract_group & grp = static_cast<abstract_group&>(node);
                size_t tail_nodes = grp.tail_nodes();

                if (tail_nodes != 0) /* use tail nodes of previous group */
                    return tail_nodes;
                else                /* skip empty groups */
                    continue;
            }
        }
    }

    successor_container fill_queue_recursive(abstract_group & grp, successor_container const & successors,
                                             size_t activation_limit)
    {
        if (grp.is_parallel())
            return fill_queue_recursive(static_cast<parallel_group&>(grp), successors, activation_limit);
        else
            return fill_queue_recursive(static_cast<group&>(grp), successors, activation_limit);
    }

    successor_container fill_queue_recursive(group & g, successor_container const & successors_from_parent,
                                             size_t previous_activation_limit)
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
                sequential_group_handle_synth(it, sequential_children, g, previous_activation_limit, successors, children);
            else
                sequential_group_handle_group(it, node, g, children, previous_activation_limit, successors);
        }
        assert(children == 0);
        return successors;
    }

    template <typename iterator>
    iterator iterator_add(iterator it, std::ptrdiff_t count)
    {
        assert(count > 0);
        for (std::ptrdiff_t i = 0; i != count; ++i)
            ++it;
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
        else if (it_before->is_group())
            // we reached a child group
            activation_limit = get_previous_activation_count(it, g.child_nodes.rend(), previous_activation_limit);
        else
            // before this node, there is a synth
            activation_limit = 1;

        if (first_node->has_satellite_predecessor())
            activation_limit += count_satellite_predecessor_nodes(*first_node);

        size_t node_count = sequential_children.size();
        sequential_child_list::reverse_iterator seq_it = sequential_children.rbegin();

        thread_queue_item * q_item;

        if (last_node->has_satellite_successor())
        {
            successor_container satellites = fill_satellite_successors(last_node->satellite_successors, 1);
            successor_container combined_successors = concat_successors(satellites, successors);

            q_item = q->allocate_queue_item(queue_node(static_cast<abstract_synth*>(*seq_it++), node_count),
                                            combined_successors, activation_limit);
        } else
            q_item = q->allocate_queue_item(queue_node(static_cast<abstract_synth*>(*seq_it++), node_count),
                                   successors, activation_limit);

        queue_node & q_node = q_item->get_job();

        // now we can add all nodes sequentially
        for(;seq_it != sequential_children.rend(); ++seq_it)
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
    server_node_list::reverse_iterator
    find_synth_sequence_start(server_node_list::reverse_iterator it,
                              server_node_list & child_nodes,
                              sequential_child_list & sequential_children)
    {
        for(;;)
        {
            sequential_children.push_back(&*it);
            ++it;
            if (it == child_nodes.rend())
                break; // we found the beginning of this group

            if (!it->is_synth())
                break; // we hit a child group, later we may want to add it's nodes, too?

            if (it->has_satellite_successor())
                break; // synths with satellite successors should be part of the previous queue item

            if (it->has_satellite_predecessor()) {
                // synths with satellite predecessors are the head of this queue item
                sequential_children.push_back(&*it);
                return it;
            }
        }
        --it;
        return it;
    }

    void sequential_group_handle_group(server_node_list::reverse_iterator const & it,
                                       server_node & node, group & g,
                                       size_t & children, size_t previous_activation_limit,
                                       successor_container & successors)
    {
        abstract_group & grp = static_cast<abstract_group&>(node);

        if (grp.has_synth_children())
        {
            int activation_limit = get_previous_activation_count(it, g.child_nodes.rend(), previous_activation_limit)
                                   + count_satellite_predecessor_nodes(node);

            if (node.has_satellite_successor())
            {
                successor_container satellites = fill_satellite_successors(node.satellite_successors, grp.tail_nodes());
                successor_container group_successors = concat_successors(satellites, successors);

                successors = fill_queue_recursive(grp, group_successors, activation_limit);
            } else
                successors = fill_queue_recursive(grp, successors, activation_limit);

            if (node.has_satellite_predecessor())
                fill_satellite_predecessors(node.satellite_predecessors, successors);
        } else
        {
            // TODO: parse empty groups for successors
        }

        children -= 1;
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

            q_item = q->allocate_queue_item(queue_node(static_cast<abstract_synth *>(&node)),
                                            concat_successors(successors, satellites),
                                            activation_limit);
        } else {
            q_item = q->allocate_queue_item(queue_node(static_cast<abstract_synth *>(&node)),
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
        if (!grp.has_synth_children()) //TODO: parse empty groups for successors
            return;

        if (node.has_satellite_successor()) {
            successor_container satellites = fill_satellite_successors(node.satellite_successors,
                                                                       grp.tail_nodes());

            successor_container group_successors = fill_queue_recursive(grp,
                                                                        concat_successors(successors, satellites),
                                                                        activation_limit);

            for (size_t i = 0; i != group_successors.size(); ++i)
                collected_nodes.push_back(group_successors[i]);

            if (node.has_satellite_predecessor())
                fill_satellite_predecessors(node.satellite_predecessors, group_successors);

        } else {
            successor_container group_successors = fill_queue_recursive(grp, successors, activation_limit);

            for (size_t i = 0; i != group_successors.size(); ++i)
                collected_nodes.push_back(group_successors[i]);

            if (node.has_satellite_predecessor())
                fill_satellite_predecessors(node.satellite_predecessors, group_successors);
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
                thread_queue_item * q_item = q->allocate_queue_item(queue_node(static_cast<abstract_synth *>(&node)),
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

    size_t count_satellite_predecessor_nodes(server_node const & node)
    {
        size_t ret = 0;
        for (server_node_list::const_iterator it = node.satellite_predecessors.begin();
            it != node.satellite_predecessors.end(); ++it)
        {
            if (it->is_synth())
                ret += 1;
            else {
                abstract_group const & grp = static_cast<abstract_group const &>(*it);
                ret += grp.tail_nodes();
            }
        }

        return ret;
    }

    successor_container concat_successors(successor_container const & lhs, successor_container const & rhs)
    {
        successor_container ret(lhs.size() + rhs.size());
        memcpy(ret.data->content,              lhs.data->content, lhs.size() * sizeof(thread_queue_item *));
        memcpy(ret.data->content + lhs.size(), rhs.data->content, rhs.size() * sizeof(thread_queue_item *));
        return ret;
    }
};

} /* namespacen nova */

#endif /* SERVER_DEPENDENCY_GRAPH_GENERATOR_HPP */
