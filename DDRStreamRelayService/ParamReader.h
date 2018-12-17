#ifndef PARAMREADER_H
#define PARAMREADER_H

#include <string>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <map>

//using namespace std;

class ParameterReader
{
public:
	ParameterReader(std::string filename = "parameters.txt")
	{
		std::ifstream fin(filename.c_str());
		if (!fin)
		{
			std::cerr << "OA_Params_SND.txt does not exist." << std::endl;
			return;
		}
		while (!fin.eof())
		{
			std::string str;
			getline(fin, str);
			if (str[0] == '#')
				continue;

			int pos = str.find("=");
			if (pos == -1)
				continue;
			std::string key = str.substr(0, pos);
			int commentPos = str.find("#");
			int valuePos;
			if (commentPos == -1)
			{
				valuePos = str.length();
			}
			else
			{
				valuePos = commentPos;
			}
			std::string value = str.substr(pos + 1, valuePos - pos - 1);
			data[key] = value;

			if (!fin.good())
				break;
		}
	}
	std::string getData(std::string key)
	{
		std::map<std::string, std::string>::iterator iter = data.find(key);
		if (iter == data.end())
		{
			std::cerr << "Parameter name " << key << " not found!" << std::endl;
			return std::string("NOT_FOUND");
		}
		return iter->second;
	}
private:
	std::map<std::string, std::string> data;
};
#endif
