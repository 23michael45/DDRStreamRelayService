/**
Text-to-Speech(TTS) service with channel selection.
The caller call issue a request to covert and play
texts with specification of a channel. This request
will be fullfilled if:
~ no speech is being played
~ the channel currently playing has a lower priority
than the requested channel
In this case, the speech being played will be stopped,
and requested texts will be played.

带通道选择的文字转语音(TTS)服务.
调用者可以对指定通道发起TTS服务请求. 满足以下条件时请求将
得到响应:
~ 当前未播放语音
~ 当前播放语音的通道等级低于请求指定的通道
得到响应后, 当前播放的语音被停止, 请求的文字将被播放.
*/
#ifndef __TTS_QUEUE_PLAYER_H_INCLUDED__
#define __TTS_QUEUE_PLAYER_H_INCLUDED__

namespace DDRGadgets
{
	class MultiChannelPlayer
	{
	public:
		MultiChannelPlayer(unsigned int nTotalChannels = 8);
		void Quit();
		~MultiChannelPlayer() { Quit(); }
		
		bool Play(const char *pText2Play, // for Chinese characters, use GB encoding
			      unsigned int nChannel = 1,
			      unsigned int nIntervals = 0); // millisecond

		bool PlayFile(const char *pPlayPath,
			      unsigned int nChannel = 1,
			      unsigned int nIntervals = 0);

		bool StopCyclePlay();
				  
		unsigned int GetCurrentTTSChannel();
		bool BlockLowerChannels(unsigned int minChannel);
		bool ClearChannelBlocking();

	private:
		struct _strIMPL;
		_strIMPL *m_pIMPL;
	};

	extern MultiChannelPlayer g_TTSPlayer;
	
	// 发起播放TTS请求, 附带通道的指定，以及循环播放时间
	inline bool PlayVoice(const char *pText2Play, unsigned int nChannel = 1,unsigned int nIntervals = 0) 
	{
		return g_TTSPlayer.Play(pText2Play, nChannel, nIntervals);
	}

	// 发起播放WAV文件请求, 附带通道的指定，以及循环播放时间
	inline bool PlayFile(const char *pText2Play, unsigned int nChannel = 1, unsigned int nIntervals = 0)
	{
		return g_TTSPlayer.PlayFile(pText2Play, nChannel, nIntervals);
	}

	// 停止循环播放
	inline bool StopCyclePlay()
	{
		return g_TTSPlayer.StopCyclePlay();
	}
	
	// 获取当前正在播放的通道号
	inline unsigned int GetCurrentTTSChannel() {
		return g_TTSPlayer.GetCurrentTTSChannel();
	}
	// 设定可以播放的最低通道号(在特定模式下可用于屏蔽低等级语音的播放)
	inline bool BlockLowerChannels(unsigned int minChannel) {
		return g_TTSPlayer.BlockLowerChannels(minChannel);
	}
	//去掉播放的最低通道号限制
	inline bool ClearChannelBlocking() {
		return g_TTSPlayer.ClearChannelBlocking();
	}
}

#endif // __TTS_QUEUE_PLAYER_H_INCLUDED__