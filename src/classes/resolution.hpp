#pragma once

struct ScreenResolution
{
	ScreenResolution() = default;
	ScreenResolution(int x, int y) : x(x), y(y) {}
	int x{};
	int y{};

	ScreenResolution operator/(const ScreenResolution a) const { return { x / a.x, y / a.y }; }
};