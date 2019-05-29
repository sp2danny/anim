
#include <SFML/Graphics.hpp>

#include "Anim.h"

int main()
{
	sf::RenderWindow window(sf::VideoMode(200, 200), "SFML works!");
	sf::CircleShape shape(100.f);
	shape.setFillColor(sf::Color::Green);

	Anim::AnimDir horse;
	horse.LoadExt("walk.ad");
	horse.Instance(35);
	Anim::AnimReflection ar = horse.Refl(90,35);

	int i=0;
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		if ((++i%50)==0)
			ar.Next();
		window.clear();
		ar.Overlay(window, 100,100);

		//window.draw(shape);
		window.display();
	}

	return 0;
}

