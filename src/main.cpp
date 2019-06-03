
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <utility>
#include <experimental/filesystem>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <TGUI/TGUI.hpp>

#include "Anim.h"

#include "alib.hpp"

#include "back.h"

namespace fs = std::experimental::filesystem;

extern bool LZMADecompress(const BVec& inBuf, BVec& outBuf);

void Main(const std::vector<std::string>& args)
{
	if (args.size() != 1)
	{
		std::cerr << "Usage: View <file>\n";
		return;
	}

	sf::RenderWindow window(sf::VideoMode(640, 480), "SFML works!");

	tgui::Gui gui{window};

	bool moveon = false;

	tgui::Button::Ptr start = tgui::Button::create();
	start->setSize(80, 30);
	start->setPosition(10, 10);
	start->setText("Start");
	start->connect("pressed", [&]() { moveon = true; });
	gui.add(start);

	while (window.isOpen() && !moveon)
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			bool consumed = gui.handleEvent(event);
			if (consumed)
				continue;
			if (moveon)
				break;
			if (event.type == sf::Event::KeyPressed)
			{
				/**/ if (event.key.code == sf::Keyboard::Escape)
				{
					window.close();
				}
			}
		}

		window.clear();
		gui.draw();
		window.display();
	}

	gui.remove(start);

	Anim::Init();

	int c = 35, a = 0;

	Anim::AC item(args[0]);

	alib::AC item2;
	{
		auto          fn = "Battleship.fzac"s;
		std::ifstream ifs{fn, std::fstream::binary};
		UL            sz = (UL)fs::file_size(fn);
		BVec          v1, v2;
		v1.resize(sz);
		ifs.read((char*)v1.data(), sz);
		bool ok = LZMADecompress(v1, v2);
		if (!ok)
			throw "load error";
		ok = item2.LoadFast(v2);
		if (!ok)
			throw "load error";
	}
	alib::Refl refl2 = item2.Refl("idle", 90, 55);
	refl2.Start();
	refl2.setPosition({150, 150});

	auto         names = item.CoreNames();
	unsigned int i     = 0;

	item.Instance(c);
	Anim::AnimReflection refl = item.Refl(names[i], a, c);
	refl.Start();

	auto btnAction = [&](int j) {
		i = j;
		window.setTitle(names[i]);
		refl.Set(item.Refl(names[i], a, c));
	};

	std::vector<tgui::Button::Ptr> btns;

	{
		int j = 0;
		for (auto&& str : names)
		{
			tgui::Button::Ptr btn = tgui::Button::create();
			btn->setSize(80, 30);
			btn->setPosition(10, 10 + j * 40);
			btn->setText(str);
			btn->connect("pressed", btnAction, j);
			++j;
			btns.push_back(std::move(btn));
			gui.add(btns.back());
		}
	}

	sf::Image img;
	img.create(640, 480, (sf::Uint8*)back);
	sf::Texture tx;
	tx.loadFromImage(img);
	sf::Sprite spr;
	spr.setTexture(tx);
	Anim::Pos pos{320, 240};

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			bool consumed = gui.handleEvent(event);
			if (consumed)
				continue;
			if (event.type == sf::Event::KeyPressed)
			{
				/**/ if (event.key.code == sf::Keyboard::Escape)
				{
					window.close();
				}
				else if (event.key.code == sf::Keyboard::Add)
				{
					c = (c + 8) % 256;
					item.Instance(c);
					refl.ContinueWith(item.Refl(names[i], a, c));
				}
				else if (event.key.code == sf::Keyboard::Numpad6)
				{
					refl.ContinueWith(item.Refl(names[i], a = 0, c));
				}
				else if (event.key.code == sf::Keyboard::Numpad9)
				{
					refl.ContinueWith(item.Refl(names[i], a = 45, c));
				}
				else if (event.key.code == sf::Keyboard::Numpad8)
				{
					refl.ContinueWith(item.Refl(names[i], a = 90, c));
				}
				else if (event.key.code == sf::Keyboard::Numpad7)
				{
					refl.ContinueWith(item.Refl(names[i], a = 135, c));
				}
				else if (event.key.code == sf::Keyboard::Numpad4)
				{
					refl.ContinueWith(item.Refl(names[i], a = 180, c));
				}
				else if (event.key.code == sf::Keyboard::Numpad1)
				{
					refl.ContinueWith(item.Refl(names[i], a = 225, c));
				}
				else if (event.key.code == sf::Keyboard::Numpad2)
				{
					refl.ContinueWith(item.Refl(names[i], a = 270, c));
				}
				else if (event.key.code == sf::Keyboard::Numpad3)
				{
					refl.ContinueWith(item.Refl(names[i], a = -45, c));
				}
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				pos.x = event.mouseButton.x;
				pos.y = event.mouseButton.y;
			}
		}

		window.clear();
		window.draw(spr);
		refl.Update();
		refl.Overlay(window, pos);

		refl2.Update();
		window.draw(refl2);

		gui.draw();
		window.display();
	}
}

std::vector<std::string> split(const std::string& s, char delim)
{
	std::vector<std::string> result;
	std::stringstream        ss{s};
	std::string              item;

	while (std::getline(ss, item, delim))
	{
		result.push_back(item);
	}

	return result;
}

#ifdef WINDOWS
int __cdecl WinMain(void*, void*, char* cargs, int)
{
	Main(split(cargs, ' '));
	return 0;
}
#else
int main(int argc, char* argv[])
{
	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i)
		args.push_back(argv[i]);
	Main(args);
}
#endif
