/*!
 * @file 		make_unique.h
 * @author 		Zdenek Travnicek
 * @date		6.9.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef MAKE_UNIQUE_H_
#define MAKE_UNIQUE_H_

/*
 * std::make_unique, by. S. T. Lavalej, from N3656
 * (with modified identifiers)
 */

#include <memory>
#include <type_traits>
#include <utility>
namespace yuri {
    template<class T> struct unique_if {
        typedef std::unique_ptr<T> single_object;
    };
    template<class T> struct unique_if<T[]> {
        typedef std::unique_ptr<T[]> unknown_bound;
    };

#if __cplusplus >= 201402L
    using std::make_unique;
#else


  template<class T, size_t N> struct unique_if<T[N]> {
      typedef void known_bound;
  };

  template<class T, class... Args>
      typename unique_if<T>::single_object
      make_unique(Args&&... args) {
          return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
      }

  template<class T>
      typename unique_if<T>::unknown_bound
      make_unique(size_t n) {
          typedef typename std::remove_extent<T>::type U;
          return std::unique_ptr<T>(new U[n]());
      }

  template<class T, class... Args>
      typename unique_if<T>::known_bound
      make_unique(Args&&...) = delete;
#endif
  template<class T>
	typename unique_if<T>::unknown_bound
	make_unique_uninitialized(size_t n) {
		typedef typename std::remove_extent<T>::type U;
		return std::unique_ptr<T>(new U[n]);
	}

}
#endif /* MAKE_UNIQUE_H_ */
