//  $Id$
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

#ifndef __LOCKFREE_ATOMIC_PTR_HPP
#define __LOCKFREE_ATOMIC_PTR_HPP

#include "branch_hints.hpp"

#include <cstddef>
#include <atomic>

namespace lockfree
{
    using std::size_t;

    template <class T>
    class atomic_ptr
    {
    public:
        atomic_ptr() {}

       // atomic_ptr(T *p) : ptr(p), tag(0) {}
        atomic_ptr(const atomic_ptr &p): ptr(p.ptr),tag(p.tag) {}

        atomic_ptr(T *p,size_t t = 0): ptr(p),tag(t) {}

        /** atomic set operation */
        inline atomic_ptr &operator =(const atomic_ptr &p)
        {
            for (;;)
            {
//                atomic_ptr current(ptr, tag);

//                if(likely(ptr.compare_exchange_weak(current.ptr, p.ptr)))
                    return *this;
            }
        }

        inline atomic_ptr &operator()(T *p,size_t t)
        {
            return operator=(atomic_ptr(p, t) );
        }


        inline bool operator ==(const atomic_ptr &p) const { return ptr == p.ptr && tag == p.tag; }

        inline bool operator !=(const atomic_ptr &p) const { return !operator ==(p); }


        inline T * getPtr() const { return ptr; }

        inline void setPtr(T * p) { ptr = p; }


//        inline size_t getTag() const { return tag; }
//
//        inline void setTag(size_t t) { tag = t; }
//
//        inline size_t incTag() { return ++tag; }


        inline bool CAS(const atomic_ptr &oldval,const atomic_ptr &newval)
        {
            return lockfree::CAS2(this,oldval.ptr,oldval.tag,newval.ptr,newval.tag);
        }

        inline bool CAS(const atomic_ptr &oldval,T *newptr)
        {
            return lockfree::CAS2(this,oldval.ptr,oldval.tag,newptr,oldval.tag+1);
        }

        inline bool CAS(const T *oldptr,size_t oldtag,T *newptr)
        {
            return lockfree::CAS2(this,oldptr,oldtag,newptr,oldtag+1);
        }

    protected:
        
//        std::atomic<T *> ptr;
        T * volatile ptr;
        size_t volatile tag;
    };

} // namespace

#endif /* __LOCKFREE_ATOMIC_PTR_HPP */
