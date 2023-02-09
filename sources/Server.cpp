#include "../includes/Server.hpp"
#include "../includes/User.hpp"
#include <algorithm>
#include <string>
#include <sstream>

/*	CONSTRUCTOR / DESTRUCTOR	*/
Server::Server(int port, std::string password)
{
	int on = 1;

	this->_password = password;
	this->_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_sockfd < 0)
	{
		std::perror("Error creating socket");
		exit(1);
	}
	if (setsockopt(this->_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)))
	{
		std::perror("Setsockopt failed");
		close(this->_sockfd);
		exit(1);
	}
	if (fcntl(this->_sockfd, F_SETFL, O_NONBLOCK))
	{
		std::perror("Fcntl failed");
		close(this->_sockfd);
		exit(1);
	}
	this->_addr.sin_family = AF_INET;
	this->_addr.sin_port = htons(port);
	this->_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(this->_sockfd, (struct sockaddr *)&this->_addr, sizeof(this->_addr)) < 0)
	{
		perror("Error binding socket");
		close(this->_sockfd);
		exit(1);
	}

	if (listen(this->_sockfd, MAX_QUEUED) < 0)
	{
		perror("Error listening socket");
		close(this->_sockfd);
		exit(1);
	}
	struct pollfd tmp = {this->_sockfd, POLLIN, 0};
	this->_sockets.push_back(tmp);
	this->init_cmd_functions();
	return ;
}

Server::~Server()
{
	for (std::vector<pollfd>::iterator it = this->_sockets.begin() ; it != this->_sockets.end(); it++)
		close(it->fd);
	return ;
}

/*	GETTER	*/
int Server::getSocket() const { return(this->_sockfd); }

/*	FUNCTIONS	*/
int	Server::run()
{
	while (42)
	{
		int ret = poll(this->_sockets.data(), this->_sockets.size(), -1);
		if (ret < 0)
		{
			std::perror("Error polling");
			return (1);
		}
		for (int i = 0; i < (int)this->_sockets.size(); i++)
		{
			if (this->_sockets[i].revents == 0)
				continue ;
			if (this->_sockets[i].revents != POLLIN)
			{
				std::perror("Error about revents");
				return (1);
			}
			if (this->_sockets[i].fd == this->_sockfd)
			{
				ret = this->new_socket();
				if (ret == 1)
					return (1);
				else if (ret == 2)
					break;
			}
			else
			{
				ret = this->new_msg(i);
				if (ret == 1)
					return (1);
				else if (ret == 2)
					continue;
			}
		}
	}
	return (0);
}


/*	PRIVATE	*/

void	Server::init_cmd_functions()
{
	this->_cmd_functions["PASS"] = &Server::cmd_password;
	this->_cmd_functions["LIST"] = &Server::cmd_list;
	this->_cmd_functions["JOIN"] = &Server::cmd_join;
	// this->_cmd_functions["NICK"] = &Server::cmd_nick;
	// this->_cmd_functions["USER"] = &Server::cmd_user;
	return ;
}

/*	UTILS	*/
bool operator==(const pollfd &lhs, const pollfd &rhs)
{
    return lhs.fd == rhs.fd;
}

void	Server::delete_socket(pollfd pfd)
{
	std::vector<pollfd>::iterator	tmp;

	close(pfd.fd);
	tmp = std::find(this->_sockets.begin(), this->_sockets.end(), pfd);
	this->_sockets.erase(tmp);
	this->_users.erase(pfd.fd);
}

int		Server::new_socket()
{
	int		new_socket = -1;

	std::cout << "Listening socket is readable" << std::endl;
	do
	{
		struct sockaddr_in client_addr;
		socklen_t client_len = sizeof(client_addr);
		new_socket = accept(this->_sockfd, (struct sockaddr*)&client_addr, &client_len);
		if (new_socket < 0)
		{
			if (errno != EWOULDBLOCK)
			{
				std::perror("Accept failed");
				return (1);
			}
			return (2);
		}
		struct pollfd	tmp = {new_socket, POLLIN, 0};
		this->_sockets.push_back(tmp);
		this->_users.insert(std::make_pair(new_socket, User(tmp, client_addr)));
		send(new_socket, ":127.0.0.1 001 test :LETS GO CA MARCHE\r\n", 41, 0);
		std::cout << "New connection, socket fd is " << new_socket << std::endl;
	} while (new_socket != -1);
	return (0);
}

int		Server::new_msg(int &i)
{
	char	msg[1024];
	int		ret = -1;

	ret = recv(this->_sockets[i].fd, &msg, sizeof(msg), 0);
	if (ret < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return (1);
		std::perror("Error receiving data");
		return (1);
	}
	if (ret == 0)
	{
		delete_socket(this->_sockets[i]);
		i--;
		return (2);
	}
	msg[ret] = '\0';
	std::cout << "Data received from " << this->_sockets[i].fd << ": " << msg << std::endl;
	std::string msg_s(msg);
	this->monitor_cmd(this->parsing_msg(msg_s), this->_sockets[i].fd);
	return (0);
}

std::vector<std::vector<std::string> >	Server::parsing_msg(std::string msg)
{
	std::vector<std::vector<std::string> > 	big_v;
	std::istringstream						sstream(msg);
	std::string								s;

	while (std::getline(sstream, s, '\n'))
	{
		std::istringstream			sstream2(s);
		std::string					s2;
		std::vector<std::string>	tmp;
		while (std::getline(sstream2, s2, ' '))
			tmp.push_back(s2);
		big_v.push_back(tmp);
	}
	return (big_v);
}

/*	CMD	*/
void	Server::monitor_cmd(std::vector<std::vector<std::string> > input, int user_fd)
{
	for (std::vector<std::vector<std::string> >::iterator it = input.begin(); it != input.end(); it++)
	{
		std::string									tmp((*it)[0]);
		cmd_f										tmp_func = 0;
		std::map<std::string, cmd_f>::iterator  	tmp_it;

		tmp_it = this->_cmd_functions.find(tmp);
		if (tmp_it == this->_cmd_functions.end())
			break;
		else
		{
			tmp_func = tmp_it->second;
			(*it).erase((*it).begin());
			if (!(this->*tmp_func)((*it), this->_users.find(user_fd)->second))
				break;
		}
	}
}

int		Server::cmd_password(std::vector<std::string> params, User user)
{
	std::cout << "JE SUIS DANS LE CMD PASSWORD" << std::endl;
	std::string pass;
	for (std::vector<std::string>::iterator it = params.begin(); it != params.end(); it++)
		pass = pass + *it;
	if (pass != this->_password) //il faut check le password, pas d'espace...
	{
		delete_socket(user.getFd());
		return (0);
	}
	user.setAut(1);//Rajouter les codes retours
	return (0);
}

int		Server::cmd_nick(std::vector<std::string> params, User user)
{
	std::string nick;
	std::cout << "JE SUIS DANS LA CMD NICK" << std::endl;
	for (std::vector<std::string>::iterator it = params.begin(); it != params.end(); it++)
		nick = nick + *it;
	if (user.getAut())
	{
		user.setNick(nick);
		_nicks.push_back(nick); //Rajouter les codes retours
		return (1);
	}
	return (0);
}

int		Server::cmd_user(std::vector<std::string> params, User user)
{
	(void) user;
	std::string name;
	std::cout << "JE SUIS DANS LA CMD USER" << std::endl;


	// for (std::vector<std::string>::iterator it = params.begin() + 4; it != params.end(); it++)
	// 	std::cout << *it << std::endl;


// USER cmarion cmarion 127.0.0.1 :Caroline MARION
	return (0);
}

int		Server::cmd_list(std::vector<std::string> params, User user)
{
	(void)params;
	std::cout << "JE SUIS DANS LE CMD LIST" << std::endl;
	//for (std::vector<std::string>::iterator it = params.begin(); it != params.end(); it++)
	//	std::cout << *it << std::endl;
	std::string	msg = ":127.0.0.1 322 ";
	msg.append(user.getNick()).append(" ");
	for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); it++)
	{
		msg.append(it->first);
		msg.append(" :");
		msg.append(it->second.getTopic());
		msg.append("\r\n");
	}
	std::cout << msg << std::endl;

	send(user.getFd().fd, &msg, sizeof(msg), 0);
	send(user.getFd().fd, ":127.0.0.1 323 test :End of /LIST\r\n", 36, 0);
	return (0);
}

int		Server::cmd_join(std::vector<std::string> params, User user)
{
	std::cout << "JE SUIS DANS LE CMD JOIN" << std::endl;
	Channel channel;
	channel.setName("#channel");
	channel.setTopic("Topic test");
	this->_channels.insert(std::make_pair(channel.getName(), channel));


	std::string msg = ":127.0.0.1 ";
	if (params.size() == 0)
	{
		msg.append("461 JOIN :Not enough parameters\r\n");
		send(user.getFd().fd, &msg, sizeof(msg), 0);
		return (0);
	}
	for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); it++)
	{
		std::string tmp = it->first;
		std::cout << params[0][params[0].size() - 2] << std::endl;
		std::cout << params[0][params[0].size() - 1];
		std::cout << "FIN|" << std::endl;
		//std::cout << "it->first :" << tmp << " | " << tmp.length() << std::endl;
		//std::cout << "params[0] :" << params[0] << " | " << params[0].length() << std::endl;
		std::cout << params[0].compare(it->first) << std::endl;
		if (it->first == params[0])
		{
			std::cout << "JE SUIS DANS LE IF" << std::endl;
			msg.append("332 " + user.getNick());
			msg.append(params[0] + " :" + it->second.getTopic()).append("\r\n");
			send(user.getFd().fd, &msg, sizeof(msg), 0);
			std::cout << msg << std::endl;
			send(user.getFd().fd, ":127.0.0.1 353 test = #channel :test\r\n", 40, 0);
			send(user.getFd().fd, ":127.0.0.1 366 test #channel :End of /NAMES list\r\n", 51, 0);
			return (0);
		}
	}

	//send(user.getFd().fd, ":127.0.0.1 332 test #channel1 :Premier channel du serveur !\r\n", 62, 0);
	//send(user.getFd().fd, ":127.0.0.1 353 test = #channel1 :test\r\n", 40, 0);
	//send(user.getFd().fd, ":127.0.0.1 366 test #channel1 :End of NAMES list\r\n", 51, 0);

	return (0);
}

int		Server::cmd_list(std::vector<std::string> params, User user)
{
	(void)params;
	std::cout << "JE SUIS DANS LE CMD LIST" << std::endl;
	//for (std::vector<std::string>::iterator it = params.begin(); it != params.end(); it++)
	//	std::cout << *it << std::endl;
	std::string	msg = ":127.0.0.1 322 ";
	msg.append(user.getNick()).append(" ");
	for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); it++)
	{
		msg.append(it->first);
		msg.append(" :");
		msg.append(it->second.getTopic());
		msg.append("\r\n");
	}
	std::cout << msg << std::endl;

	send(user.getFd().fd, &msg, sizeof(msg), 0);
	send(user.getFd().fd, ":127.0.0.1 323 test :End of /LIST\r\n", 36, 0);
	return (0);
}

int		Server::cmd_join(std::vector<std::string> params, User user)
{
	std::cout << "JE SUIS DANS LE CMD JOIN" << std::endl;
	Channel channel;
	channel.setName("#channel");
	channel.setTopic("Topic test");
	this->_channels.insert(std::make_pair(channel.getName(), channel));


	std::string msg = ":127.0.0.1 ";
	if (params.size() == 0)
	{
		msg.append("461 JOIN :Not enough parameters\r\n");
		send(user.getFd().fd, &msg, sizeof(msg), 0);
		return (0);
	}
	for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); it++)
	{
		std::string tmp = it->first;
		std::cout << params[0][params[0].size() - 2] << std::endl;
		std::cout << params[0][params[0].size() - 1];
		std::cout << "FIN|" << std::endl;
		//std::cout << "it->first :" << tmp << " | " << tmp.length() << std::endl;
		//std::cout << "params[0] :" << params[0] << " | " << params[0].length() << std::endl;
		std::cout << params[0].compare(it->first) << std::endl;
		if (it->first == params[0])
		{
			std::cout << "JE SUIS DANS LE IF" << std::endl;
			msg.append("332 " + user.getNick());
			msg.append(params[0] + " :" + it->second.getTopic()).append("\r\n");
			send(user.getFd().fd, &msg, sizeof(msg), 0);
			std::cout << msg << std::endl;
			send(user.getFd().fd, ":127.0.0.1 353 test = #channel :test\r\n", 40, 0);
			send(user.getFd().fd, ":127.0.0.1 366 test #channel :End of /NAMES list\r\n", 51, 0);
			return (0);
		}
	}

	//send(user.getFd().fd, ":127.0.0.1 332 test #channel1 :Premier channel du serveur !\r\n", 62, 0);
	//send(user.getFd().fd, ":127.0.0.1 353 test = #channel1 :test\r\n", 40, 0);
	//send(user.getFd().fd, ":127.0.0.1 366 test #channel1 :End of NAMES list\r\n", 51, 0);

	return (0);
}
