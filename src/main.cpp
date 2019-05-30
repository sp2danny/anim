
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Anim.h"

int main()
{
	sf::RenderWindow window(sf::VideoMode(640, 480), "SFML works!");
	sf::CircleShape shape(100.f);
	shape.setFillColor(sf::Color::Green);

	Anim::Init();
	Anim::AnimDir horse;
	horse.LoadExt("walk.ad");
	int c, a=90;
	horse.Instance(c = 35);
	Anim::AnimReflection ar = horse.Refl(a, c);
	Anim::Pos pos = { 320, 240 };
	ar.Start();
	
	Anim::AC horseman("horseman.ac");
	
	auto names = horseman.CoreNames();
	unsigned int i=0, n=names.size();

	horseman.Instance(c);
	Anim::AnimReflection hm = horseman.Refl(names[i], a, c);
	hm.Start();

	while (window.isOpen())
	{
		sf::Event event;
		if (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape)
					window.close();
				if (event.key.code == sf::Keyboard::Add)
				{
					c = (c + 8) % 256;
					horse.Instance(c);
					ar.ContinueWith(horse.Refl(a, c));
					horseman.Instance(c);
					hm.ContinueWith(horseman.Refl(names[i], a, c));
				}
				if (event.key.code == sf::Keyboard::Numpad6 )
				{
					ar.ContinueWith(horse.Refl(a=0, c));
					hm.ContinueWith(horseman.Refl(names[i], a, c));
				}
				if (event.key.code == sf::Keyboard::Numpad9)
				{
					ar.ContinueWith(horse.Refl(a=45, c));
					hm.ContinueWith(horseman.Refl(names[i], a, c));
				}
				if (event.key.code == sf::Keyboard::Numpad7)
				{
					ar.ContinueWith(horse.Refl(a=135, c));
					hm.ContinueWith(horseman.Refl(names[i], a, c));
				}
				if (event.key.code == sf::Keyboard::Numpad4)
				{
					ar.ContinueWith(horse.Refl(a=180, c));
					hm.ContinueWith(horseman.Refl(names[i], a, c));
				}
				if (event.key.code == sf::Keyboard::Numpad1)
				{
					ar.ContinueWith(horse.Refl(a=225, c));
					hm.ContinueWith(horseman.Refl(names[i], a, c));
				}
				if (event.key.code == sf::Keyboard::Numpad3)
				{
					ar.ContinueWith(horse.Refl(a=-45, c));
					hm.ContinueWith(horseman.Refl(names[i], a, c));
				}
				if (event.key.code == sf::Keyboard::N)
				{
					i = (i+1) % n;
					window.setTitle(names[i]);
					hm.Set(horseman.Refl(names[i], a, c));
				}
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				pos.x = event.mouseButton.x;
				pos.y = event.mouseButton.y;
			}
		}

		window.clear();
		ar.Update();
		ar.Overlay(window, pos);
		hm.Update();
		hm.Overlay(window,320,480);

		//window.draw(shape);
		window.display();
	}

	return 0;
}

#ifdef WINDOWS
int __cdecl WinMain(void*, void*, char*, int) { return main(); }
#endif
