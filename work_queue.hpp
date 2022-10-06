#ifndef _WORK_QUEUE_HPP__
#define _WORK_QUEUE_HPP__

#include <queue>
#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/locks.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace concurrent {

  //############################################################################
  /// CLASS:  Queue
  //############################################################################
  template <typename T>
  class queue_t {
  public:

    //##########################################################################
    /// Constructor
    ///
    /// Constructs shared pointers to mutex and condition variable.
    ///
    /// @param[in/out]  none
    /// @return         none
    /// @throws         none
    //##########################################################################
    queue_t();

    //##########################################################################
    /// Push
    ///
    /// Pushes item onto queue in a thread safe manner.
    ///
    /// @param[in]  t  item pushed  onto the back of the queue
    /// @return        none
    /// @throws        can throw exceptions from boost::thread api
    //##########################################################################
    void push(const T& t);

    //##########################################################################
    /// Pop Front
    ///
    /// Pops item from front of queue in a thread safe manner.
    ///
    /// @param[inout]  none
    /// @param[in]     none
    /// @return        item T
    /// @throws        std::string on failure
    //##########################################################################
    T pop_front();

    //##########################################################################
    /// Pop Front
    ///
    /// Pops item from front of queue in a thread safe manner.
    ///
    /// @param[in]  t  item removed from the front of the queue
    /// @return        none
    /// @throws        std::string on failure
    //##########################################################################
    void pop_front(T& t);

  private:

    boost::shared_ptr<boost::mutex>              mutex_;
    boost::shared_ptr<boost::condition_variable> cond_;
    std::queue<T> queue_;
  };

  //############################################################################
  /// Constructor
  //############################################################################
  template <typename T>
  inline queue_t<T>::queue_t() :
    mutex_(boost::make_shared<boost::mutex>()),
    cond_(boost::make_shared<boost::condition_variable>()) {
  }

  //############################################################################
  /// Push
  //############################################################################
  template <typename T>
  inline void queue_t<T>::push(const T& t) {

    try {

      boost::lock_guard<boost::mutex> lock(*mutex_);
      queue_.push(t);
      cond_->notify_all();
    }
    catch (const std::exception& ex) {
      std::cerr << "queue_t<T>::push caught: " << ex.what() << std::endl;
      throw;
    }
    catch (...) {
      std::cerr << "queue_t<T>::push caught unknown ex" << std::endl;
      throw;
    }
  }

  //############################################################################
  /// Pop Front
  //############################################################################
  template <typename T>
  inline void queue_t<T>::pop_front(T& t) {

    try {

      boost::unique_lock<boost::mutex> lock(*mutex_);

      while (queue_.empty())
        cond_->wait(lock);
    
      t = queue_.front();
      queue_.pop();
    }
    catch (const std::exception& ex) {
      std::cerr << "queue_t<T>::pop_front caught: " << ex.what() << std::endl;
      throw;
    }
    catch (...) {
      std::cerr << "queue_t<T>::pop_front caught unknown ex" << std::endl;
      throw;
    }
  }
  
  //############################################################################
  /// Pop Front
  //############################################################################
  template <typename T>
  inline T queue_t<T>::pop_front() {
    T item;
    pop_front(item);
    return item;
  }

}  /// namespace concurrent

#endif  /// _WORK_QUEUE_HPP__
