/*
 * Copyright (C) 2010-2018 Codership Oy <info@codership.com>
 */

/*! @file page file class */

#ifndef _gcache_page_hpp_
#define _gcache_page_hpp_

#include "gcache_memops.hpp"
#include "gcache_bh.hpp"

#include "gu_fdesc.hpp"
#include "gu_mmap.hpp"
#include "gu_logger.hpp"

#include <string>
#include <ostream>
#include <memory>

namespace gcache
{
    class Page : public MemOps
    {
    public:

        Page (void* ps, const std::string& name, size_t size, int dbg, bool encrypt, size_t encryptCachePageSize, size_t encryptCachePagesCount);
        ~Page () {}

        void* malloc  (size_type size);

        void* realloc (void* ptr, size_type size);

        void  free    (BufferHeader* bh)
        {
            assert(bh >= mmap_.get_ptr());
            assert(static_cast<void*>(bh) <=
                   (static_cast<uint8_t*>(mmap_.get_ptr()) + mmap_.get_size() -
                    sizeof(BufferHeader)));
            assert(bh->size > 0);
            assert(bh->store == BUFFER_IN_PAGE);
            assert(bh->ctx == reinterpret_cast<BH_ctx_t>(this));
            assert (used_ > 0);
            used_--;
#ifndef NDEBUG
            if (debug_) { log_info << name() << " freed " << bh; }
#endif
        }

        void  repossess(BufferHeader* bh)
        {
            assert(bh >= mmap_.get_ptr());
            assert(reinterpret_cast<uint8_t*>(bh) + bh->size <= next_);
            assert(bh->size > 0);
            assert(bh->seqno_g != SEQNO_NONE);
            assert(bh->store == BUFFER_IN_PAGE);
            assert(bh->ctx == reinterpret_cast<BH_ctx_t>(this));
            assert(BH_is_released(bh)); // will be marked unreleased by caller
            used_++;
#ifndef NDEBUG
            if (debug_) { log_info << name() << " repossessed " << bh; }
#endif
        }

        void discard (BufferHeader* bh)
        {
#ifndef NDEBUG
            if (debug_) { log_info << name() << " discarded " << bh; }
#endif
        }

        size_t used () const { return used_; }

#ifdef PXC
        size_t size() const { return size_; } /* size on storage */
#else
        size_t size() const { return fd_.size(); } /* size on storage */
#endif /* PXC */

        const std::string& name() const { return fd_.name(); }

        void reset ();

        /* Drop filesystem cache on the file */
        void drop_fs_cache() const;

        void* parent() const { return ps_; }

#ifdef PXC
        size_t allocated_pool_size ();
#endif /* PXC */

        void print(std::ostream& os) const;

        void set_debug(int const dbg) { debug_ = dbg; }

    private:

        gu::FileDescriptor fd_;
        std::shared_ptr<gu::IMMap>  mmapptr_;  // keep mmap_ member as the reference
        gu::IMMap&         mmap_;
        void* const        ps_;
        uint8_t*           next_;
        size_t             space_;
        size_t             used_;
#ifdef PXC
        size_t             size_;
        size_t             min_space_;
#endif /* PXC */
        int                debug_;

        Page(const gcache::Page&);
        Page& operator=(const gcache::Page&);
    };

    static inline std::ostream&
    operator <<(std::ostream& os, const gcache::Page& p)
    {
        p.print(os);
        return os;
    }
}

#endif /* _gcache_page_hpp_ */
