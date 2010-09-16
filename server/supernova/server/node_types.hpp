//  node types
//  Copyright (C) 2008, 2009, 2010 Tim Blechmann
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

#ifndef SERVER_NODE_TYPES_HPP
#define SERVER_NODE_TYPES_HPP

#include <boost/detail/atomic_count.hpp>
#include <boost/bind.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive_ptr.hpp>

#include "memory_pool.hpp"
#include "synth_prototype.hpp"
#include "utilities/static_pool.hpp"

namespace nova
{

class server_node;

class abstract_group;
class group;

namespace bi = boost::intrusive;

typedef boost::intrusive::list<class server_node,
                                boost::intrusive::constant_time_size<false> >
server_node_list;

class server_node:
    public bi::list_base_hook<bi::link_mode<bi::auto_unlink> >,          /* group member */
    public bi::unordered_set_base_hook<bi::link_mode<bi::auto_unlink> >  /* for node_id mapping */
{
protected:
    server_node(int32_t node_id, bool type):
        node_id(node_id), synth_(type), running_(true), parent_(0), use_count_(0), satellite_reference_node(NULL),
        satellite_count(0)
    {}

    virtual ~server_node(void)
    {
        assert(parent_ == 0);
    }

    /* @{ */
    /** node id handling */
    void reset_id(int32_t new_id)
    {
        node_id = new_id;
    }

    int32_t node_id;

public:
    int32_t id(void) const
    {
        return node_id;
    }
    /* @} */

    typedef bi::list_base_hook<bi::link_mode<bi::auto_unlink> > parent_hook;

    /* @{ */
    /** node_id mapping */
    friend std::size_t hash_value(server_node const & that)
    {
        return hash(that.id());
    }

    static int32_t hash(int32_t id)
    {
        return id * 2654435761U; // knuth hash, 32bit should be enough
    }

    friend bool operator< (server_node const & lhs, server_node const & rhs)
    {
        return lhs.node_id < rhs.node_id;
    }

    friend bool operator== (server_node const & lhs, server_node const & rhs)
    {
        return lhs.node_id == rhs.node_id;
    }
    /* @} */

    bool is_synth(void) const
    {
        return synth_;
    }

    bool is_group(void) const
    {
        return !synth_;
    }

    /** set a slot */
    /* @{ */
    virtual void set(slot_index_t slot_id, float val) = 0;
    virtual void set(slot_index_t slot_str, size_t n, float * values) = 0;
    virtual void set(const char * slot_str, float val) = 0;
    virtual void set(const char * slot_str, size_t n, float * values) = 0;
    virtual void set(const char * slot_str, size_t hashed_str, float val) = 0;
    virtual void set(const char * slot_str, size_t hashed_str, size_t n, float * values) = 0;
    /* @} */


    /* @{ */
    /** group traversing */
    inline const server_node * previous_node(void) const;
    inline server_node * previous_node(void);
    inline const server_node * next_node(void) const;
    inline server_node * next_node(void);
    /* @} */

private:
    bool synth_;

    /** support for pausing node */
    /* @{ */
    bool running_;

public:
    virtual void pause(void)
    {
        running_ = false;
    }

    virtual void resume(void)
    {
        running_ = true;
    }

    bool is_running(void) const
    {
        return running_;
    }
    /* @} */

private:
    friend class node_graph;
    friend class abstract_group;
    friend class group;
    friend class parallel_group;

public:
    /* @{ */
    /** parent group handling */
    const abstract_group * get_parent(void) const
    {
        return parent_;
    }

    abstract_group * get_parent(void)
    {
        return parent_;
    }

    inline void set_parent(abstract_group * parent);

    void init_parent(abstract_group * parent)
    {
        add_ref();
        assert(parent_ == 0);
        parent_ = parent;
    }

    void clear_parent(void)
    {
        if (is_satellite())
            satellite_deinit();
        else
            node_deinit();

        parent_ = NULL;
        release();
    }
    /* @} */

private:
    abstract_group * parent_;

    /* clean up parent group for regular nodes */
    inline void node_deinit(void);

public:
    /* memory management for server_nodes */
    /* @{ */
    inline void * operator new(std::size_t size)
    {
        return rt_pool.malloc(size);
    }

    inline void operator delete(void * p)
    {
        rt_pool.free(p);
    }
    /* @} */


    /** satellite management */
public:
    /* @{ */
    void add_sat_before(server_node * node)
    {
        satellite_predecessors.push_front(*node);
        node->satellite_init(this, parent_);
    }

    void add_sat_after(server_node * node)
    {
        satellite_successors.push_front(*node);
        node->satellite_init(this, parent_);
    }

    void remove_satellite(server_node * node)
    {
        node->clear_parent();
    }

    bool is_satellite(void) const
    {
        return satellite_reference_node != NULL;
    }

    bool has_satellite(void) const
    {
        return satellite_count != 0;
    }

    bool has_satellite_predecessor(void) const
    {
        return !satellite_predecessors.empty();
    }

    bool has_satellite_successor(void) const
    {
        return !satellite_successors.empty();
    }

protected:
    void set_on_satellites(slot_index_t slot_id, float val)
    {
        apply_on_satellites(boost::bind(static_cast<void (server_node::*)(slot_index_t, float)>(&server_node::set),
                                        _1, slot_id, val));
    }

    void set_on_satellites(slot_index_t slot_id, size_t n, float * values)
    {
        apply_on_satellites(boost::bind(static_cast<void (server_node::*)(slot_index_t, size_t, float*)>(&server_node::set),
                                        _1, slot_id, n, values));
    }

    void set_on_satellites(const char * slot_str, size_t hashed_str, float val)
    {
        apply_on_satellites(boost::bind(static_cast<void (server_node::*)(const char*, size_t, float)>(&server_node::set),
                                        _1, slot_str, hashed_str, val));
    }

    void set_on_satellites(const char * slot_str, size_t hashed_str, size_t n, float * values)
    {
        apply_on_satellites(boost::bind(static_cast<void (server_node::*)(const char*, size_t, size_t, float*)>(&server_node::set),
                                        _1, slot_str, hashed_str, n, values));
    }

    void pause_satellites(void)
    {
        apply_on_satellites(boost::bind(static_cast<void (server_node::*)(void)>(&server_node::pause),
                                        _1));
    }

    void resume_satellites(void)
    {
        apply_on_satellites(boost::bind(static_cast<void (server_node::*)(void)>(&server_node::resume),
                                        _1));
    }

private:
    template <typename functor>
    inline void apply_on_satellites(functor const & f)
    {
        if (!has_satellite())
            return; // fast path

        assert(satellite_predecessors.size() + satellite_successors.size() == satellite_count);
        std::for_each(satellite_predecessors.begin(), satellite_predecessors.end(), f);
        std::for_each(satellite_successors.begin(), satellite_successors.end(), f);
    }

    void satellite_init(server_node * satellite_reference, abstract_group * parent)
    {
        init_parent(parent);

        satellite_reference_node = satellite_reference;
        ++(satellite_reference_node->satellite_count);
    }

    /* called from clear_parent() */
    void satellite_deinit(void)
    {
        --(satellite_reference_node->satellite_count);
        server_node::parent_hook::unlink();
        satellite_reference_node = NULL;
    }

    server_node * satellite_reference_node;
    uint32_t satellite_count;
    server_node_list satellite_predecessors;
    server_node_list satellite_successors;
    friend class dependency_graph_generator;
    /* @} */

public:
    /* refcountable */
    void add_ref(void)
    {
        ++use_count_;
    }

    void release(void)
    {
        if(unlikely(--use_count_ == 0))
            delete this;
    }

private:
    boost::detail::atomic_count use_count_;
    /* @} */
};

inline void intrusive_ptr_add_ref(server_node * p)
{
    p->add_ref();
}

inline void intrusive_ptr_release(server_node * p)
{
    p->release();
}

typedef boost::intrusive_ptr<server_node> server_node_ptr;
typedef boost::intrusive_ptr<class abstract_synth> synth_ptr;
typedef boost::intrusive_ptr<group> group_ptr;

enum node_position
{
    head = 0,
    tail = 1,
    before = 2,
    after = 3,
    replace = 4,
    insert = 5,                  /* for pgroups */
    satellite_before = 6,
    satellite_after = 7
};

typedef std::pair<server_node *, node_position> node_position_constraint;


} /* namespace nova */

#endif /* SERVER_NODE_TYPES_HPP */
