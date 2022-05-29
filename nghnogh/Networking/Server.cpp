#include "Server.hpp"
#include <signal.h>

// print("Content-type:text\html")
/*
** --------------------------------- METHODS ----------------------------------
*/



int Server::Create()
{


	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
		throw SocketException();

	fcntl(_fd, F_SETFL, O_NONBLOCK);

	if (bind(_fd, (struct sockaddr * ) &_addr, sizeof(_addr)) == -1)
		throw BindException();
	
	if (listen(_fd, 1000) < 0)
		throw ListenException();// 1000 10000 EDIT
	std::cout <<  "SERVER FD : " << _fd << std::endl;

	return (0);
}

/*
** --------------------------------- I/O NETWORKING ---------------------------------
*/

int		Server::accept()
{
	//int clnt = accept(_fd, (struct sockaddr * ) &_addr, sizeof(_addr));
	int clnt;
	if ((clnt = ::accept(_fd, (struct sockaddr *)&_addr, (socklen_t*)&_addrlen)) < 0)
    {
		std::cout << "[   ]" <<  _fd << "[   ]"<< "hello from accept" << std::endl;
        perror("In accept");
        exit(EXIT_FAILURE);
    }
	fcntl(clnt, F_SETFL, O_NONBLOCK);

	_requestmap.insert(std::make_pair(clnt, new _body()));

	return (clnt);
}

int		Server::send(int sock)
{
	std::map<int, _body *>::iterator	it;

	it = _requestmap.find(sock);
	if (it == _requestmap.end())
	{
		std::cout << "ERROR IN Request MAP\n";
		exit(EXIT_FAILURE);
	}


	_body *bd;


	bd = it->second;
	std::string	my_method;
	std::string	my_chunk;
	int			my_len;
	std::string	error_msg;
	std::string	request_target;
	std::string	red_target;

	// bd->handle_body
	bd->init_values(my_method, my_chunk, _index, my_len, request_target);
	
	if (it->second->_startedwrite == false)
		it->second->_startedwrite = true;




	// std::cout << request_target << std::endl;
	// std::cout << my_method << std::endl;
	// std::cout << bd->_ok.get_server(_index).get_cgi().get_cgi_block_path() << std::endl;
	// std::cout << bd->_ok.get_server(_index).get_cgi().get_cgi_path() << std::endl;
	// std::cout << bd->_http.get_my_port() << std::endl;
	// std::cout << bd->_http.get_my_host() << std::endl;
	// std::cout << bd->_http.get_value("Content-type") << std::endl;

	std::string extention;
	extention = request_target.substr(request_target.find("."));
	

	int cgi = 0;
	std::string response;
	//CGI IMPLEMENTATION
	if (extention == ".py")
	{
		cgi = 1;
		int fd = open("test", O_RDWR | O_CREAT, 0777);
		int body = open("body", O_RDWR | O_CREAT, 0777);
	
		int fork_id = fork();

		if (fork_id == -1)
		{
			std::cout << "error on fork forking" << std::endl;
			exit(0);
		}
		else if (fork_id == 0)
		{
			if (my_method == "POST")
			{
				std::string bodyy = bd->_body_stream.str();
				write(body,bodyy.c_str(),bodyy.size());
				dup2(body,0);
			}
			dup2(fd,1);
			std::string cgi_location =  bd->_ok.get_server(_index).get_cgi().get_cgi_path();
			std::string executable_script = request_target.substr(1);

			char *args[3];
			args[0] = (char *)cgi_location.c_str();
			args[1] = (char *)executable_script.c_str();
			args[2] = NULL;

			std::vector<std::string> env;

			env.push_back(std::string("GATEWAY_INTERFACE=CGI/1.1"));
			env.push_back(std::string("SERVER_SOFTWARE=webserv"));
			env.push_back(std::string("SERVER_PROTOCOL=HTTP/1.1"));
			env.push_back(std::string("SERVER_PORT=") + std::to_string( bd->_http.get_my_port() ));
			env.push_back(std::string("REQUEST_METHOD=") + my_method);
			env.push_back(std::string("PATH_INFO=") + executable_script);
			env.push_back(std::string("QUERY_STRING=empty=1&name=5"));
			env.push_back(std::string("SCRIPT_NAME=script.py"));
			env.push_back(std::string("CONTENT_TYPE=") + bd->_http.get_value("Content-Type"));
			env.push_back(std::string("CONTENT_LENGTH=") + bd->_http.get_value("Content-Length"));
			env.push_back(std::string("HTTP_USER_AGENT=") + bd->_http.get_value("User-Agent"));

			char **env_arr = new char*[env.size() + 1];

			for (int i = 0; i < env.size(); ++i)
				env_arr[i] = strdup(env[i].c_str());
			env_arr[env.size()] = NULL;
			if (execve(cgi_location.c_str(),args,env_arr) == -1)
				perror("Could not execute");
		}
		else //parent
		{
			int timout = 0;
			int status;
			time_t start = time(NULL);
			while(difftime(time(NULL),start) <= 5)
			{
				int ret = waitpid(fork_id, &status, WNOHANG);
				if (ret == 0)
					timout = 1;
				else
					timout = 0;
			}
			if (timout)
			{
				std::cout << "erreeur " << std::endl;
				kill(9,fork_id);
			}
			int nbytes;
			char cgi_buff[1024] = {0};

			lseek(fd, 0, SEEK_SET);
			std::string ret;
			while ((nbytes = read(fd, cgi_buff, 1024)) > 0)
			{
				ret.append(cgi_buff, nbytes);
			}
			Response ok;
			response = ok.parse_response_cgi(ret) ;
			ok.set_hello(response);
			std::cout << response << std::endl;
			close(fd);
		}
	

	}
	//std::cout << bd->_body_stream.str() << std::endl; /// This is where to find the body

	// std::cout << bd->_ok.set_hello(RESPONSE_KAMLA);



	// if ()








	// red_target = request_target.substr(request_target.find_last_of("/") + 1, request_target.size());
	// if (bd->_ok.get_server(_index).get_redirection_value(red_target))
	// {
	// 	request_target = red_target;
	// 	bd->_ok.set_request_target(request_target);
	// 	bd->_ok.handle_redirect_response(bd->_http.get_value("Connection"));
	// }
	// else if (my_method == "POST" && ((my_len < 0) || (bd->_body_stream.str() == "" && my_len != 0)))
	// 	bd->_ok.error_handling("400 Bad Request");
	// else
	// {
	// 	if (!bd->handle_body(my_method, my_chunk, error_msg, my_len))
	// 		bd->_ok.error_handling("400 Bad Request");
	// 	else
	// 	{
	// 		if (bd->_ok.get_max_body_size() < 0)
	// 			bd->_ok.error_handling("500 Webservice currently unavailable");
	// 		else if (bd->_http.get_total_size() > bd->_ok.get_max_body_size() && bd->_ok.get_max_body_size() != 0)
	// 			bd->_ok.error_handling("413 Payload Too Large");
	// 		else
	// 		{
	// 			if (error_msg != "")
	// 				bd->_ok.error_handling(error_msg);
	// 			else
	// 				bd->handle_response(my_method);
				
	// 		}			
	// 	}
	// }
	if (cgi == 0)
		bd->_writecount += write(sock , bd->_ok.get_hello() + bd->_writecount , bd->_ok.get_total_size() - bd->_writecount);
	else
		write(sock, response.c_str(),response.size());

	//bd->_ok.clear();
	//bd->close_file();
	//delete[] bd;
	if (bd->_ok.get_total_size() <= bd->_writecount)
	{
		_requestmap.erase(it);
		return 0;
	}
	else
		return (1);

}
int		Server::recv(int sock)
{
	std::map<int, _body *>::iterator	it;
	int									flag;

	it = _requestmap.find(sock);
	if (it ==  _requestmap.end())
	{
		std::cout << "couldnt receve request\n";
		//exit(EXIT_FAILURE);
	}
	_body *bd = it->second;
	flag = bd->_http.handle_http_request(sock, bd->_body_file, bd->_body_size, bd->_body_stream);
	return flag;
}

/*
** --------------------------------- Modefiers ---------------------------------
*/

int Server::SAcceptCon()
{
	// std::cout << "hello from accept§" << std::endl;
	// //int clnt = accept(_fd, (struct sockaddr * ) &_addr, sizeof(_addr));
	// int clnt;
	// if ((clnt = ::accept(_fd, (struct sockaddr *)&_addr, (socklen_t*)&_addrlen)) < 0)
    // {
    //     perror("In accept");
    //     exit(EXIT_FAILURE);
    // }
	// fcntl(clnt, F_SETFL, O_NONBLOCK);

	// return (clnt);
	return 0;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

	void 	Server::setSocket(std::string hoststring, int port)
	{
		//TODO: change to network/host EQUIVALENT
		_port = port;//htons(port);
		_host = inet_addr(hoststring.c_str());
		_hoststring = hoststring;
		setAddress();
	}

	void	Server::setAddress()
	{
		memset((char *)&_addr, 0, sizeof(_addr)); 
    	_addr.sin_family = AF_INET;
    	_addr.sin_port =  htons(_port);//htons(_port);
		//std::cout << _host << std::endl;
    	_addr.sin_addr.s_addr = _host;//INADDR_ANY;//_host;//htonl(_host);0.0.0.0 127.0.0.1
		_addrlen = sizeof(_addr);
	}

	int		Server::getsocketfd()
	{
		return _fd;
	}



		void		Server::setIndex(int i)
		{
			_index = i;
		}
		int			Server::getIndex()
		{
			return _index;
		}

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Server::Server()
{

}

Server::Server( const Server & src )
{
}

Server::Server(unsigned int host, int port)
{
	_host = host;
	_port = port;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Server::~Server()
{
}


/*
** --------------------------------- OVERLOAD ---------------------------------
*/

Server &				Server::operator=( Server const & rhs )
{
	//if ( this != &rhs )
	//{
		//this->_value = rhs.getValue();
	//}
	return *this;
}

std::ostream &			operator<<( std::ostream & o, Server const & i )
{
	//o << "Value = " << i.getValue();
	return o;
}

/* ************************************************************************** */