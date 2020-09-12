#pragma once

#include "maths/types/vector2.h"

namespace engine
{
	struct mousestate
	{
		bool buttons[5];
		vector2 position;
		vector2 delta;

		mousestate()
		{
			std::fill_n(buttons, 5, 0);
		}
	};

	struct keystate
	{
		bool keys[128];

		keystate()
		{
			std::fill_n(keys, 128, 0);
		}
	};

	namespace input
	{
		extern mousestate m_current;
		extern mousestate m_previous;

		extern keystate k_current;
		extern keystate k_previous;

		void update_mbs(int mb, bool d);
		void update_mpos(float x, float y);
		void update_keys(int c, bool d);

		bool mb_down(int mb);
		bool mb_clicked(int mb);
		bool key_down(int c);
		bool key_pressed(int c);

		vector2& mouse_pos();
		vector2& mouse_delta();

		void update();
	};
}