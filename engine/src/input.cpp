#include "pch.h"

#include "input.h"

namespace engine
{
	namespace input
	{
		mousestate m_current = mousestate();
		mousestate m_previous = mousestate();

		keystate k_current = keystate();
		keystate k_previous = keystate();

		void update_mbs(int mb, bool d) { m_current.buttons[mb] = d; }
		void update_mpos(float x, float y) { m_current.position.x = x; m_current.position.y = y; }
		void update_keys(int c, bool d)
		{
			k_current.keys[c] = d;
		}

		bool mb_down(int mb) { return m_current.buttons[mb]; }
		bool mb_clicked(int mb) { return m_current.buttons[mb]; }
		bool key_down(int c)
		{
			return k_current.keys[c];
		}
		bool key_pressed(int c) { return (k_current.keys[c] && !k_previous.keys[c]); }

		vector2& mouse_pos() { return m_current.position; }
		vector2& mouse_delta() { return m_current.delta; }

		void update()
		{
			m_previous = m_current;

			k_previous = k_current;
		}
	}
}