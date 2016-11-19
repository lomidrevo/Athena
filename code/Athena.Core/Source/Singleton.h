#ifndef __singleton_h
#define __singleton_h

#include "Mutex.h"


template <class T> class Singleton
{
public:

	static T* Instance()
	{
		singletonAccess.Lock();

		if (!Singleton<T>::instance)
			Singleton<T>::instance = new T();

		singletonAccess.UnLock();

		return Singleton<T>::instance;
	}

	static void Destroy() { SAFE_DELETE(Singleton<T>::instance); }

private:

	static Mutex singletonAccess;
	static T* instance;
};

template <class T>T* Singleton<T>::instance = null;
template <class T>Mutex Singleton<T>::singletonAccess;

#endif __singleton_h
