#include "../../../includes/Server.hpp"

int		Server::cmd_password(std::vector<std::string> params, User &user)
{
	std::string pass;
	if (params[0].empty())
	{
		this->send_client(":127.0.0.1 461 PASS : Not enough parameters", user.getFd());
		this->disconnect(user);
		return (0);
	}
	for (std::vector<std::string>::iterator it = params.begin(); it != params.end(); it++)
		pass = pass + *it;
	if (this->_password != pass)
	{
		this->send_client(":127.0.0.1 464 : Password incorrect", user.getFd());
		this->disconnect(user);
		return (0);
	}
	else
		user._auth_ok.pass = true;
	return (1);//question pour le code ERR_ALREADYREGISTERED (462)
}