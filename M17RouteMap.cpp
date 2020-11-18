/*
 *   Copyright (c) 2020 by Thomas A. Early N7TAE
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <fstream>

#include "M17RouteMap.h"
#include "Utilities.h"

#ifndef CFG_DIR
#define CFG_DIR "/tmp/"
#endif

CM17RouteMap::~CM17RouteMap()
{
	mux.lock();
	baseMap.clear();
	cs2baseMap.clear();
	mux.unlock();
}

const std::shared_ptr<SHost> CM17RouteMap::Find(const std::string &callsign) const
{
	std::shared_ptr<SHost> rval = nullptr;
	mux.lock();
	auto cit = cs2baseMap.find(callsign);
	if (cs2baseMap.end() != cit) {
		auto bit = baseMap.find(cit->second);
		if (bit != baseMap.end())
			rval = bit->second;
	}
	mux.unlock();
	return rval;
}

const std::shared_ptr<SHost> CM17RouteMap::FindBase(const std::string &call) const
{
	std::shared_ptr<SHost> rval = nullptr;
	auto pos = call.find_first_of(" ./");
	if (pos < 3)
		return rval;
	const std::string b(call, 0, pos);
	mux.lock();
	const auto it = baseMap.find(b);
	if (it != baseMap.end())
		rval = it->second;
	mux.unlock();
	return rval;
}

void CM17RouteMap::Update(const std::string &callsign, const std::string &url, const std::string &ip4addr, const std::string &ip6addr, const uint16_t port)
{
	std::string base;
	auto pos = callsign.find_first_of(" /.");
	if (pos < 3)
		return;
	mux.lock();
	auto cit = cs2baseMap.find(callsign);
	if (cs2baseMap.end() == cit) {
		base.assign(callsign, 0, pos);
		cs2baseMap[callsign] = base;
	} else {
		base.assign(cit->second);
	}
	auto host = std::make_shared<SHost>();
	if (! url.empty())
		host->url.assign(url);
	if (! ip4addr.empty() && ip4addr.compare("none"))
		host->ip4addr.assign(ip4addr);
	if (! ip6addr.empty() && ip6addr.compare("none"))
		host->ip6addr.assign(ip6addr);
	host->port = port;
	baseMap[base] = host;
	mux.unlock();
}

void CM17RouteMap::ReadAll()
{
	mux.lock();
	baseMap.clear();
	cs2baseMap.clear();
	mux.unlock();
	Read("M17Hosts.csv");
	Read("M17Hosts.cfg");
}

void CM17RouteMap::Read(const char *filename)
{
	std::string path(CFG_DIR);
	path.append(filename);
	std::ifstream file(path, std::ifstream::in);
	if (file.is_open()) {
		std::string line;
		while (getline(file, line)) {
			trim(line);
			if (0==line.size() || '#'==line[0]) continue;
			std::vector<std::string> elem;
			split(line, ',', elem);
			Update(elem[0], elem[1], elem[2], elem[3], std::stoul(elem[4]));
		}
		file.close();
	}
}

void CM17RouteMap::Save() const
{
	std::string path(CFG_DIR);
	path.append("M17Hosts.cfg");
	std::ofstream file(path.c_str(), std::ofstream::out | std::ofstream::trunc);
	if (file.is_open()) {
		mux.lock();
		for (const auto &pair : cs2baseMap) {
			auto bit = baseMap.find(pair.second);
			if (baseMap.end() != bit) {
				const auto host = bit->second;
				if (host->url.empty()) {
					file << pair.first << ",," << host->ip4addr << ',' << host->ip6addr << ',' << host->port << ",," << std::endl;
				}
			}
		}
		file.close();
		mux.unlock();
	}
}

const std::list<std::string> CM17RouteMap::GetKeys() const
{
	std::list<std::string> keys;
	mux.lock();
	for (const auto &pair : cs2baseMap)
		keys.push_back(pair.first);
	mux.unlock();
	return keys;
}

void CM17RouteMap::Erase(const std::string &cs)
{
	mux.lock();
	auto it = cs2baseMap.find(cs);
	if (it != cs2baseMap.end())
		cs2baseMap.erase(it);
	mux.unlock();
}

size_t CM17RouteMap::Size() const
{
	return cs2baseMap.size();
}
