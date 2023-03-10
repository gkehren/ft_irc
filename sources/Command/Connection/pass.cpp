#include "../../../includes/Server.hpp"

int		Server::cmd_password(std::vector<std::string> params, User &user)
{
	std::string pass;
	if (user.isAut() == true)
	{
		this->add_client("462 PASS :You may not reregister", user);
		return (1);
	}
	if (params.size() == 0 || params[0].empty())
	{
		this->add_client("461 PASS :Not enough parameters", user);
		this->disconnect(user);
		return (0);
	}
	for (std::vector<std::string>::iterator it = params.begin(); it != params.end(); it++)
		pass = pass + *it;
	if (this->_password != pass)
	{
		this->add_client("464 :Password incorrect", user);
		this->disconnect(user);
		return (0);
	}
	else
		user.auth_ok.pass = true;
	return (1);
}
