#include "Hy3Node.hpp"

Axis getAxis(Hy3GroupLayout layout) {
	switch (layout)
	{
	case Hy3GroupLayout::SplitH:
		return Axis::Horizontal;
	case Hy3GroupLayout::SplitV:
		return Axis::Vertical;
	default:
		return Axis::None;
	}
}

Axis getAxis(ShiftDirection direction) {
	switch (direction)
	{
	case ShiftDirection::Left:
	case ShiftDirection::Right:
		return Axis::Horizontal;
	case ShiftDirection::Down:
	case ShiftDirection::Up:
		return Axis::Vertical;
	default:
		return Axis::None;
	}
}

SearchDirection getSearchDirection(ShiftDirection direction) {
	switch(direction) {
		case ShiftDirection::Left:
		case ShiftDirection::Up:
			return SearchDirection::Backwards;
		case ShiftDirection::Right:
		case ShiftDirection::Down:
			return SearchDirection::Forwards;
		default:
			return SearchDirection::None;
	}
}

char directionToChar(ShiftDirection direction) {
	switch (direction)
	{
	case ShiftDirection::Left: return 'l';
	case ShiftDirection::Up: return 'u';
	case ShiftDirection::Down: return 'd';
	case ShiftDirection::Right: return 'r';
	default: return 'r';
	}
}
