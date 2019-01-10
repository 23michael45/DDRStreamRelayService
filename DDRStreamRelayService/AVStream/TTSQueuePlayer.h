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

��ͨ��ѡ�������ת����(TTS)����.
�����߿��Զ�ָ��ͨ������TTS��������. ������������ʱ����
�õ���Ӧ:
~ ��ǰδ��������
~ ��ǰ����������ͨ���ȼ���������ָ����ͨ��
�õ���Ӧ��, ��ǰ���ŵ�������ֹͣ, ��������ֽ�������.
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
	
	// ���𲥷�TTS����, ����ͨ����ָ�����Լ�ѭ������ʱ��
	inline bool PlayVoice(const char *pText2Play, unsigned int nChannel = 1,unsigned int nIntervals = 0) 
	{
		return g_TTSPlayer.Play(pText2Play, nChannel, nIntervals);
	}

	// ���𲥷�WAV�ļ�����, ����ͨ����ָ�����Լ�ѭ������ʱ��
	inline bool PlayFile(const char *pText2Play, unsigned int nChannel = 1, unsigned int nIntervals = 0)
	{
		return g_TTSPlayer.PlayFile(pText2Play, nChannel, nIntervals);
	}

	// ֹͣѭ������
	inline bool StopCyclePlay()
	{
		return g_TTSPlayer.StopCyclePlay();
	}
	
	// ��ȡ��ǰ���ڲ��ŵ�ͨ����
	inline unsigned int GetCurrentTTSChannel() {
		return g_TTSPlayer.GetCurrentTTSChannel();
	}
	// �趨���Բ��ŵ����ͨ����(���ض�ģʽ�¿��������ε͵ȼ������Ĳ���)
	inline bool BlockLowerChannels(unsigned int minChannel) {
		return g_TTSPlayer.BlockLowerChannels(minChannel);
	}
	//ȥ�����ŵ����ͨ��������
	inline bool ClearChannelBlocking() {
		return g_TTSPlayer.ClearChannelBlocking();
	}
}

#endif // __TTS_QUEUE_PLAYER_H_INCLUDED__