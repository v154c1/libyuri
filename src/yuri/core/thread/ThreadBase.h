/*!
 * @file 		ThreadBase.h
 * @author 		Zdenek Travnicek
 * @date 		24.7.2010
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2010 - 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef THREADBASE_H_
#define THREADBASE_H_
#include "yuri/core/forward.h"
#include "yuri/log/Log.h"

#include "yuri/core/thread/ThreadChild.h"
#include "yuri/core/parameter/Parameters.h"
//#include "yuri/core/thread/ThreadSpawn.h"
#include <map>
#include <vector>
#include <atomic>
#include "yuri/core/utils/time_types.h"
#ifdef __linux__
#include <sys/time.h>
#else
#include <ctime>
typedef int pid_t;
#endif

namespace yuri
{
namespace core
{

const int yuri_exit_finished = 0;
const int yuri_exit_interrupted = 1;

class ThreadBase :public std::enable_shared_from_this<ThreadBase>
{
public:
	EXPORT static Parameters	configure();
	EXPORT 						ThreadBase(const log::Log &_log, pwThreadBase parent, const std::string& = std::string());
	EXPORT virtual 				~ThreadBase() noexcept;
	EXPORT 						ThreadBase(const ThreadBase&) = delete;
	EXPORT 						ThreadBase(ThreadBase&&) = delete;
	EXPORT void					operator=(const ThreadBase&) = delete;
	EXPORT void					operator=(ThreadBase&&) = delete;

	/*!
	 * Executes main thread loop.
	 * Should not be called directly
	 */
	EXPORT void 				operator()();

	/*!
	 * Notification for the thread that one of it's childs is ending.
	 * Should not be called directly
	 *
	 * @param child				weak pointer to the ending child
	 * @param code				Return code (reason for ending).
	 */
	EXPORT void 				child_ends(pwThreadBase child, int code);
	/*!
	 * Signal for the thread to quit.
	 * User can call this method to request termination.
	 */
	EXPORT void 				finish() noexcept;
	/*!
	 * Method for the thread itself to request to be ended.
	 *
	 * @param code				Optional return code
	 */
	EXPORT void					request_end(int code = yuri_exit_finished);
//	virtual pid_t 				get_tid();
	/*!
	 * Returns true if the thread is currently running
	 * This basically means, that the 'operator()' method is executing
	 * This should be used for external entities to check whether this thread is still active.
	 *
	 * @return true while the thread is running
	 */
	EXPORT bool					running() const noexcept { return running_;}
private:
	/*!
	 * Implementation of the main loop.
	 * Has to be implemented in child classes.
	 */
	virtual void 				run() = 0;
protected:
	/*!
	 * Returns true while the thread should continue running.
	 * The method also processes events and should be called regularly
	 *
	 * @return false if the thread should quit, true otherwise.
	 */
	EXPORT bool 				still_running();

	/*!
	 * Returns shared_ptr for this instance
	 * @return pointer to @em this.
	 */
	EXPORT pThreadBase			get_this_ptr();

	/*!
	 * Returns shared_ptr for this instance (const)
	 * @return pointer to @em this.
	 */
	EXPORT pcThreadBase			get_this_ptr() const;
	//! Returns false if the thread should quit, true otherwise.
	//! The method also processes events and should be called occasionally
	/*!
	 * Adds a new child thread and spawns it as new thread
	 *
	 * @param thread			Child to spawn as a new child
	 */
	EXPORT bool 				spawn_thread(pThreadBase  thread);

	/*!
	 * Adds a new child without spawning it.
	 *
	 * @param thread			Child to add as a new child
	 */
	EXPORT bool		 			add_child(pThreadBase  thread);


//	//! Return current thread id, if supported on current platform
//	virtual pid_t 				retrieve_tid();
	//! Prints own tid. Useful for debugging
	EXPORT virtual void 		print_id(log::debug_flags_t f=log::info);
	//! Sets CPU affinity to a single CPU core.
	EXPORT virtual bool 		bind_to_cpu(size_t cpu);

/* ****************************************************************************
 *   Methods for managing thread hierarchy. Should not be called directly
 * ****************************************************************************/
protected:
	EXPORT void		 			finish_thread(pwThreadBase child, bool join=false);
	EXPORT void 				join_thread(pwThreadBase child);
	EXPORT void 				finish_all_threads(bool join=false);
	EXPORT void 				join_all_threads() noexcept;
	EXPORT void 				request_finish_thread(pwThreadBase child);
	EXPORT virtual void 		do_request_finish_thread(pwThreadBase child);
	EXPORT void		 			process_pending_requests();
	EXPORT virtual yuri::size_t get_active_childs();

	EXPORT bool		 			set_params(const Parameters &parameters);
	EXPORT virtual bool 		set_param(const Parameter &parameter);
	template<typename T> bool 	set_param(const std::string& name, const T& value);
	EXPORT std::string			get_node_name() const;
private:
	bool 						do_spawn_thread(pThreadBase  thread);
	bool 						do_add_child(pThreadBase  thread, bool spawned=true);

	void 						do_finish_thread(pwThreadBase child, bool join=false) noexcept;
	void		 				do_join_thread(pwThreadBase child, bool check=true) noexcept;

//	virtual	bool				do_child_ended(size_t remaining_child_count);
	EXPORT virtual void			child_ends_hook(pwThreadBase child, int code, size_t remaining_child_count);
protected:
	log::Log					log;
	pwThreadBase 				parent_;
	std::atomic<bool>			ending_;
	std::atomic<bool>			interrupted_;
//	bool					end_requested;
//	mutex 					end_lock;
	mutex						finish_lock;

	mutex 						children_lock_;
	std::map<pwThreadBase, ThreadChild, std::owner_less<pwThreadBase>>
								children_;

	mutex 						timer_lock;
	duration_t					join_timeout;
//	int 					exitCode;
//	bool 						finishWhenChildEnds;
//	bool					quitWhenChildsEnd;
//	pid_t 						own_tid;
private:
	std::vector<pwThreadBase>	ending_childs_;
	mutex						ending_childs_mutex_;
	position_t	 				cpu_affinity_;
	std::atomic<bool>			running_;
	std::string 				node_id_;
	std::string					node_name_;

public:
	EXPORT static void 				sleep (const duration_t& us);
};


template<typename T> bool ThreadBase::set_param(const std::string& name, const T& value)
{
	return set_param(Parameter (name,value));
}

}
}
#endif /*THREADBASE_H_*/
