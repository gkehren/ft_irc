#pragma once
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <fcntl.h>
#include <poll.h>
#include "Channel.hpp"
#include "User.hpp"

#define MAX_QUEUED 1000

class User;
class Channel;

class Server
{
	typedef int(Server::*cmd_f)(std::vector<std::string>, User &);
	/*	MEMBER VAR	*/
	private:
		int								_sockfd;
		struct sockaddr_in				_addr;
		std::string						_password;
		std::vector<pollfd>				_sockets;
		std::map<int, User>				_users;
		std::vector<std::string>		_nicks;
		std::map<std::string, cmd_f>	_cmd_functions;
		std::map<std::string, Channel>	_channels;

		void									init_cmd_functions();
		/*	UTILS	*/
		void									send_client(std::string msg, User user);
		void									send_client(std::string msg, User user_from, User user_to);
		void									delete_socket(pollfd fd);
		void									disconnect(User user);
		int										new_socket();
		int										new_msg(int &i);
		std::vector<std::vector<std::string> >	parsing_msg(std::string msg);
		std::vector<std::string> 				params_channel(std::string params);
		bool									isUser(std::string const &nick);
		bool									isChannel(std::string channel);
		int										get_user_fd(std::string nick);
		std::map<int, User>::iterator 			find_user_by_name(std::string name);

		/*	CMD	*/
		bool	check_pass(std::vector<std::vector<std::string> > input);
		void	monitor_cmd(std::vector<std::vector<std::string> > input, int user_fd);

		// Connection
		int		cmd_password(std::vector<std::string> params, User &user);
		int 	is_auth_nick(User &user);
		int		cmd_nick(std::vector<std::string> params, User &user);
		int		cmd_user(std::vector<std::string> params, User &user);
		int		cmd_ping(std::vector<std::string> params, User &user);
		void	monitor_ping();
		void	send_ping(User &user);
		int		cmd_pong(std::vector<std::string> params, User &user);
		int		cmd_quit(std::vector<std::string> params, User &user);
		int		cmd_oper(std::vector<std::string> params, User &user);

		// Channel
		int		cmd_list(std::vector<std::string> params, User &user);
		void	send_info_join(Channel &channel, std::string title, User &user);
		int		cmd_join(std::vector<std::string> params, User &user);
		int		cmd_part(std::vector<std::string> params, User &user);
		int		cmd_topic(std::vector<std::string> params, User &user);
		int		cmd_names(std::vector<std::string> params, User &user);
		int		cmd_privmsg(std::vector<std::string> params, User &user);
		int		cmd_invite(std::vector<std::string> params, User &user);


		int		cmd_who(std::vector<std::string> params, User &user);
		int		cmd_motd(std::vector<std::string> params, User &user);
		int		cmd_notice(std::vector<std::string> params, User &user);
		int		cmd_info(std::vector<std::string> params, User &user);

		//Operator
		int		cmd_kill(std::vector<std::string> params, User &user);

		// Utils
		void	pop_back_str(std::string &str);

	public:
		/*	CONSTRUCTOR / DESTRUCTOR	*/
		Server(int port, std::string password);
		virtual ~Server();

		/*	GETTER	*/
		int		getSocket() const;

		/*	FUNCTIONS	*/
		int		run();
		// TMP
		void	add_channel(Channel channel);
};

