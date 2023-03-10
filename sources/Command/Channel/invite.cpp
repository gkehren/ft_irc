#include "../../../includes/Server.hpp"

int		Server::cmd_invite(std::vector<std::string> params, User &user)
{
	if (params.size() == 0 || params[0].empty())
	{
		for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); it++)
		{
			if (it->second.is_invited(params[0]) == true)
				this->add_client("336 INVITE " + it->second.getName(), user);
		}
		this->add_client("337 INVITE :End of /INVITE list", user);
		return (1);
	}
	if (params.size() != 2)
	{
		this->add_client("461 INVITE :Not enough parameters", user);
		return (0);
	}
	std::map<std::string, Channel>::iterator chan = this->_channels.find(params[1]);
	if (chan == this->_channels.end())
	{
		this->add_client("403 INVITE :No such channel", user);
		return (0);
	}
	if (chan->second.isUserInChannel(user) == false)
	{
		this->add_client("442 INVITE :You're not on that channel", user);
		return (0);
	}
	if (chan->second.isUserInChannel(this->find_user_by_name(params[0])->second) == true)
	{
		std::string sret = "443 INVITE " + params[0] + " " + params[1] + " :is already on channel";
		this->add_client(sret, user);
		return (0);
	}
	this->add_client("341 " + user.getNick() + " " + params[1] + " " + params[0], user);
	this->add_client("INVITE " + params[0] + " " + params[1], user, this->find_user_by_name(params[0])->second);
	chan->second.add_invited(params[0]);
	return (1);
}