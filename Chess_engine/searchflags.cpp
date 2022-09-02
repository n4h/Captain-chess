#include <atomic>

#include "searchflags.hpp"

std::atomic_flag searchFlags::searching = ATOMIC_FLAG_INIT;
std::atomic_flag searchFlags::ponder = ATOMIC_FLAG_INIT;