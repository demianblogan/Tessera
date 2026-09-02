#include "KeyName.h"

#include <string>

namespace Input
{
	sf::String KeyName(sf::Keyboard::Scancode key)
	{
		using S = sf::Keyboard::Scancode;

		// Letters and digits: the plain character.
		if (key >= S::A && key <= S::Z)
		{
			return sf::String(static_cast<char>('A' + (static_cast<int>(key) - static_cast<int>(S::A))));
		}
		if (key >= S::Num1 && key <= S::Num9)
		{
			return sf::String(static_cast<char>('1' + (static_cast<int>(key) - static_cast<int>(S::Num1))));
		}
		if (key == S::Num0)
		{
			return "0";
		}
		if (key >= S::F1 && key <= S::F24)
		{
			return sf::String("F" + std::to_string(static_cast<int>(key) - static_cast<int>(S::F1) + 1));
		}
		if (key >= S::Numpad1 && key <= S::Numpad9)
		{
			return sf::String("Num " + std::to_string(static_cast<int>(key) - static_cast<int>(S::Numpad1) + 1));
		}

		switch (key)
		{
		case S::Numpad0:        return "Num 0";
		case S::Space:          return "Space";
		case S::Enter:          return "Enter";
		case S::Escape:         return "Esc";
		case S::Backspace:      return "Backspace";
		case S::Tab:            return "Tab";
		case S::Hyphen:         return "-";
		case S::Equal:          return "=";
		case S::LBracket:       return "[";
		case S::RBracket:       return "]";
		case S::Backslash:      return "\\";
		case S::Semicolon:      return ";";
		case S::Apostrophe:     return "'";
		case S::Grave:          return "`";
		case S::Comma:          return ",";
		case S::Period:         return ".";
		case S::Slash:          return "/";
		case S::NonUsBackslash: return "\\";
		case S::Left:           return "Left";
		case S::Right:          return "Right";
		case S::Up:             return "Up";
		case S::Down:           return "Down";
		case S::Insert:         return "Insert";
		case S::Delete:         return "Delete";
		case S::Home:           return "Home";
		case S::End:            return "End";
		case S::PageUp:         return "Page Up";
		case S::PageDown:       return "Page Down";
		case S::CapsLock:       return "Caps Lock";
		case S::NumLock:        return "Num Lock";
		case S::ScrollLock:     return "Scroll Lock";
		case S::PrintScreen:    return "Print Screen";
		case S::Pause:          return "Pause";
		case S::Menu:           return "Menu";
		case S::Application:    return "App";
		case S::NumpadDivide:   return "Num /";
		case S::NumpadMultiply: return "Num *";
		case S::NumpadMinus:    return "Num -";
		case S::NumpadPlus:     return "Num +";
		case S::NumpadEqual:    return "Num =";
		case S::NumpadEnter:    return "Num Enter";
		case S::NumpadDecimal:  return "Num .";
		case S::LControl:       return "Left Ctrl";
		case S::RControl:       return "Right Ctrl";
		case S::LShift:         return "Left Shift";
		case S::RShift:         return "Right Shift";
		case S::LAlt:           return "Left Alt";
		case S::RAlt:           return "Right Alt";
		case S::LSystem:        return "Left Win";
		case S::RSystem:        return "Right Win";
		default:               break;
		}

		return "Key " + std::to_string(static_cast<int>(key));
	}
}
