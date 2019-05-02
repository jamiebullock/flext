//  $Id$
//
//  lock-free stack
//
//  Copyright (C) 2007 Tim Blechmann & Thomas Grill
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

//  $Revision$
//  $LastChangedRevision$
//  $LastChangedDate$
//  $LastChangedBy$

#ifndef __LOCKFREE_STACK_HPP
#define __LOCKFREE_STACK_HPP

#include "branch_hints.hpp"

#include <memory>  // for std::allocator
#include <atomic>

#if HAVE_BOOST
#   include <boost/type_traits.hpp>
#   include <boost/static_assert.hpp>
#else
#   define BOOST_STATIC_ASSERT(x)
#endif

namespace lockfree
{
    //! nodes for the intrusive_stack must be derived from that
    class stack_node 
    {
        template <class T> friend class intrusive_stack;

    public:
        stack_node(): next(NULL) {}
    
    //private:
        std::atomic<stack_node *> next;
    };

    //! intrusive lock-free stack implementation with T being the node type (inherited from stack_node)
    template <typename T>
    class intrusive_stack 
    {
        BOOST_STATIC_ASSERT((boost::is_base_of<stack_node,T>::value));

    public:
        intrusive_stack(): head(NULL) {}

        ~intrusive_stack()
        {
            assert(empty());
        }

        bool empty() const { return !head.load(); }

        void push(T *node) 
        {
            assert(!node->next.load());
            for(;;) {
                stack_node *current = head.load();
                node->next.store(current);
                if(unlikely(!head.compare_exchange_weak(current, node))) {
                    break;
                }
            }
        }

        T *pop() 
        {
            for(;;) {
                stack_node *node = head.load();
                stack_node *next = node->next.load();
                if(!node || likely(head.compare_exchange_weak(node, next))) {
                    if(node) node->next.store(NULL);
                    return static_cast<T *>(node);
                }
            }
        }
  
    private:
        std::atomic<stack_node *> head;
    };


    //! node type used by non-intrusive stack
    template <typename T>
    class stack_value_node
        : public stack_node
    {
    public:
        stack_value_node(T const &v): value(v) {}   
        T value;
    };


    //! non-intrusive lock-free stack
    template <typename T,class Alloc = std::allocator<T> >
    class stack
        : intrusive_stack<stack_value_node<T> >
    {
    public:
        ~stack()
        {
            // delete remaining elements
            stack_value_node<T> * node;
            while((node = intrusive_stack<stack_value_node<T> >::pop()) != NULL)
                free(node);
        }

        void push(T const &v) 
        {
            intrusive_stack<stack_value_node<T> >::push(alloc(v));
        }

        bool pop(T &v) 
        {
            stack_value_node<T> *node = intrusive_stack<stack_value_node<T> >::pop();
            if(!node)
                return false;
            v = node->value;
            free(node);
            return true;
        }
        
    private:

        inline stack_value_node<T> *alloc(const T &k)
        {
            stack_value_node<T> *node = allocator.allocate(1);
            allocator.construct(node,k);
            return node;
        }

        inline void free(stack_value_node<T> *n)
        {
            assert(n);
            allocator.destroy(n);
            allocator.deallocate(n,1);
        }

        typename Alloc::template rebind<stack_value_node<T> >::other allocator;
    };

} // namespace

#endif
