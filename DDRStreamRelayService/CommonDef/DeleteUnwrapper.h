#ifndef __DELETE_UNWRAPPER_RECURSION_AVOIDANCE_H_INCLUDED__
#define __DELETE_UNWRAPPER_RECURSION_AVOIDANCE_H_INCLUDED__

namespace DDRCommonDef
{
	template <typename T>
	class DefaultDelete : public T
	{
	public:
		void operator delete(void* p) {
			::operator delete(p);
		}
	};
}

#endif // __DELETE_UNWRAPPER_RECURSION_AVOIDANCE_H_INCLUDED__