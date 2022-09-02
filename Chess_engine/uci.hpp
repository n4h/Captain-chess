#ifndef UCI_H
#define UCI_H

#include <vector>
#include <string>
#include <atomic>
#include <future>

#include "board.hpp"
#include "engine.hpp"
#include "searchflags.hpp"


// from Stockfish
enum SyncCout {ioLock, ioUnlock};
std::ostream& operator<<(std::ostream&, SyncCout);
#define sync_cout std::cout << ioLock
#define sync_endl std::endl << ioUnlock

namespace uci
{
	class UCIProtocol
	{
	public:
		void UCIStartup();
		void UCIStartLoop();
		UCIProtocol() {};
	private:
		void UCIPositionCommand(const std::vector<std::string>&);
		void UCIGoCommand(const std::vector<std::string>&);
		void UCIStopCommand();
		std::string UCIName = "Captain";
		std::string UCIAuthor = "Narbeh Mouradian";
		bool bitboardsInitialized = false;
		board::Board b;
		engine::Engine e;
		std::future<void> engineResult;
		searchFlags sf;

		friend class engine::Engine;
	};

	board::Move uciMove2boardMove(const board::Board&, const std::string&);
}


#endif
