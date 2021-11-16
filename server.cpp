#include "Connection.hpp"

#include "hex_dump.hpp"
#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <unordered_map>
#include "NetworkPlayer.hpp"
#include <unordered_set>

#ifdef _WIN32
extern "C" { uint32_t GetACP(); }
#endif
int main(int argc, char **argv) {
#ifdef _WIN32
	{ //when compiled on windows, check that code page is forced to utf-8 (makes file loading/saving work right):
		//see: https://docs.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
		uint32_t code_page = GetACP();
		if (code_page == 65001) {
			std::cout << "Code page is properly set to UTF-8." << std::endl;
		} else {
			std::cout << "WARNING: code page is set to " << code_page << " instead of 65001 (UTF-8). Some file handling functions may fail." << std::endl;
		}
	}

	//when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
	try {
#endif

	//------------ argument parsing ------------

	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	//------------ initialization ------------

	Server server(argv[1]); 


	//------------ main loop ------------
	constexpr float ServerTick = 1.0f / 20.0f; // set a server tick that makes sense for your game

	//server state:
	bool ping = false;

	// clients' state:
	std::unordered_map< Connection *,  Server_Player> players;
	players.reserve(PLAYER_NUM); 

	while (true) {

		// game logic here
		{
			static auto prev_play_time = std::chrono::steady_clock::now();
			auto now = std::chrono::steady_clock::now();
			if(std::chrono::duration< float >(now-prev_play_time).count() >= 3.0f){
				prev_play_time = std::chrono::steady_clock::now();
				ping = true;
			}
			else{
				ping = false;
			}	
		}

		static auto next_tick = std::chrono::steady_clock::now() + std::chrono::duration< double >(ServerTick);

		//hit list
		std::unordered_set<int> hit_list;

		//process incoming data from clients until a tick has elapsed:
		while (true) {
			auto now = std::chrono::steady_clock::now();
			double remain = std::chrono::duration< double >(next_tick - now).count();
			if (remain < 0.0) {
				next_tick += std::chrono::duration< double >(ServerTick);
				break;
			}
			server.poll([&](Connection *c, Connection::Event evt){
				if (evt == Connection::OnOpen) {
					//client connected:
					// refuse connection if over size
					if(players.size() >= PLAYER_NUM){
						std::cout << "Over Limit\n";
						//shut down client connection:
						c->close();
						return;
					}
					//create some player info for them:
					players.emplace(c, Server_Player());
				} 
				else if (evt == Connection::OnClose) {
					//client disconnected:
					std::cout << "Closing\n";
					//remove them from the players list:
					auto f = players.find(c);
					assert(f != players.end());
					players.erase(f);
					// make its id available
					Server_Player::id_used[f->second.id-1] = false;
				} 
				else { assert(evt == Connection::OnRecv);
					//got data from client:
					//std::cout << "got bytes:\n" << hex_dump(c->recv_buffer); std::cout.flush();

					//look up in players list:
					auto f = players.find(c);
					assert(f != players.end());
					Server_Player &player = f->second;

					// ----------- handle messages from client ---------- //
					uint8_t hit_id;
					player.read_from_message(c, hit_id);
					hit_list.insert(hit_id);
				}
			}, remain);
		}

		// ----------- send updated game state to all clients -------------- //
		for (auto &[c, player] : players) {
			(void)player; //work around "unused variable" warning on whatever g++ github actions uses
			// construct a status message
			std::vector<unsigned char> server_message;

			// ------- first put public message (same for all player) ------- //
			server_message.emplace_back((unsigned char)ping);

			// ------- put all players' infos -------- //
			// put the info of client itself at first
			if (hit_list.find(player.id) != hit_list.end()) {
				player.gotHit = true;
			}
			else {
				player.gotHit = false;
			}
			player.convert_to_message(server_message);
			// put the info of other players 
			for (auto &[c_other, player_other] : players){
				if (c != c_other){
					player_other.convert_to_message(server_message);
				}
			}
			//send an update starting with 'm'
			c->send('m');
			// send size
			sizet_as_byte sizet_bytes;
			sizet_bytes.sizet_value = server_message.size();
			for (size_t i =0; i < sizeof(size_t); i++){
				c->send(sizet_bytes.bytes_value[i]);
			}
			// send message
			c->send_buffer.insert(c->send_buffer.end(), server_message.begin(), server_message.end());
		}

	}

	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}
