
#include <SFML/Graphics.hpp>

#include "Anim.h"

int main()
{
	sf::RenderWindow window(sf::VideoMode(640, 480), "SFML works!");
	sf::CircleShape shape(100.f);
	shape.setFillColor(sf::Color::Green);

	Anim::AnimDir horse;
	horse.LoadExt("walk.ad");
	int c, a=90;
	horse.Instance(c = 35);
	Anim::AnimReflection ar = horse.Refl(a, c);
	Anim::Pos pos = { 320, 240 };

	int i=0;
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Add)
				{
					c = (c + 8) % 255;
					//horse.UnInstance();
					horse.Instance(c);
					auto curr = ar.Current();
					ar = horse.Refl(a, c);
					ar.Current(curr);
				}
				if (event.key.code == sf::Keyboard::Numpad6 )
				{
					auto curr = ar.Current();
					ar = horse.Refl(a=0, c);
					ar.Current(curr);
				}
				if (event.key.code == sf::Keyboard::Numpad9)
				{
					auto curr = ar.Current();
					ar = horse.Refl(a=45, c);
					ar.Current(curr);
				}
				if (event.key.code == sf::Keyboard::Numpad7)
				{
					auto curr = ar.Current();
					ar = horse.Refl(a=135, c);
					ar.Current(curr);
				}
				if (event.key.code == sf::Keyboard::Numpad4)
				{
					auto curr = ar.Current();
					ar = horse.Refl(a = 180, c);
					ar.Current(curr);
				}
				if (event.key.code == sf::Keyboard::Numpad1)
				{
					auto curr = ar.Current();
					ar = horse.Refl(a = 225, c);
					ar.Current(curr);
				}
				if (event.key.code == sf::Keyboard::Numpad3)
				{
					auto curr = ar.Current();
					ar = horse.Refl(a = -45, c);
					ar.Current(curr);
				}
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				pos.x = event.mouseButton.x;
				pos.y = event.mouseButton.y;
			}
		}

		if ((++i%250)==0)
			ar.Next();
		window.clear();
		ar.Overlay(window, pos);

		//window.draw(shape);
		window.display();
	}

	return 0;
}

#ifdef WINDOWS
int __cdecl WinMain(void*, void*, char*, int) { return main(); }
#endif
