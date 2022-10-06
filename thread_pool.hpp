#ifndef __THREADPOOL_HPP__
#define __THREADPOOL_HPP__

#include <stdio.h>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/asio/io_service.hpp>

namespace concurrent {

  //############################################################################
  /// CLASS Thread Pool
  //############################################################################
  class thread_pool_t {
  public:

    typedef boost::thread_group            thread_group_t;
    typedef boost::asio::io_service        io_service_t;
    typedef boost::asio::io_service::work  work_t;
    typedef boost::shared_ptr<work_t>      work_ptr;

    //##########################################################################
    /// Singleton Accessor
    ///
    /// @param[in/out]  none
    /// @return         single instance
    /// @throws         none
    //##########################################################################
    static thread_pool_t& instance() {
      static thread_pool_t instance_;
      return instance_;
    }

    //##########################################################################
    /// Constructor
    ///
    /// @param[in/out]  none
    /// @return         none
    /// @throws         none
    //##########################################################################
    thread_pool_t() :
      size_(0)
    { work_ = boost::make_shared<work_t>(boost::ref(iosvc_)); }

    //##########################################################################
    /// Expand
    ///
    /// Can be used to expand the thread pool based on the input size.
    ///
    /// @param[in]  sz  number of threads to be added
    /// @return         none
    /// @throws         std::string on failure
    //##########################################################################
    void expand(size_t size) {
      size_ += size;
      create_threads(size);
    }

    //##########################################################################
    /// Wait
    ///
    /// @param   none
    /// @return  none
    /// @throws  none
    //##########################################################################
    void wait() {
      threads_.join_all();
    }

    //##########################################################################
    /// Stop
    ///
    /// -# Reset work ptr.
    /// -# Stop iosvc
    /// -# Interrupt all threads.
    /// -# Join on thread pool?
    ///
    /// @param   none
    /// @return  none
    /// @throws  none
    //##########################################################################
    void stop() {
      work_.reset();
      iosvc_.stop();
      threads_.interrupt_all();
    }

    //##########################################################################
    /// Post
    ///
    /// @param[in]  t  worker function to run on a thread
    /// @return        none
    /// @throws        none
    //##########################################################################
    template <typename T>
    void post(T t) {
      iosvc_.post(t);
    }

    //##########################################################################
    /// Iosvc Accessor
    ///
    /// @param   none
    /// @return  ref to iosvc_
    /// @throws  none
    //##########################################################################
    boost::asio::io_service& iosvc() {
      return iosvc_;
    }

    //##########################################################################
    /// Thread Pool Size
    ///
    /// @param   none
    /// @return  number of threads in the pool
    /// @throws  none
    //##########################################################################
    size_t size() const {
      return threads_.size();
    }

    typedef boost::shared_ptr<thread_pool_t> ptr;

  private:

    //##########################################################################
    /// Create Threads
    ///
    /// Creates input size number of threads into the boost thread group.
    ///
    /// @param[in]  size  number of threads to be created
    /// @return           none
    /// @throws           std::string on failure
    //##########################################################################
    void create_threads(size_t size) {

      for (int i = 0; i < size; ++i) {

        boost::thread* t =
          threads_.create_thread(
            boost::bind(&boost::asio::io_service::run,
                        boost::ref(iosvc_)));
        if (! t) {
          char buf[256] = {0};
          ::snprintf(buf, sizeof(buf-1), "Thread pool create failed at: %d", i);
          throw std::string(buf);
        }
      }
    }
  
    size_t          size_;
    work_ptr        work_;
    io_service_t    iosvc_;
    thread_group_t  threads_;

  };  /// class thread_pool_t

}  /// namespace concurrent

#endif
