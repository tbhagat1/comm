#ifndef __TRACER_HPP__
#define __TRACER_HPP__

#include <iostream>
#include <fstream>
#include <boost/thread.hpp>
#include <thread_pool.hpp>

namespace trading {

  //############################################################################
  /// CLASS: Tracer
  ///
  /// Rudimentary thread-safe tracer. Only supports enable/disable of tracer.
  /// Must be used with TRACE_BEGIN, TRACE and TRACE_END macros to ensure
  /// traces are not distorted.
  //############################################################################
  class tracer_t {
  public:

    //##########################################################################
    /// Singleton Accessor
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        singleton instance
    /// @throws        none
    //##########################################################################
    static tracer_t& instance() {
      static tracer_t tracer_;
      return tracer_;
    }

    //##########################################################################
    /// Operator() (std::ostream)
    ///
    /// At this time everything goes to std::cout.
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        std::cout reference
    /// @throws        none
    //##########################################################################
    operator std::ostream&() {
      return std::cout;
    }

    //##########################################################################
    /// Operator<<
    ///
    /// Prints a basic prefix of thread id and then the input parameter. Should
    /// be extended to print timestamp, pid, machine, trace level etc.
    ///
    /// @param[in]     t  object to be traced
    /// @param[inout]     none
    /// @return           reference to this
    /// @throws           none
    //##########################################################################
    template <typename T>
    std::ostream& operator<<(const T& t) {
      std::cout << pthread_self() << ": " << t;
      return *this;
    }

    //##########################################################################
    /// Mutex Accessor
    ///
    /// Returns ref to mutex used to serialize tracing.
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        reference to mutex_
    /// @throws        none
    //##########################################################################
    boost::mutex& mutex() {
      return mutex_;
    }

    //##########################################################################
    /// Enabled Accessor
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        enabled_ member
    /// @throws        none
    //##########################################################################
    bool enabled() const {
      return enabled_;
    }

    //##########################################################################
    /// Disabled Accessor
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        negation of enabled accessor
    /// @throws        none
    //##########################################################################
    bool disabled() const {
      return !enabled();
    }

    //##########################################################################
    /// Enable Mutator
    ///
    /// Sets enabled_ to true.
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        none
    /// @throws        none
    //##########################################################################
    void enable() {
      enabled_ = true;
    }

    //##########################################################################
    /// Disable Mutator
    ///
    /// Sets enabled_ to false.
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        none
    /// @throws        none
    //##########################################################################
    void disable() {
      enabled_ = false;
    }

  private:

    //##########################################################################
    /// Constructor
    ///
    /// enabled_ is true by default.
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        none
    /// @throws        none
    //##########################################################################
    tracer_t() :
      enabled_(true) {
    }

    bool          enabled_;
    boost::mutex  mutex_;
  };
}

#define TRACE_BEGIN \
  if (trading::tracer_t::instance().enabled()) { \
    boost::lock_guard<boost::mutex> lock(trading::tracer_t::instance().mutex()); \
    trading::tracer_t::instance()

#define TRACE \
  trading::tracer_t::instance()

#define TRACE_END \
  }
  
#endif
