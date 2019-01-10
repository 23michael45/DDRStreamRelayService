#include "../AVStream/MyMTModules.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
//#include <iostream>

namespace DDRMTLib {

struct Event::_impl {
	bool _bManualReset;
	bool _bSignaled;
	std::mutex _muTex;
	std::condition_variable _condVar;
	int _cntThreadsWaiting;
	_impl(bool bManualReset, bool bSignaled) {
		_bManualReset = bManualReset;
		_bSignaled = bSignaled;
		_cntThreadsWaiting = 0;
	}
	void _Set() {
		_muTex.lock();
		_bSignaled = true;
		_condVar.notify_all();
		_muTex.unlock();
	}
	void _Reset() {
		_muTex.lock();
		_bSignaled = false;
		_muTex.unlock();
	}
	bool _Wait(int nMilliseconds) {
		std::unique_lock<std::mutex> lk(_muTex);
		if (_bSignaled) { return true; }
		++_cntThreadsWaiting;
		bool bSucc = true;
		if (nMilliseconds >= 0) {
			auto _tp = std::chrono::system_clock::now() +
				       std::chrono::milliseconds(nMilliseconds);
			while (1) {
				if (std::cv_status::timeout ==
					_condVar.wait_until(lk, _tp)) {
					bSucc = false;
					break;
				} else if (_bSignaled) {
					bSucc = true;
					break;
				}
			}
		} else {
			while (1) {
				_condVar.wait(lk);
				if (_bSignaled) { break; }
			}
		}
		if (bSucc) {
			if (--_cntThreadsWaiting == 0 && !_bManualReset) {
				_bSignaled = false;
			}
			return true;
		} else { return false; }
	}
};

Event::Event(bool bManualReset, bool bSignaled) {
	m_pImp = new _impl(bManualReset, bSignaled);
}

Event::~Event() {
	if (m_pImp) {
		delete m_pImp;
		m_pImp = nullptr;
	}
}

bool Event::Wait(int nWaitTimeInMilliseconds) {
	if (m_pImp) {
		return m_pImp->_Wait(nWaitTimeInMilliseconds);
	}
	return false;
}

bool Event::Set() {
	if (m_pImp) {
		m_pImp->_Set();
		return true;
	}
	return false;
}

bool Event::Reset() {
	if (m_pImp) {
		m_pImp->_Reset();
		return true;
	}
	return false;
}

struct MainSubsModel::_impl {
	int nStage;
	int nThreads;
	int wtIn;
	int wtOut;
	bool bSetEvenINRegardless;

	std::thread *pThreads;

	bool sigNotSuspended;
	std::mutex mutexNotSuspended;
	std::condition_variable condVarNotSuspended;
	std::atomic<int> thCntSuspended;

	bool sigIn;
	std::mutex mutexIn;
	std::condition_variable condVarIn;

	bool sigOut;
	std::mutex mutexOut;
	std::condition_variable condVarOut;

	bool bQuit;
	bool threadsEventInWaitResult;
	bool threadsQuit;
	std::atomic<int> thCntBarrier[2];

	_impl(int, int, int, bool);

	static void _CoreThreadFunc(MainSubsModel*, int);
};

MainSubsModel::_impl::_impl(int nSubThreads,
	                        int waitTimeSubs4MainMS,
							int waitTimeMain4SubsMS,
							bool bForceEventIn) {
	nStage = 0; // uninitialized
	if (nSubThreads <= 0 || nSubThreads > 50) { return; }
	if (waitTimeSubs4MainMS < 0) {
		waitTimeSubs4MainMS = 0;
	} else if (waitTimeSubs4MainMS > 1000) {
		waitTimeSubs4MainMS = 1000;
	}
	if (waitTimeMain4SubsMS < 0) {
		waitTimeMain4SubsMS = 0;
	} else if (waitTimeMain4SubsMS > 10) {
		waitTimeMain4SubsMS = 10;
	}
	nThreads = nSubThreads;
	pThreads = nullptr;
	wtIn = waitTimeSubs4MainMS;
	wtOut = waitTimeMain4SubsMS;
	bSetEvenINRegardless = bForceEventIn;

	sigNotSuspended = true;
	sigIn = false;
	sigOut = true;
	thCntBarrier[0] = 0;
	thCntBarrier[1] = 0;
	thCntSuspended = 0;
	bQuit = false;
	threadsEventInWaitResult = false;
	threadsQuit = false;
	nStage = 1; // initialized
}

void MainSubsModel::_impl::_CoreThreadFunc(MainSubsModel *pMSM, int subThreadID) {
	MainSubsModel::_impl *pImpl = pMSM->m_pImp;
	while (1) {
		// stage I. through event NotSuspended
		if (1) { 
			std::unique_lock<std::mutex> lk(pImpl->mutexNotSuspended);
			if (!pImpl->sigNotSuspended) {
				lk.unlock();
				pMSM->_onSuspension_Sub(subThreadID); // on suspension
				lk.lock();
				++pImpl->thCntSuspended;
				while (1) {
					pImpl->condVarNotSuspended.wait(lk);
					if (pImpl->sigNotSuspended) {
						--pImpl->thCntSuspended;
						break;
					}
				}
			}
		}

		// stage II, through barrier #0
		// the last thread is responsible for checking eventIN
		// and assigning the COMMON wait flag
		if (++pImpl->thCntBarrier[0] == pImpl->nThreads) {
			pImpl->threadsEventInWaitResult = false;
			std::unique_lock<std::mutex> lk(pImpl->mutexIn);
			if (pImpl->sigIn) {
				pImpl->threadsEventInWaitResult = true;
				pImpl->sigIn = false; // resetting eventIN
			} else {
				auto _tp = std::chrono::system_clock::now() +
					       std::chrono::milliseconds(pImpl->wtIn);
				while (1) {
					if (std::cv_status::timeout == pImpl->condVarIn.wait_until(lk, _tp)) {
						break;
					} else if (pImpl->sigIn) {
						pImpl->threadsEventInWaitResult = true;
						pImpl->sigIn = false; // resetting eventIN
						break;
					}
				}
			}
			pImpl->thCntBarrier[0] = 0;
		}
		// threads waiting for the last thread to arrive at this checkpoint #0
		while (pImpl->thCntBarrier[0] != 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		// stage III, core working routine
		// every sub thread checking the COMMON wait result and exception status
		if (pImpl->threadsEventInWaitResult) {
			// run once, and record exceptions
			if (!pMSM->_onRunOnce_Sub(subThreadID)) {
				pMSM->m_bException = true;
			}
		} else {
			pMSM->_onIdlingOnce_Sub(subThreadID); // idle once
		}

		// stage IV, barrier #1
		// the last thread is responsible for setting eventOUT
		// and assigning the COMMON quit flag
		if (++pImpl->thCntBarrier[1] == pImpl->nThreads) {
			bool _bQuit = !pMSM->_onSummarizing_sub(pImpl->threadsEventInWaitResult);
			pImpl->mutexOut.lock();
			pImpl->sigOut = true;
			pImpl->condVarOut.notify_all(); // setting eventOUT
			pImpl->mutexOut.unlock();
			pImpl->threadsQuit = _bQuit || pImpl->bQuit || pMSM->m_bException;
			pImpl->thCntBarrier[1] = 0;
		}
		// threads waiting for the last thread to arrive at this checkpoint #1
		while (pImpl->thCntBarrier[1] != 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		
		// stage V, checking the COMMON quit flag
		if (pImpl->threadsQuit) {
			break;
		}
	}

	pMSM->_onCleaningUp_Sub(subThreadID); // on cleaning up
}

MainSubsModel::MainSubsModel(int nSubThreads,
	                         int waitTimeSubs4MainMS,
							 int waitTimeMain4SubsMS,
							 bool bForceEventIn) {
	m_pImp = new _impl(nSubThreads, waitTimeSubs4MainMS,
		               waitTimeMain4SubsMS, bForceEventIn);
	m_bException = false;
	m_bLastCheckOn = false;
}

MainSubsModel::~MainSubsModel() {
	if (m_pImp) {
		EndSubThreads();
		delete m_pImp;
		m_pImp = nullptr;
	}
}

bool MainSubsModel::Launch(bool bLaunchSuspended) {
	if (!m_pImp) { return false; }
	if (m_pImp->nStage != 1) { return false; }

	m_pImp->sigNotSuspended = !bLaunchSuspended;
	m_pImp->pThreads = new std::thread[m_pImp->nThreads];
	for (int i = 0; i < m_pImp->nThreads; ++i) {
		m_pImp->pThreads[i] = std::thread(_impl::_CoreThreadFunc, this, i);
	}
	m_pImp->nStage = bLaunchSuspended ? 3 : 2;
	return true;
}

bool MainSubsModel::Resume() {
	if (!m_pImp) { return false; }
	if (m_pImp->nStage != 3) { return false; }
	std::lock_guard<std::mutex> _lk(m_pImp->mutexNotSuspended);
	m_pImp->sigNotSuspended = true;
	m_pImp->condVarNotSuspended.notify_all();
	m_pImp->nStage = 2;
	return true;
}

bool MainSubsModel::Pause() {
	if (!m_pImp) { return false; }
	if (m_pImp->nStage != 2) { return false; }
	m_pImp->mutexNotSuspended.lock();
	m_pImp->sigNotSuspended = false;
	m_pImp->nStage = 3;
	m_pImp->mutexNotSuspended.unlock();
	while (m_pImp->thCntSuspended < m_pImp->nThreads) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return true;
}

void MainSubsModel::EndSubThreads() {
	if (!m_pImp) { return; }
	if (m_pImp->nStage < 2) { return; }
	m_pImp->bQuit = true;
	Resume();
	for (int i = 0; i < m_pImp->nThreads; ++i) {
		if (m_pImp->pThreads[i].joinable()) {
			m_pImp->pThreads[i].join();
		}
	}
	delete[] m_pImp->pThreads;
	m_pImp->nStage = 1;
}

bool MainSubsModel::AreSubThreadsBusy() {
	std::lock_guard<std::mutex> lk(m_pImp->mutexOut);
	return (!m_pImp->sigOut);
}

int MainSubsModel::Process() {
	if (!m_pImp) { return 0x80000000; }
	if (m_pImp->nStage != 2) { return 0x80000000; }
	if (m_pImp->threadsQuit) { return 0; }
	int ret = 0x08;

	// check if eventOUT is signaled
	m_bLastCheckOn = false;
	std::unique_lock<std::mutex> lk(m_pImp->mutexOut);
	if (m_pImp->sigOut) {
		m_bLastCheckOn = true;
	} else {
		auto _tp = std::chrono::system_clock::now() +
			       std::chrono::milliseconds(m_pImp->wtOut);
		while (1) {
			if (std::cv_status::timeout == m_pImp->condVarOut.wait_until(lk, _tp)) {
				m_bLastCheckOn = false;
				break;
			} else if (m_pImp->sigOut) {
				m_bLastCheckOn = true;
				break;
			}
		}
	}

	if (!m_bLastCheckOn) {
		onSubsBusy_Main(); // on SubThreads Busy
		if (!m_pImp->bSetEvenINRegardless) { return ret; }
	} else {
		ret |= 0x01;
		// processing results
		if (processFinishedTask_Main()) {
			ret |= 0x02;
		}
		m_pImp->sigOut = false; // resetting eventOUT
		lk.unlock();
	}

	if (isTaskAllowed_Main()) {
		std::lock_guard<std::mutex> lk_(m_pImp->mutexIn);
		m_pImp->sigIn = true;
		m_pImp->condVarIn.notify_all();
		ret |= 0x04;
	}
	return ret;
}

int MainSubsModel::GetMTStatus() const { return m_pImp->nStage; }

void MainSubsModel::setEventIN_Main() {
	std::lock_guard<std::mutex> lk_(m_pImp->mutexIn);
	m_pImp->sigIn = true;
	m_pImp->condVarIn.notify_all();
}

void MainSubsModel::resetEventOUT_Main() {
	std::lock_guard<std::mutex> lk_(m_pImp->mutexOut);
	m_pImp->sigOut = false;
}

}