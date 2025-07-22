/*!
 * @file 		uvector.h
 * @author 		Zdenek Travnicek
 * @date		16.2.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */


#ifndef UVECTOR_H_
#define UVECTOR_H_
#include <cstddef>
#include <algorithm>
#include <stdexcept>
#include "make_unique.h"

namespace yuri {

struct yuri_deleter {
	virtual void 				operator()(void*) noexcept {}
	virtual 					~yuri_deleter() noexcept {};
};

/*!
 * @tparam Deleter type of deleting functor.
 * 			It has to have operator() that DOESN'T throw and has noexcept specification
 * 			Deleter has to be copy-assignable or move assignable
 *
 */
template<typename Deleter>
struct impl_yuri_deleter:public yuri_deleter {
								impl_yuri_deleter(const Deleter& d):d(d) {
									static_assert(noexcept(d(nullptr)),"Deleter has to have noexcept operator()!");
								}
								impl_yuri_deleter(Deleter&& d) noexcept:d(std::move(d)) {
									static_assert(noexcept(d(nullptr)),"Deleter has to have noexcept operator()!");
								}
	void 						operator() (void* mem) noexcept { d(mem); }
private:
	Deleter 					d;
};



/*!
 * Simple std::vector like class providing uninitialized storage.
 * It's @em NOT suitable for storing classes etc as it doesn't call constructors.
 */
template<typename T, bool Realloc = true>
class uvector {
public:
	typedef T 					value_type;
	typedef std::size_t 		size_type;
	typedef std::ptrdiff_t 		difference_type;
	typedef T& 					reference;
	typedef const T& 			const_reference;
	typedef T* 					pointer;
	typedef const T* 			const_pointer;
	typedef T* 					iterator;
	typedef const T* 			const_iterator;
	typedef std::unique_ptr<T[]>upointer;
	static const bool 			reallocating = Realloc;

	uvector():size_(0),allocated_(0) {}
	uvector(size_type count)
		//:data_(make_unique<T[]>(count)),size_(count),allocated_(count) {}
	:data_(make_unique_uninitialized<T[]>(count)),size_(count),allocated_(count) {}
	uvector(size_type count, const T& value)
		:data_(make_unique_uninitialized<T[]>(count)),size_(count),allocated_(count) {
		std::fill(data_.get(),data_.get()+size_,value);
	}
	~uvector() noexcept { impl_delete(data_);}

	uvector(const uvector<T,Realloc>& other_):size_(other_.size_),allocated_(other_.allocated_) {
		if (size_) {
			data_ = make_unique_uninitialized<T[]> (allocated_);
			std::copy(other_.begin(),other_.end(),begin());
		}
	}

	uvector<T,Realloc>& operator=(const uvector<T,Realloc>& rhs) {
		resize(rhs.size());
		std::copy(rhs.begin(),rhs.end(),begin());
		return *this;
	}

	uvector(uvector<T, Realloc>&& rhs) noexcept:data_(std::move(rhs.data_)),size_(rhs.size_),allocated_(rhs.allocated_),deleter_(std::move(rhs.deleter_))
	{}

	uvector<T, Realloc>& operator=(uvector<T, Realloc>&& rhs) noexcept {
		// By using swap, there's no need to deallocate data immediately
		swap(rhs);
		return *this;
	}

	template<class Iterator>
	uvector(Iterator first, Iterator last)
				:size_(0),allocated_(0) {
		resize(std::distance(first, last));
		std::copy(first,last,begin());
	}

	template<class Deleter> uvector(pointer data, size_type size, Deleter deleter)
			:size_(0),allocated_(0) { set(data, size, deleter); }
	/*!
	 * Method to user provided pointer as internal data
	 * @param data		pointer to T[]
	 * @param size		size of the data @ em data points to
	 * @param deleter	deleter that will be used to release the memory
	 * @return			reference to itself
	 */
	template<class Deleter>
	uvector<T,Realloc>& set(pointer data, size_type size, Deleter deleter) {
		impl_delete(data_);
		data_.reset(data);
		size_ = size;
		allocated_ = size;
		deleter_ = make_unique<impl_yuri_deleter<Deleter>>(deleter);
		return *this;
	}


	size_type 					size() const noexcept { return size_;}
	bool 						empty() const noexcept { return size_ == 0; }
	size_type 					capacity() const noexcept { return allocated_;}
	reference	 			operator[](size_type pos) noexcept  { return data_[pos]; }
	const_reference		 	operator[](size_type pos) const noexcept { return data_[pos]; }
	reference 				at(size_type pos) { if (pos>=size_) throw std::out_of_range(""); return data_[pos]; }
	const_reference 		at(size_type pos) const { if (pos>=size_) throw std::out_of_range(""); return data_[pos]; }

	reference 				front() noexcept { return data_[0]; }
	const_reference 		front() const noexcept { return data_[0]; }
	reference 				back() noexcept { return data_[size_-1]; }
	const_reference 		back() const noexcept { return data_[size_-1]; }

	iterator	 			begin() noexcept { /*assert(data_);*/ return &data_[0]; }
	const_iterator	 		begin() const noexcept { return &data_[0]; }
	iterator 				end() noexcept { return &data_[size_]; }
	const_iterator 			end() const noexcept { return &data_[size_]; }
	const_iterator	 		cbegin() const noexcept { return &data_[0]; }
	const_iterator 			cend() const noexcept { return &data_[size_]; }

	pointer					data() noexcept { return data_.get();}
	const_pointer			data() const noexcept { return data_.get();}

	void 					reserve(size_type size) {reserve_impl<Realloc>(size);}


	void 					resize(size_type size) {
		reserve(size); size_ = size;
	}
	void 					clear() noexcept { size_=0;}

	iterator 				insert( iterator pos, const T& value);
	void 					insert( iterator pos, size_type count, const T& value );
	template< class InputIt >
	void 					insert( iterator pos, InputIt first, InputIt last) {
		size_type count = std::distance(first,last);
		reserve(size_+count);
		if (pos<end()) std::copy_backward(pos,end(),end());
		std::copy(first,last,pos);
		size_ += count;
	}
	void 					assign( size_type count, const T& value ) {
		size_=count; std::fill(&data_[0],data_[size_],value);}

	template< class InputIt >
	void 					assign( InputIt first, InputIt last ) {
		size_=std::distance(first,last);
		std::copy(first,last,&data_[0]);
	}

	void 					swap( uvector<T,Realloc>& other ) noexcept {
		using std::swap;
		swap(allocated_,other.allocated_);
		swap(size_,other.size_);
		swap(data_,other.data_);
		swap(deleter_, other.deleter_);
	}
							operator uvector<T,!Realloc>&() {
		return *(reinterpret_cast<uvector<T,!Realloc>* >(this));
	}
private:
	upointer				data_;
	size_type 				size_;
	size_type 				allocated_;
	std::unique_ptr<yuri_deleter>
							deleter_;
	template<bool R>
	typename std::enable_if<R, void>::type
							reserve_impl(size_type size) {
		if (allocated_<size) {
			std::unique_ptr<T[]> tmp = make_unique_uninitialized<T[]>(size);
			if (data_) std::copy(data_.get(),data_.get()+size_,tmp.get());
			std::swap(data_,tmp);
			allocated_=size;
			impl_delete(tmp);
		}
	}
	template<bool R>
	typename std::enable_if<!R, void>::type
							reserve_impl(size_type size) {
		if (allocated_<size) {
			impl_delete(data_);
			data_ = std::move(make_unique_uninitialized<T[]>(size));
			allocated_=size;
		}
	}
	void 					impl_delete(upointer& mem) {
		if (deleter_) {
			(*deleter_)(mem.release());
			deleter_.reset();
		} else mem.reset();
	}
};

	template<typename T, bool ReallocL, bool ReallocR>
	void swap(yuri::uvector<T,ReallocL>& lhs, yuri::uvector<T,ReallocR>& rhs) {
		lhs.swap(rhs);
	}
}
#endif /* UVECTOR_H_ */
