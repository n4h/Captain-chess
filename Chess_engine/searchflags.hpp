#ifndef SEARCHFLAGS_H
#define SEARCHFLAGS_H
#include <atomic>

class searchFlags
{
public:
	searchFlags() {}
	static std::atomic_flag searching;
	static std::atomic_flag ponder;
};


#endif