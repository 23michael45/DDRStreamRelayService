#include "TTSQueuePlayer.h"

#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <fstream>
#include <iostream>

#include <windows.h>
#include "mmsystem.h"
#pragma comment(lib, "winmm.lib")

namespace DDRGadgets {

	MultiChannelPlayer g_TTSPlayer(16);

	struct MultiChannelPlayer::_strIMPL {
		unsigned int m_nChannels;
		std::mutex m_mutex;
		int m_queuedChannel;
		std::string m_queuedTxt;
		int m_nType; // 0 - TTS. 1 - play file
		unsigned int m_curChannel;
		std::string m_curText;
		unsigned int m_minChannel;
		std::string m_strPlayFile;
		std::chrono::system_clock::time_point m_curSoundEndingTP;
		bool m_bRequested2Quit;
		bool m_bSubThreadActive;
		std::thread m_thread;

		bool m_bInLoopPlay;
		std::string m_strLoopPlayFile;
		std::string m_strQueuedLoopText;
		int m_nLoopType; // 0 - TTS. 1 - play file. else cancel loop
		int m_nLoopQueuedChannel;
		int m_nLoopTime;
		bool m_bFirstPlay;
		bool m_bQuitPlay;

		void Quit();
		_strIMPL(unsigned int nChannels);
		~_strIMPL() { Quit(); }
		bool Play(const char *pText2Play, unsigned int nChannel, int nIntervals = 0);
		bool PlayFile(const char *pText2Play, unsigned int nChannel, int nIntervals = 0);
		unsigned int GetCurChannel();
		bool StopCyclePlay();
		void _myLoop();
		static void _subThreadFunc(void *pArg);
	};

	MultiChannelPlayer::_strIMPL::_strIMPL(unsigned int nChannels) {
		m_nChannels = nChannels;
		m_queuedChannel = m_curChannel = -1;
		m_bRequested2Quit = false;
		m_bSubThreadActive = false;
		m_thread = std::thread(_subThreadFunc, (void*)this);
		m_bSubThreadActive = true;
		m_minChannel = 0;
		m_nType = 0;

		m_bInLoopPlay = false;
		m_strLoopPlayFile = "";
		m_strQueuedLoopText = "";
		m_nLoopType = -1; // 0 - TTS. 1 - play file. else cancel loop
		m_nLoopQueuedChannel = -1;
		m_nLoopTime = -1;
		m_bFirstPlay = false;
		m_bQuitPlay = false;
	}

	void MultiChannelPlayer::_strIMPL::Quit() {
		if (m_bSubThreadActive) {
			m_bRequested2Quit = true;
			m_thread.join();
			m_bSubThreadActive = false;
			::system("if exist audio\\1.txt del audio\\1.txt");
			::system("if exist audio\\1.wav del audio\\1.wav");
			WIN32_FIND_DATAA fd;
			HANDLE hFind = ::FindFirstFileA("audio\\msc\\*.*", &fd);
			std::string sss;
			while (hFind != INVALID_HANDLE_VALUE) {
				if (strcmp(fd.cFileName, "res") != 0 &&
					strcmp(fd.cFileName, ".") != 0 &&
					strcmp(fd.cFileName, "..") != 0) {
					if (fd.dwFileAttributes & 0x10) {
						sss = "audio\\msc\\";
						sss.append(fd.cFileName);
						sss = "rmdir /S/Q " + sss;
						::system(sss.c_str());
					}
					else if (fd.dwFileAttributes & 0x20) {
						sss = "audio\\msc\\";
						sss.append(fd.cFileName);
						sss = "del " + sss;
						::system(sss.c_str());
					}
				}
				if (!::FindNextFileA(hFind, &fd)) { break; }
			}
		}
	}

	bool MultiChannelPlayer::_strIMPL::Play(const char *pText2Play, unsigned int nChannel, int nIntervals/* = 0*/)
	{
		if (!pText2Play) { return false; }
		if (nChannel >= m_nChannels) {
			nChannel = m_nChannels - 1;
		}
		if (nChannel < m_minChannel) { return false; }
		std::lock_guard<std::mutex> lg(m_mutex);

		if(nIntervals > 0)
		{
			if (m_curChannel != -1 && nChannel <= (int)m_curChannel)
			{
				//std::cout << "Loop play error1" << std::endl;
				return false;
			}

			if ((-1 != m_nLoopQueuedChannel) && (nChannel <= m_nLoopQueuedChannel))
			{
				//std::cout << "Loop play error2" << std::endl;
				return false;
			}

			m_strQueuedLoopText = pText2Play;
			m_nLoopType = 0;
			m_nLoopQueuedChannel = nChannel;
			m_nLoopTime = nIntervals;
			m_bFirstPlay = true;
			return true;
		}
		
		if ((-1 == m_curChannel || nChannel > m_curChannel) &&
			(-1 == m_queuedChannel || nChannel >= m_queuedChannel))
		{
			m_queuedChannel = nChannel;
			m_queuedTxt = pText2Play;
			m_nType = 0;
			return true;
		}
		return false;
	}

	bool MultiChannelPlayer::_strIMPL::PlayFile(const char *pText2Play, unsigned int nChannel, int nIntervals/* = 0*/)
	{
		if (!pText2Play) { return false; }
		if (nChannel >= m_nChannels) {
			nChannel = m_nChannels - 1;
		}
		if (nChannel < m_minChannel) { return false; }
		std::lock_guard<std::mutex> lg(m_mutex);

		if(nIntervals > 0)
		{
			if (m_curChannel != -1 && nChannel <= (int)m_curChannel)
			{
				//std::cout << "Loop playfile error1" << std::endl;
				return false;
			}

			if ((-1 != m_nLoopQueuedChannel) && (nChannel <= m_nLoopQueuedChannel))
			{
				//std::cout << "Loop playfile error2" << std::endl;
				return false;
			}

			m_strLoopPlayFile = pText2Play;
			m_nLoopType = 1;
			m_nLoopQueuedChannel = nChannel;
			m_nLoopTime = nIntervals;
			m_bFirstPlay = true;
			return true;
		}

		
		if ((-1 == m_curChannel || nChannel > m_curChannel) &&
			(-1 == m_queuedChannel || nChannel >= m_queuedChannel)) {
			m_queuedChannel = nChannel;
			m_strPlayFile = pText2Play;
			m_nType = 1;
			return true;
		}
		return false;
	}

	bool MultiChannelPlayer::_strIMPL::StopCyclePlay()
	{
		//std::cout << "MultiChannelPlayer::_strIMPL::StopCyclePlay() +++" << std::endl;
		if(-1 == m_nLoopQueuedChannel)
		{
			std::cout << "MultiChannelPlayer::_strIMPL::StopCyclePlay() error" << std::endl;
			return false;
		}
		m_strLoopPlayFile = "";
		m_strQueuedLoopText = "";
		m_nLoopType = -1;
		m_nLoopQueuedChannel = -1;
		m_bQuitPlay = true;
		return false;
	}

	unsigned int MultiChannelPlayer::_strIMPL::GetCurChannel() {
		std::lock_guard<std::mutex> lg(m_mutex);
		return m_curChannel;
	}

	void MultiChannelPlayer::_strIMPL::_subThreadFunc(void *pArg) {
		_strIMPL *pThis = (_strIMPL*)pArg;
		pThis->_myLoop();
		
	}

	/*
		在这里根据m_queuedChannel m_queuedTxt 等播放一次的数据
		和 m_curLoopQueuedChannel m_curLoopText 等循环播放的数据进行控制播放
		*/
	void MultiChannelPlayer::_strIMPL::_myLoop()
	{
		char waveInfo[100];
		while (1)
		{
			Sleep(1);
			if (m_bRequested2Quit) { break; }
			
			if (m_curChannel != -1)
			{
				if (std::chrono::system_clock::now() >= m_curSoundEndingTP)
				{	
					m_curChannel = -1;
				}
			}
			
			m_mutex.lock();
			if (m_queuedChannel != -1 || m_nLoopQueuedChannel != -1)
			{
				if (-1 == m_curChannel || m_queuedChannel > m_curChannel || m_nLoopQueuedChannel >= m_curChannel)
				{
					std::string strWAVPath = "";
					std::string strTxtPath = "audio\\1.txt";
			
					if (m_queuedChannel > m_nLoopQueuedChannel) // 播放一次的
					{	
						if (m_curChannel != -1)
						{
							::PlaySoundA(nullptr, nullptr, SND_ASYNC);
						}
						
						if (0 == m_nType)
						{
							strTxtPath = "audio\\1.txt";
							strWAVPath = "audio\\1.wav";
							m_curChannel = m_queuedChannel;
							m_curText = m_queuedTxt;
							m_queuedChannel = -1;

							m_mutex.unlock();
							if("" == m_curText)
								continue;
							std::ofstream ofs;
							ofs.open(strTxtPath.c_str(), std::fstream::out);
							ofs << m_curText;
							ofs.close();
							::system("audio\\tts_all_offline.exe audio\\");
						}
						else if (1 == m_nType)
						{
							m_curChannel = m_queuedChannel;
							m_queuedChannel = -1;
							strWAVPath = m_strPlayFile;
							m_mutex.unlock();
						}
						else
						{
							m_mutex.unlock();
						}
					}
					else  // 循环播放
					{
						std::chrono::system_clock::time_point nowTic = std::chrono::system_clock::now();
						__int64 tdiff = std::chrono::duration_cast<std::chrono::milliseconds>(nowTic - m_curSoundEndingTP).count();

						bool bIsInterrupt = false;
						if ((m_nLoopQueuedChannel > m_curChannel) && (-1 != m_curChannel))
						{
							::PlaySoundA(nullptr, nullptr, SND_ASYNC);
							bIsInterrupt = true;
						}
						
						if ((tdiff > m_nLoopTime) || m_bFirstPlay || bIsInterrupt)
						{
							bIsInterrupt = false;
							m_bFirstPlay = false;
							if (0 == m_nLoopType)
							{
								strTxtPath = "audio\\1.txt";
								strWAVPath = "audio\\1.wav";
								m_curChannel = m_nLoopQueuedChannel;
								m_curText = m_strQueuedLoopText;
								
								m_mutex.unlock();
								std::ofstream ofs;
								ofs.open(strTxtPath.c_str(), std::fstream::out);
								ofs << m_curText;
								ofs.close();
								::system("audio\\tts_all_offline.exe audio\\");
							}
							else if (1 == m_nLoopType)
							{
								m_curChannel = m_nLoopQueuedChannel;
								strWAVPath = m_strLoopPlayFile;
								m_mutex.unlock();
							}
							else
							{
								m_mutex.unlock();
							}
						}
						else
						{
							m_mutex.unlock();
							continue;
						}
					}

					std::string strMciSend = std::string("open ") + strWAVPath + std::string(" type waveaudio alias wave");
					::mciSendStringA(strMciSend.c_str(), nullptr, 0, nullptr);
					::mciSendStringA("status wave length", waveInfo, 100, nullptr);
					::mciSendStringA("close wave", nullptr, 0, nullptr);
					int lengthMs = atoi(waveInfo); // Get total play time
					m_curSoundEndingTP = std::chrono::system_clock::now() + std::chrono::milliseconds(lengthMs);
					::PlaySoundA(strWAVPath.c_str(), NULL, (SND_FILENAME | SND_ASYNC));
					continue;
				}
			}

			// 调用了stop立即退出循环播报
			if(m_bQuitPlay)
			{
				if(-1 != m_curChannel)
				{
					::PlaySoundA(nullptr, nullptr, SND_ASYNC);
				}
				m_bQuitPlay = false;
			}

			m_mutex.unlock();
		}
	}

#if 0
	/*
		在这里根据m_queuedChannel m_queuedTxt 等播放一次的数据
		和 m_curLoopQueuedChannel m_curLoopText 等循环播放的数据进行控制播放
		*/
	void MultiChannelPlayer::_strIMPL::_myLoop() 
	{
		char waveInfo[100];
		while (1)
		{
			if (m_bRequested2Quit) { break; }
			if (m_curChannel != -1) 
			{
				if (std::chrono::system_clock::now() >= m_curSoundEndingTP) 
				{
					m_curChannel = -1;
				}
			}
			m_mutex.lock();
			if (m_queuedChannel != -1)
			{
				if (-1 == m_curChannel || m_queuedChannel > m_curChannel) 
				{
					if (m_curChannel != -1) 
					{
						::PlaySoundA(nullptr, nullptr, SND_ASYNC);
					}

					std::string strWAVPath = "";
					std::string strTxtPath = "audio\\1.txt";
					if(0 == m_nType)
					{
						strTxtPath = "audio\\1.txt";
						strWAVPath = "audio\\1.wav";
						m_curChannel = m_queuedChannel;
						m_curText = m_queuedTxt;
						m_queuedChannel = -1;

						m_mutex.unlock();
						std::ofstream ofs;
						ofs.open(strTxtPath.c_str(), std::fstream::out);
						ofs << m_curText;
						ofs.close();
						::system("audio\\tts_all_offline.exe audio\\");
					}
					else
					{
						m_curChannel = m_queuedChannel;
						m_queuedChannel = -1;
						strWAVPath = m_strPlayFile;
						m_mutex.unlock();
					}

					std::string strMciSend = std::string("open ") + strWAVPath + std::string(" type waveaudio alias wave");
					::mciSendStringA(strMciSend.c_str(), nullptr, 0, nullptr);
					::mciSendStringA("status wave length", waveInfo, 100, nullptr);
					::mciSendStringA("close wave", nullptr, 0, nullptr);
					int lengthMs = atoi(waveInfo); // Get total play time
					m_curSoundEndingTP = std::chrono::system_clock::now() + std::chrono::milliseconds(lengthMs);
					::PlaySoundA(strWAVPath.c_str(), NULL, (SND_FILENAME | SND_ASYNC));
					continue;
				}
			}
			m_mutex.unlock();
		}
	}
#endif

	MultiChannelPlayer::MultiChannelPlayer(unsigned int nTotalChannels) {
		if (nTotalChannels < 4) {
			nTotalChannels = 4;
		}
		else if (nTotalChannels > 256) {
			nTotalChannels = 256;
		}
		m_pIMPL = new _strIMPL(nTotalChannels);
	}

	void MultiChannelPlayer::Quit() {
		if (m_pIMPL) {
			m_pIMPL->Quit();
			delete m_pIMPL;
			m_pIMPL = nullptr;
		}
	}

	bool MultiChannelPlayer::Play(const char *pText2Play,
		unsigned int nChannel,
		unsigned int nIntervals)
	{
		if (m_pIMPL) {
			m_pIMPL->Play(pText2Play, nChannel, nIntervals);
			return true;
		}
		return false;
	}

	bool MultiChannelPlayer::PlayFile(const char *pPlayPath,
		unsigned int nChannel/* = 1*/,
		unsigned int nIntervals/* = 0*/)
	{
		if (m_pIMPL) {
			m_pIMPL->PlayFile(pPlayPath, nChannel, nIntervals);
			return true;
		}
		return false;
	}

	bool MultiChannelPlayer::StopCyclePlay()
	{
		if (m_pIMPL)
		{
			m_pIMPL->StopCyclePlay();
			return true;
		}
		return false;
	}

	unsigned int MultiChannelPlayer::GetCurrentTTSChannel() {
		if (m_pIMPL) {
			return m_pIMPL->GetCurChannel();
		}
		return -1;
	}

	bool MultiChannelPlayer::BlockLowerChannels(unsigned int minChannel) {
		if (m_pIMPL) {
			if (minChannel >= m_pIMPL->m_nChannels) {
				minChannel = m_pIMPL->m_nChannels - 1;
			}
			m_pIMPL->m_minChannel = minChannel;
			return true;
		}
		else { return false; }
	}

	bool MultiChannelPlayer::ClearChannelBlocking() {
		if (m_pIMPL) {
			m_pIMPL->m_minChannel = 0;
			return true;
		}
		else { return false; }
	}


}
