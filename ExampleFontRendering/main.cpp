#include <SDLInstance.h>
#include <Window.h>
#include <../SDLClasses/include/Vector4.h>
#include <Texture2D.h>
#include <iostream>
#include <algorithm>
#include <SingleThreadRunner.h>
#include <../ClosestGL/Vector4.h>
#include <../SDLClasses/include/Vector2.h>
#include <ExampleCommon.h>
#include <SDLError.h>
#include <map>
#include <fstream>
#include <sstream>

using namespace ClosestGL;
using namespace ExampleCommon;

using Color = Math::Vector4<float>;

std::vector<std::wstring> Split(std::wstring str, wchar_t ch)
{
	size_t pos;
	std::vector<std::wstring> vec;
	while ((pos = str.find(ch)) != std::wstring::npos)
	{
		vec.push_back(str.substr(0, pos));
		str = str.substr(pos + 1, str.length() - pos - 1);
	}
	vec.push_back(str);
	return vec;
}

std::map<wchar_t, Math::Vector4<float>> LoadFontIndex(Math::Vector2<size_t> size)
{
	std::map<wchar_t, Math::Vector4<float>> map;

	const std::wstring index[]
	{
		L"C	faceID = 0	x = 0	y = 0	w = 45	h = 64",
		L"G	faceID = 0	x = 45	y = 0	w = 48	h = 64",
		L"L	faceID = 0	x = 93	y = 0	w = 38	h = 64",
		L"e	faceID = 0	x = 131	y = 0	w = 40	h = 64",
		L"l	faceID = 0	x = 171	y = 0	w = 26	h = 64",
		L"o	faceID = 0	x = 197	y = 0	w = 43	h = 64",
		L"s	faceID = 0	x = 0	y = 64	w = 35	h = 64",
		L"t	faceID = 0	x = 35	y = 64	w = 31	h = 64",
		L"体	faceID = 0	x = 66	y = 64	w = 60	h = 64",
		L"向	faceID = 0	x = 126	y = 64	w = 60	h = 64",
		L"场	faceID = 0	x = 186	y = 64	w = 60	h = 64",
		L"字	faceID = 0	x = 0	y = 128	w = 60	h = 64",
		L"染	faceID = 0	x = 60	y = 128	w = 60	h = 64",
		L"渲	faceID = 0	x = 120	y = 128	w = 60	h = 64",
		L"量	faceID = 0	x = 180	y = 128	w = 60	h = 64"
	};

	for (const auto& ch : index)
	{
		wchar_t chName = ch[0];

		const auto ppos = ch.rfind(L"x = ");
		auto position = ch.substr(ppos, ch.length() - ppos);

		position.erase(
			std::remove_if(position.begin(), position.end(), [](auto ch)
			{
				return
					ch == L'x' ||
					ch == L'y' ||
					ch == L'w' ||
					ch == L'h' ||
					ch == L' ' ||
					ch == L'=';
			}),
			position.end()
		);

		auto poses = Split(position, '\t');

		map[chName] = 
		{
			std::stof(poses[0]) / size.x,
			std::stof(poses[1]) / size.y,
			std::stof(poses[2]) / size.x,
			std::stof(poses[3]) / size.y
		};
	}


	return map;
}


int __cdecl main()
{
	constexpr Math::Vector2<int> ScreenSize{ 640,480 };

	SDL::SDLInstance sdl;
	SDL::Window window
	{ 
		"SDFFont Demo",
		SDL::Rect<int>
		{
			SDL::Window::Center,
			SDL::Window::Center,
			ScreenSize.x,
			ScreenSize.y
		},
		SDL::Window::WindowFlag::Null
	};


	ParallelStrategy::SingleThreadRunner runner;
	

	Texture::Texture2D<Color> renderTarget{ {640,480} };

	auto font = LoadBMP24<Color>("msyh0.bmp",runner);
	auto fontIndex = LoadFontIndex(font.GetSize());
	

	while (!sdl.QuitRequested())
	{
		//renderTarget.Clear({ 1,1,1,0 }, runner);

		renderTarget.Shade([&font](auto pos)
		{
			if (pos.x < font.GetSize().x && pos.y < font.GetSize().y)
				return font.ReadPixelUnsafe(pos);
			else
				return Color{ 0,0,0,0 };
		},runner);

		UpdateWindow(window, renderTarget);
	}

	return 0;
}
