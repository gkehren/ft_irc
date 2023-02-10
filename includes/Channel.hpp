#pragma once
#include "User.hpp"
#include <map>
#include <string>
#include <vector>

class User;

class Channel
{
	private:
		std::string		_name;
		std::string		_topic;
		std::map<int, User>	_users;

	public:
		Channel();
		virtual ~Channel();

		/*	GETTER	*/
		std::string			getName() const;
		std::string			getTopic() const;
		std::vector<User>	getUsers() const;

		/*	SETTER	*/
		void				setName(std::string name);
		void				setTopic(std::string topic);

		/*	FUNCTIONS	*/
		bool				isUserInChannel(User user);
		void				addUser(User user);
		void				removeUser(User user);
		void				removeUser(std::string nick);
};
