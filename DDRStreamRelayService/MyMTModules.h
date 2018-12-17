/**
C++ style cross-platform multi-thread libraries by DADAO.
Apr 2018, Jianrui Long @ DADAOII
*/

#ifndef __DDR_MULTITHREADING_LIB_CPP11_H_INCLUDED__
#define __DDR_MULTITHREADING_LIB_CPP11_H_INCLUDED__

namespace DDRMTLib
{
	// *********** C++ 11 wrapper (cross-platform) for windows-style event ***********
	class Event
	{
	public:
		Event(bool bManualReset = true, bool bSignaled = false);
		~Event();
		bool Wait(int nWaitTimeInMilliseconds);
		bool Set();
		bool Reset();
	private:
		struct _impl;
		_impl *m_pImp;
	};

	// ************ Base class for main-sub models (C++ 11 cross-platform) ************
	//
	// A main-sub-threads model for multitasking. The main thread is establishing
	// this object, setting up unmodifiable parameters, and lauching/pausing/resuming
	// sub-threads. Sub-threads are informed to run once when eventIN is set, and will
	// set eventOUT when one round is finished. The main thread can use certain
	// function (Respond()) to check running status of sub-threads, and assign another
	// round of task when the previous one is finished. Typical usage is in the case
	// where the main thread is running in a light weight (real-time) processing and
	// communicating loop, while sub-threads are assigned time-consuming taks.
	//
	// Workflow:	1. setting up (by calling constructor) parameters that can not
	//                  be changed later.
	//				2. Launch();
	//				3. Pause() / Resume()
	//				4. Use Respond() to check working status
	//
	// Usage:		1. Derive from this base class
	//				2. Implement you own "bool _onRunOnce_Sub(int)"
	//				3. Optionally implement onSubsBusy_Main(), processFinishedTask_Main,
	//				   isTaskAllowed_Main(), _onSuspension_Sub(), _onSummarizing_sub(),
	//				   _onIdlingOnce_Sub(), and/or _onCleaningUp_Sub().
	//
	class MainSubsModel
	{
	public:
		MainSubsModel(int nSubThreads, // number of sub threads
					  int waitTimeSubs4MainMS, // sub threads' waiting time for eventIN 
			          int waitTimeMain4SubsMS, // main thread's waiting for eventOUT
					  bool bForceEventIn = false // when Respond is called, forced to set eventIN or only when eventOUT is set
					  );
		// for any derived class, define its own destructor that explicitly call EndSubThreads()
		// because otherwise virtual functions such as _onCleaningUp_Sub() will be called during
		// the construction of this base class and will result in WRONG FUNCTION CALLS!
		virtual ~MainSubsModel();
		
		bool Launch(bool bLaunchSuspended = false);
		bool Resume();
		bool Pause();
		void EndSubThreads();

		// called by main thread to check if sub-threads have finished latest round of task
		bool AreSubThreadsBusy();

		// called by main thread to: ~ check if eventOUT is signaled
		//                           ~ process results of finished tasks if any
		//							 ~ put sub threads back to work if allowed (isTaskAllowed_Main())
		// return value: bit #0 - eventOUT signaled
		//               bit #1 - task results have been available and processed
		//               bit #2 - eventIN set to signaled state
		//				 bit #3 - sub threads still running
		//               bit #31 - error
		int Process();

		// call ed by main thread to check if eventOUT is signaled during last call to Process()
		bool IsLastCheckON() const { return m_bLastCheckOn; }

		// get current multi-threading status
		// 0 - uninitialized
		// 1 - initialized
		// 2 - launched and running
		// 3 - launched and suspended
		int GetMTStatus() const;

	protected:
		// ****************** madatory implementations for derivation ******************

		// called if eventIN is signaled. major function to actually execute tasks (in sub-threads).
		// return FALSE if exceptions occur, otherwise return TRUE
		virtual bool _onRunOnce_Sub(int subThreadID) = 0;

		// ****************** optional implementations to override ******************

		// called when sub-threads are still busy finishing tasks
		virtual void onSubsBusy_Main() {}

		// called when eventOUT is signaled. can be used to process finished results
		// (ignored if no task was executed during the latest iteration)
		// return if some task results are processed
		virtual bool processFinishedTask_Main() { return true; }

		// called to determine whether to signal event IN.
		// may also be used to assign parameters for next round of task
		virtual bool isTaskAllowed_Main() { return true; }

		// called when suspended/paused (in sub-threads)
		virtual void _onSuspension_Sub(int subThreadID) {}

		// called (in ONE sub thread) after all sub threads finish a new round of task
		// return FALSE if quitting condition is met, otherwise return TRUE
		virtual bool _onSummarizing_sub(bool bFinishedJustNow) { return true; }

		// called if all sub threads idle this round
		virtual void _onIdlingOnce_Sub(int subThreadID) {}

		// called when sub-threads are quitting
		virtual void _onCleaningUp_Sub(int subThreadID) {}

		void setEventIN_Main();
		void resetEventOUT_Main();

		bool m_bException;
		bool m_bLastCheckOn;

	private:
		struct _impl;
		_impl *m_pImp;
	};
}

#endif