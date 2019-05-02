//  $Id$
//
//  lock-free fifo queue from
//  Michael, M. M. and Scott, M. L.,
//  "simple, fast and practical non-blocking and blocking concurrent queue algorithms"
//
//  intrusive implementation for c++
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

#ifndef __LOCKFREE_FIFO_HPP
#define __LOCKFREE_FIFO_HPP

//#include "cas.hpp"
//#include "atomic_ptr.hpp"
#include "branch_hints.hpp"

#ifdef HAVE_BOOST
#   include <boost/type_traits.hpp>
#   include <boost/static_assert.hpp>
#else /* HAVE_BOOST */
#   ifdef BOOST_STATIC_ASSERT
#      undef BOOST_STATIC_ASSERT
#   endif
#   define BOOST_STATIC_ASSERT(x)
#endif /* HAVE_BOOST */

#include <memory>
#include <atomic>

namespace lockfree
{
    struct intrusive_fifo_node;

    typedef std::atomic<intrusive_fifo_node *> intrusive_fifo_ptr_t;

    struct intrusive_fifo_node
    {
        intrusive_fifo_ptr_t next;
        struct fifo_node * data;
    };

    struct fifo_node
    {
        intrusive_fifo_node *volatile node;

    protected:
        fifo_node(void)
        {
            node = new intrusive_fifo_node();
        }

        ~fifo_node(void)
        {
            delete node;
        }

        template <class T> friend class intrusive_fifo;
    };

    template <typename T>
    class intrusive_fifo
    {
        BOOST_STATIC_ASSERT((boost::is_base_of<fifo_node,T>::value));

    public:
        intrusive_fifo(void)
        {
            /* dummy pointer for head/tail */
            intrusive_fifo_node *dummy = new intrusive_fifo_node();
            dummy->next.store(nullptr);
            head_.store(dummy);
            tail_.store(dummy);
        }

        ~intrusive_fifo(void)
        {
            /* client must have freed all members already */
            assert (empty());
            delete head_.load();
        }

        bool empty() const
        {
            return head_.load() == tail_.load() || (!tail_.load());
        }

        void enqueue(T * instance)
        {
            intrusive_fifo_node * node = static_cast<fifo_node*>(instance)->node;
            node->next.store(nullptr);
            node->data = static_cast<fifo_node*>(instance);

            for (;;)
            {
                intrusive_fifo_ptr_t tail(tail_.load());
                std::atomic_thread_fence(std::memory_order_seq_cst);

                intrusive_fifo_ptr_t next(tail.load()->next.load());
                std::atomic_thread_fence(std::memory_order_seq_cst);

                if (likely(tail == tail_))
                {
                    auto *n = next.load();
                    auto *t = tail.load();
                    
                    if (n == nullptr)
                    {
                        if (t->next.compare_exchange_weak(n,node))
                        {
                            tail_.compare_exchange_weak(t,node);
                            return;
                        }
                    }
                    else
                        tail_.compare_exchange_weak(t,n);
                }
            }
        }

        T* dequeue (void)
        {
            T * ret;
            for (;;)
            {
                intrusive_fifo_ptr_t head(head_.load());
                std::atomic_thread_fence(std::memory_order_seq_cst);

                intrusive_fifo_ptr_t tail(tail_.load());
                /* volatile */ intrusive_fifo_node * next = head.load()->next.load();
                std::atomic_thread_fence(std::memory_order_seq_cst);

                if (likely(head == head_))
                {
                    auto *h = head.load();
                    auto *t = tail.load();
                    if (h == t)
                    {
                        if (next == nullptr)
                            return 0;
                        tail_.compare_exchange_weak(t,next);
                    }
                    else
                    {
                        ret = static_cast<T*>(next->data);
                        if (head_.compare_exchange_weak(h,next))
                        {
                            ret->node = h;
                            return ret;
                        }
                    }
                }
            }
        }

    private:
        intrusive_fifo_ptr_t head_,tail_;
    };

    template <typename T>
    class fifo_value_node:
        public fifo_node
    {
    public:
        fifo_value_node(T const & v): value(v) {}

        T value;
    };

    template <typename T, class Alloc = std::allocator<T> >
    class fifo
        : intrusive_fifo<fifo_value_node<T> >
    {
    public:
        ~fifo()
        {
            fifo_value_node<T> * node;
            while((node = intrusive_fifo<fifo_value_node<T> >::dequeue()) != NULL)
                free(node);
        }

        void enqueue(T const & v)
        {
            intrusive_fifo<fifo_value_node<T> >::enqueue(alloc(v));
        }

        bool dequeue (T & v)
        {
            fifo_value_node<T> * node = intrusive_fifo<fifo_value_node<T> >::dequeue();
            if(!node)
                return false;

            v = node->value;
            free(node);
            return true;
        }

    private:

#if 0
        inline fifo_value_node<T> *alloc(const T &k)
        {
            fifo_value_node<T> *node = allocator.allocate(1);
            allocator.construct(node,k);
            return node;
        }

        inline void free(fifo_value_node<T> *n)
        {
            assert(n);
            allocator.destroy(n);
            allocator.deallocate(n,1);
        }
#else
        // hmmm... static keyword brings 10% speedup...

        static inline fifo_value_node<T> *alloc(const T &k)
        {
            return new fifo_value_node<T>(k);
        }

        static inline void free(fifo_value_node<T> *n)
        {
            assert(n);
            delete n;
        }
#endif

        typename Alloc::template rebind<fifo_value_node<T> >::other allocator;
    };
}

#endif /* __LOCKFREE_FIFO_HPP */



