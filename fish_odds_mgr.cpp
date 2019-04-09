#include "stdafx.h"
#include "fish_odds_mgr.h"
#include "fish_odds_single.h"
#include "fish_odds_global.h"
#include "fish_odds_simple.h"
#include "logic_player.h"
#include "enable_random.h"
#include "game_db.h"

FISH_SPACE_USING

fish_odds_mgr::fish_odds_mgr()
{
	fish_odds_base* odds_base = new fish_odds_base();
	odds_base->init();
	m_fish_odds_map[odds_base->get_odds_type()] = odds_base;

	odds_base = new fish_odds_single();
	odds_base->init();
	m_fish_odds_map[odds_base->get_odds_type()] = odds_base;

	odds_base = new fish_odds_global();
	odds_base->init();
	m_fish_odds_map[odds_base->get_odds_type()] = odds_base;

	odds_base = new fish_odds_simple();
	odds_base->init();
	m_fish_odds_map[odds_base->get_odds_type()] = odds_base;

	m_odds_mode = 2;
	m_odds_elapsed = 0;
	check_state();
}


fish_odds_mgr::~fish_odds_mgr(void)
{
	for (auto it = m_fish_odds_map.begin(); it != m_fish_odds_map.end(); it++)
	{
		it->second->release();
		delete it->second;
		it->second = nullptr;
	}
}

void fish_odds_mgr::heartbeat(double elapsed)
{
	m_odds_elapsed += elapsed;
	if (m_odds_elapsed >= 30)
	{
		m_odds_elapsed = 0;
		check_state();
	}

	for (auto it = m_fish_odds_map.begin(); it != m_fish_odds_map.end(); it++)
	{
		it->second->heartbeat(elapsed);
	}
}


void fish_odds_mgr::init_player(logic_player* player)
{
	if (player->is_robot())
	{
		return;
	}
	if (player->is_ex_room())
		return;

	if (m_odds_mode == 1)
	{
		int player_value = player->get_pid()%10;
		int value = 10/m_open_odds.size();

		if (player_value >= 0 && player_value < value)
		{
			attacth_fish_odds(player, (odds_type)m_open_odds[0]);
		}
		else if (player_value > value && player_value < 2*value)
		{
			attacth_fish_odds(player, (odds_type)m_open_odds[1]);
		}
		else
		{
			attacth_fish_odds(player, (odds_type)m_open_odds[2]);
		}
	}
	else
	{
		int value = global_random::instance().rand_int(0, m_open_odds.size()-1);
		attacth_fish_odds(player, (odds_type)m_open_odds[value]);
	}
}

void fish_odds_mgr::release_player(logic_player* player)
{
	detach_fish_odds(player);
}

void fish_odds_mgr::attacth_fish_odds(logic_player* player, odds_type type)
{
	if (player->get_fish_odds() != nullptr)
	{
		detach_fish_odds(player);
	}

	auto it = m_fish_odds_map.find(type);
	if (it != m_fish_odds_map.end())
	{
		it->second->attacth_player(player);
		player->set_fish_odds(it->second);
	}
}

void fish_odds_mgr::detach_fish_odds(logic_player* player)
{
	if (player->get_fish_odds() != nullptr)
	{
		player->get_fish_odds()->detach_player(player);
		player->set_fish_odds(nullptr);
	}
}

void fish_odds_mgr::check_state()
{
	mongo::BSONObj rcond;
	std::vector<mongo::BSONObj> vec;
	db_game::instance().find(vec, DB_ODDSCFG, rcond);

	m_open_odds.clear();
	for (int i = 0; i < vec.size(); i++)
	{
		if (vec[i].hasField("key") && vec[i].hasField("value"))
		{
			std::string key = vec[i].getStringField("key");
			int value = vec[i].getField("value").numberInt();
			if (key == "odds_mode")
			{
				m_odds_mode = value;
			}
			else if (key == "open_odds1")
			{
				if (value >= 1)
					m_open_odds.push_back(1);
			}
			else if (key == "open_odds2")
			{
				if (value >= 1)
					m_open_odds.push_back(2);
			}
			else if (key == "open_odds3")
			{
				if (value >= 1)
					m_open_odds.push_back(3);
			}
		}
	}

	if (m_odds_mode <= 0 && m_odds_mode >= 3)
	{
		m_odds_mode = 2;
	}
	if (m_open_odds.size() == 0)
	{
		m_open_odds.push_back(1);
		m_open_odds.push_back(2);
		m_open_odds.push_back(3);
	}
}